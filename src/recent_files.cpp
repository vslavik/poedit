/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 2020-2021 Vaclav Slavik
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a
 *  copy of this software and associated documentation files (the "Software"),
 *  to deal in the Software without restriction, including without limitation
 *  the rights to use, copy, modify, merge, publish, distribute, sublicense,
 *  and/or sell copies of the Software, and to permit persons to whom the
 *  Software is furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in
 *  all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 *  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 *  DEALINGS IN THE SOFTWARE.
 *
 */

#include "recent_files.h"

#include "colorscheme.h"
#include "hidpi.h"
#include "str_helpers.h"
#include "unicode_helpers.h"
#include "utility.h"

#ifdef __WXOSX__
#import <AppKit/NSDocumentController.h>
#endif

#include <wx/config.h>
#include <wx/filehistory.h>
#include <wx/filename.h>
#include <wx/log.h>
#include <wx/menu.h>
#include <wx/mimetype.h>
#include <wx/renderer.h>
#include <wx/settings.h>
#include <wx/weakref.h>
#include <wx/xrc/xmlres.h>

#ifdef __WXMSW__
#include <wx/generic/private/markuptext.h>
#endif

#include <memory>
#include <mutex>
#include <map>
#include <set>
#include <vector>

wxDEFINE_EVENT(EVT_OPEN_RECENT_FILE, wxCommandEvent);

namespace
{

// Track lifetime of menus, removes no longer existing menus automatically
template<typename Payload>
class menus_tracker
{
public:
    void add(wxMenuItem *menuItem, Payload *payload)
    {
        cleanup_destroyed();
        m_menus[menuItem->GetMenu()] = payload;
    }

    template<typename Fn>
    Payload* find_if(Fn&& predicate)
    {
        cleanup_destroyed();
        for (auto& m: m_menus)
        {
            if (predicate(m.first))
                return m.second;
        }
        return nullptr;
    }

    template<typename Fn>
    void for_all(Fn&& func)
    {
        cleanup_destroyed();
        for (auto& m: m_menus)
            func(m.second);
    }

    virtual ~menus_tracker() {}

protected:
    void cleanup_destroyed()
    {
        auto i = m_menus.begin();
        while (i != m_menus.end())
        {
            if (i->first)
                ++i;
            else
                i = m_menus.erase(i);
        }
    }

    std::map<wxWeakRef<wxMenu>, Payload*> m_menus;
};


#ifndef __WXOSX__

class file_icons
{
public:
    file_icons()
    {
        m_iconSize[icon_small] = wxSystemSettings::GetMetric(wxSYS_SMALLICON_X);
        m_iconSize[icon_large] = wxSystemSettings::GetMetric(wxSYS_ICON_X);
    }

    wxBitmap get_small(const wxString& ext) { return do_get(ext, icon_small);  }
    wxBitmap get_large(const wxString& ext) { return do_get(ext, icon_large); }

private:
    enum icon_size
    {
        icon_small = 0,
        icon_large = 1,
        icon_max
    };

    wxBitmap do_get(const wxString& ext, icon_size size)
    {
        auto& cache = m_cache[size];
        auto i = cache.find(ext);
        if (i != cache.end())
            return i->second;

        std::unique_ptr<wxFileType> ft(wxTheMimeTypesManager->GetFileTypeFromExtension(ext));
        if (ft)
        {
            wxIconLocation icon;
            if (ft->GetIcon(&icon))
                return cache.emplace(ext, create_bitmap(icon, size)).first->second;
        }

        cache.emplace(ext, wxNullBitmap);
        return wxNullBitmap;
    }

    wxBitmap create_bitmap(const wxIconLocation& loc, icon_size size)
    {
        wxString fullname = loc.GetFileName();
#ifdef __WXMSW__
        if (loc.GetIndex())
        {
            // wxICOFileHandler accepts names in the format "filename;index"
            fullname << ';' << loc.GetIndex();
        }
#endif
        wxIcon icon(fullname, wxBITMAP_TYPE_ICO, m_iconSize[size], m_iconSize[size]);
        if (!icon.IsOk())
            icon.LoadFile(fullname, wxBITMAP_TYPE_ICO);

        return icon;
    }

    int m_iconSize[icon_max];
    std::map<wxString, wxBitmap> m_cache[icon_max];
};

typedef std::shared_ptr<file_icons> file_icons_ptr;

#endif // !__WXOSX__

} // anonymous namespace


#ifdef __WXOSX__

// Implementation for macOS uses native recent documents functionality with native UI.
// Some gross hacks are however required to make it work with wx's menubar and mostly
// with the way wx handles switching per-window menus on macOS where only one per-app
// menu exists.
class RecentFiles::impl
{
public:
    impl() {}

