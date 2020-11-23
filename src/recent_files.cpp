/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 2020 Vaclav Slavik
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
#include <wx/weakref.h>
#include <wx/xrc/xmlres.h>

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
            wxString f(m_history.GetHistoryFile(e.GetId() - wxID_FILE1));
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

    void ClearHistory()
    {
        while (m_history.GetCount())
            m_history.RemoveFileFromHistory(0);

        UpdateAfterChange();
    }

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
        void AddFilesToMenu(wxMenu *menu) override
        {
            std::vector<wxFileName> files;
            files.reserve(m_fileHistory.size());
            for (auto& f : m_fileHistory)
                files.emplace_back(f);

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
        }

        file_icons m_icons;
    };

private:
    const wxWindowID m_idClear = wxNewId();
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
