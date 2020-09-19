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
#include <vector>

wxDEFINE_EVENT(EVT_OPEN_RECENT_FILE, wxCommandEvent);

#ifdef __WXOSX__

class RecentFiles::impl
{
public:
    impl() {}

    void UseMenu(wxMenu*) {}
    void RemoveMenu(wxMenu*) {}

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

        wxMenu *fileMenu;
        wxMenuItem *item = bar->FindItem(XRCID("open_recent"), &fileMenu);
        if (item)
        {
            NSMenu *native = fileMenu->GetHMenu();
            NSMenuItem *nativeItem = [native itemWithTitle:str::to_NS(item->GetItemLabelText())];
            if (nativeItem)
            {
                [nativeItem setSubmenu:m_recentMenu];
                m_recentMenuItem = nativeItem;
            }
        }
    }

private:
    NSMenu *m_recentMenu = nullptr;
    NSMenuItem *m_recentMenuItem = nullptr;
};

#else // !__WXOSX__

class RecentFiles::impl
{
public:
    impl()
    {
        wxConfigBase *cfg = wxConfig::Get();
        cfg->SetPath("/");
        m_history.Load(*cfg);
    }

    void UseMenu(wxMenu *menu)
    {
        m_history.UseMenu(menu);
        m_history.AddFilesToMenu(menu);

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
    }

    void RemoveMenu(wxMenu *menu)
    {
        m_history.RemoveMenu(menu);
    }

    void NoteRecentFile(wxFileName fn)
    {
        fn.Normalize(wxPATH_NORM_DOTS | wxPATH_NORM_ABSOLUTE);
        m_history.AddFileToHistory(fn.GetFullPath());

        wxConfigBase *cfg = wxConfig::Get();
        cfg->SetPath("/");
        m_history.Save(*cfg);
    }

private:
    wxFileHistory m_history;
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

void RecentFiles::UseMenu(wxMenu *menu)
{
    m_impl->UseMenu(menu);
}

void RecentFiles::RemoveMenu(wxMenu *menu)
{
    m_impl->RemoveMenu(menu);
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