    void UseMenu(wxMenuItem *menuItem)
    {
        NSMenu *native = menuItem->GetMenu()->GetHMenu();
        NSMenuItem *nativeItem = [native itemWithTitle:str::to_NS(menuItem->GetItemLabelText())];
        wxCHECK_RET( nativeItem, "couldn't find NSMenuItem for a menu item" );
        m_menus.add(menuItem, nativeItem);
    }

    void NoteRecentFile(wxFileName fn)
    {
        fn.Normalize(wxPATH_NORM_DOTS | wxPATH_NORM_ABSOLUTE);
        NSURL *url = [NSURL fileURLWithPath:str::to_NS(fn.GetFullPath())];
        [[NSDocumentController sharedDocumentController] noteNewRecentDocumentURL:url];
    }

    std::vector<wxFileName> GetRecentFiles()
    {
        std::vector<wxFileName> f;
        NSArray<NSURL*> *urls = [[NSDocumentController sharedDocumentController] recentDocumentURLs];
        for (NSURL *url in urls)
            f.emplace_back(str::to_wx(url.path));
        return f;
    }

    void MacCreateFakeOpenRecentMenu()
    {
        // Populate the menu with a hack that will be replaced.
        NSMenu *mainMenu = [NSApp mainMenu];

        NSMenuItem *item = [mainMenu addItemWithTitle:@"File" action:NULL keyEquivalent:@""];
        NSMenu *menu = [[NSMenu alloc] initWithTitle:NSLocalizedString(@"File", nil)];

        item = [menu addItemWithTitle:NSLocalizedString(@"Open Recent", nil)
                action:NULL
                keyEquivalent:@""];
        NSMenu *openRecentMenu = [[NSMenu alloc] initWithTitle:@"Open Recent"];
        #pragma clang diagnostic push
        #pragma clang diagnostic ignored "-Wundeclared-selector"
        [openRecentMenu performSelector:@selector(_setMenuName:) withObject:@"NSRecentDocumentsMenu"];
        #pragma clang diagnostic pop
        [menu setSubmenu:openRecentMenu forItem:item];
        m_recentMenuItem = item;
        m_recentMenu = openRecentMenu;

        item = [openRecentMenu addItemWithTitle:NSLocalizedString(@"Clear Menu", nil)
                action:@selector(clearRecentDocuments:)
                keyEquivalent:@""];
    }

    void MacTransferMenuTo(wxMenuBar *bar)
    {
        if (m_recentMenuItem)
            [m_recentMenuItem setSubmenu:nil];

        if (!bar)
            return;

        NSMenuItem *nativeItem = m_menus.find_if([=](wxMenu *menu){ return menu->GetMenuBar() == bar; });
        if (nativeItem)
        {
            [nativeItem setSubmenu:m_recentMenu];
            m_recentMenuItem = nativeItem;
        }
    }

private:
    menus_tracker<NSMenuItem> m_menus;
    NSMenu *m_recentMenu = nullptr;
    NSMenuItem *m_recentMenuItem = nullptr;
};

#else // !__WXOSX__

// Generic implementation use wxFileHistory (mainly for Windows). Doesn't use
// wxFileHistory's menus management, because it requires explicit RemoveMenu()
// and because we want to add "Clear menu" items as well.
class RecentFiles::impl
{
public:
    impl()
        : m_icons_cache(new file_icons),
          m_history(m_icons_cache)
    {
        wxConfigBase *cfg = wxConfig::Get();
        cfg->SetPath("/");
        m_history.Load(*cfg);
    }

    void UseMenu(wxMenuItem *menuItem)
    {
        auto menu = menuItem->GetSubMenu();
        m_menus.add(menuItem, menu);

        RebuildMenu(menu);

        menu->Bind(wxEVT_MENU, [=](wxCommandEvent& e)
        {
            auto f = GetRecentFiles()[e.GetId() - wxID_FILE1].GetFullPath();
            if (!wxFileExists(f))
            {
                wxLogError(_(L"File “%s” doesn’t exist."), f.c_str());
                return;
            }

            wxCommandEvent event(EVT_OPEN_RECENT_FILE);
            event.SetEventObject(menu);
            event.SetString(f);
            menu->GetWindow()->ProcessWindowEvent(event);
        },
        wxID_FILE1, wxID_FILE9);

        menu->Bind(wxEVT_MENU, [=](wxCommandEvent&)
        {
            ClearHistory();
        },
        m_idClear);
    }

    void NoteRecentFile(wxFileName fn)
    {
        fn.Normalize(wxPATH_NORM_DOTS | wxPATH_NORM_ABSOLUTE);
        m_history.AddFileToHistory(fn.GetFullPath());

        UpdateAfterChange();
    }

    std::vector<wxFileName> GetRecentFiles()
    {
        return m_history.GetRecentFiles();
    }

    void ClearHistory()
    {
        while (m_history.GetCount())
            m_history.RemoveFileFromHistory(0);

        UpdateAfterChange();
    }

    file_icons_ptr GetIconsCache() const { return m_icons_cache; }

protected:
    void RebuildMenu(wxMenu *menu)
    {
        // clear the menu entirely:
        while (menu->GetMenuItemCount())
            menu->Destroy(menu->FindItemByPosition(0));

        // add wxFileHistory files:
        m_history.AddFilesToMenu(menu);

        // and an item for clearning the menu:
        const bool hasItems = menu->GetMenuItemCount() > 0;
        if (hasItems)
            menu->AppendSeparator();
        auto clearItem = menu->Append(m_idClear, MSW_OR_OTHER(_("Clear menu"), _("Clear Menu")));
        clearItem->Enable(hasItems);

    }

    void UpdateAfterChange()
    {
        // Update all menus with visible history:
        m_menus.for_all([=](wxMenu *menu){ RebuildMenu(menu); });

        // Save the changes to persistent storage:
        wxConfigBase *cfg = wxConfig::Get();
        cfg->SetPath("/");
        m_history.Save(*cfg);
    }

    // customized implementation of storage that makes nicer menus
    class MyHistory : public wxFileHistory
    {
    public:
        MyHistory(file_icons_ptr icons_cache) : m_icons_cache(icons_cache) {}

        std::vector<wxFileName> GetRecentFiles()
        {
            std::vector<wxFileName> files;
            files.reserve(m_fileHistory.size());
            for (auto& f : m_fileHistory)
            {
                if (wxFileName::FileExists(f))
                    files.emplace_back(f);
            }
            return files;
        }

        void AddFilesToMenu(wxMenu *menu) override
        {
            auto files = GetRecentFiles();

            std::map<wxString, int> nameUses;
            for (auto& f : files)
            {
                auto name = f.GetFullName();
                nameUses[name] += 1;
            }

            for (size_t i = 0; i < files.size(); ++i)
                DoAddFile(menu, i, files[i], nameUses[files[i].GetFullName()] > 1);
        }

    protected:
        void DoAddFile(wxMenu* menu, int n, const wxFileName& fn, bool showFullPath)
        {
            auto menuEntry = bidi::platform_mark_direction(
                                 showFullPath
                                 ? wxString::Format(L"%s — %s", fn.GetFullName(), fn.GetPath())
                                 : fn.GetFullName());

            // we need to quote '&' characters which are used for mnemonics
            menuEntry.Replace("&", "&&");

            auto item = menu->Append(wxID_FILE1 + n, wxString::Format("&%d %s", n + 1, menuEntry));
            item->SetHelp(fn.GetFullPath());
            item->SetBitmap(m_icons_cache->get_small(fn.GetExt()));
        }

        file_icons_ptr m_icons_cache;
    };

private:
    const wxWindowID m_idClear = wxNewId();

    file_icons_ptr m_icons_cache;
    MyHistory m_history;
    menus_tracker<wxMenu> m_menus;
};

#endif // !__WXOSX__



// Boilerplate:


namespace
{
    static std::once_flag initializationFlag;
    RecentFiles* gs_instance = nullptr;
}

RecentFiles& RecentFiles::Get()
{
    std::call_once(initializationFlag, []() {
        gs_instance = new RecentFiles;
    });
    return *gs_instance;
}

void RecentFiles::CleanUp()
{
    if (gs_instance)
    {
        delete gs_instance;
        gs_instance = nullptr;
    }
}

RecentFiles::RecentFiles() : m_impl(new impl) {}
RecentFiles::~RecentFiles() {}

void RecentFiles::UseMenu(wxMenuItem *menu)
{
    m_impl->UseMenu(menu);
}

void RecentFiles::NoteRecentFile(const wxFileName& fn)
{
    m_impl->NoteRecentFile(fn);
}

std::vector<wxFileName> RecentFiles::GetRecentFiles()
{
    return m_impl->GetRecentFiles();
}


#ifdef __WXOSX__
void RecentFiles::MacCreateFakeOpenRecentMenu()
{
    m_impl->MacCreateFakeOpenRecentMenu();
}

void RecentFiles::MacTransferMenuTo(wxMenuBar *bar)
{
    m_impl->MacTransferMenuTo(bar);
}
#endif // __WXOSX__



class RecentFilesCtrl::MultilineTextRenderer : public wxDataViewTextRenderer
{
public:
    MultilineTextRenderer() : wxDataViewTextRenderer()
    {
#if wxCHECK_VERSION(3,1,1)
        EnableMarkup();
#endif
    }

#ifdef __WXMSW__
    bool Render(wxRect rect, wxDC *dc, int state)
    {
        int flags = 0;
        if ( state & wxDATAVIEW_CELL_SELECTED )
            flags |= wxCONTROL_SELECTED;

        for (auto& line: wxSplit(m_text, '\n'))
        {
            wxItemMarkupText markup(line);
            markup.Render(GetView(), *dc, rect, flags, GetEllipsizeMode());
            rect.y += rect.height / 2;
        }

        return true;
    }

    wxSize GetSize() const
    {
        if (m_text.empty())
            return wxSize(wxDVC_DEFAULT_RENDERER_SIZE,wxDVC_DEFAULT_RENDERER_SIZE);

        auto size = wxDataViewTextRenderer::GetSize();
        size.y *= 2; // approximation enough for our needs
        return size;
    }
#endif // __WXMSW__
};

struct RecentFilesCtrl::data
{
    std::vector<wxFileName> files;
#ifndef __WXOSX__
    file_icons_ptr icons_cache;
#endif
};


// TODO: merge with CrowdinFileList which is very similar and has lot of duplicated code
RecentFilesCtrl::RecentFilesCtrl(wxWindow *parent)
    : wxDataViewListCtrl(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxDV_NO_HEADER | wxBORDER_NONE),
      m_data(new data)
{
#ifdef __WXOSX__
    NSScrollView *scrollView = (NSScrollView*)GetHandle();
    scrollView.automaticallyAdjustsContentInsets = NO;

    NSTableView *tableView = (NSTableView*)[scrollView documentView];
    tableView.selectionHighlightStyle = NSTableViewSelectionHighlightStyleSourceList;
    [tableView setIntercellSpacing:NSMakeSize(0.0, 0.0)];
    const int icon_column_width = PX(32 + 12);
#else // !__WXOSX__
    ColorScheme::SetupWindowColors(this, [=]
    {
        SetBackgroundColour(ColorScheme::Get(Color::SidebarBackground));
    });

    m_data->icons_cache = RecentFiles::Get().m_impl->GetIconsCache();
    const int icon_column_width = wxSystemSettings::GetMetric(wxSYS_ICON_X) + PX(12);
#endif

#if wxCHECK_VERSION(3,1,1)
    SetRowHeight(PX(46));
#endif

    AppendBitmapColumn("", 0, wxDATAVIEW_CELL_INERT, icon_column_width);
    auto renderer = new MultilineTextRenderer();
    auto column = new wxDataViewColumn(_("File"), renderer, 1, -1, wxALIGN_NOT, wxDATAVIEW_COL_RESIZABLE);
    AppendColumn(column, "string");

    ColorScheme::SetupWindowColors(this, [=]{ RefreshContent(); });

    Bind(wxEVT_DATAVIEW_ITEM_ACTIVATED, &RecentFilesCtrl::OnActivate, this);

    wxGetTopLevelParent(parent)->Bind(wxEVT_SHOW, [=](wxShowEvent& e){
        e.Skip();
        RefreshContent();
    });
}

void RecentFilesCtrl::RefreshContent()
{
#ifdef __WXGTK__
    auto secondaryFormatting = "alpha='50%'";
#else
    auto secondaryFormatting = wxString::Format("foreground='%s'", ColorScheme::Get(Color::SecondaryLabel).GetAsString(wxC2S_HTML_SYNTAX));
#endif

    DeleteAllItems();

    m_data->files = RecentFiles::Get().GetRecentFiles();
    for (auto f : m_data->files)
    {
#ifndef __WXMSW__
        f.ReplaceHomeDir();
#endif

#if wxCHECK_VERSION(3,1,1)
        wxString text = wxString::Format
        (
            "%s\n<small><span %s>%s</span></small>",
            EscapeMarkup(f.GetFullName()),
            secondaryFormatting,
            EscapeMarkup(f.GetPath())
        );
#else
        wxString text = f.GetFullPath();
#endif

#ifdef __WXOSX__
        wxBitmap icon([[NSWorkspace sharedWorkspace] iconForFileType:str::to_NS(f.GetExt())]);
#else
        wxBitmap icon(m_data->icons_cache->get_large(f.GetExt()));
#endif
        wxVector<wxVariant> data;
        data.push_back(wxVariant(icon));
        data.push_back(text);
        AppendItem(data);
    }
}

void RecentFilesCtrl::OnActivate(wxDataViewEvent& event)
{
    auto index = ItemToRow(event.GetItem());
    if (index == wxNOT_FOUND || index >= (int)m_data->files.size())
        return;

    auto fn = m_data->files[index].GetFullPath();

    wxCommandEvent cevent(EVT_OPEN_RECENT_FILE);
    cevent.SetEventObject(this);
    cevent.SetString(fn);
    ProcessWindowEvent(cevent);
}
