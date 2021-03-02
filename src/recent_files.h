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

#ifndef Poedit_recent_files_h
#define Poedit_recent_files_h

#include <wx/dataview.h>
#include <wx/event.h>

#include <memory>
#include <vector>

class WXDLLIMPEXP_FWD_BASE wxFileName;
class WXDLLIMPEXP_FWD_CORE wxMenuBar;
class WXDLLIMPEXP_FWD_CORE wxMenuItem;


/// Event for opening recent files
wxDECLARE_EVENT(EVT_OPEN_RECENT_FILE, wxCommandEvent);


/// Management of recently opened files.
class RecentFiles
{
public:
    /// Return singleton instance of the manager.
    static RecentFiles& Get();

    /// Destroys the singleton, must be called (only) on app shutdown.
    static void CleanUp();

    /// Use this menu to show recent items.
    void UseMenu(wxMenuItem *menu);

    /// Record a file as being recently edited.
    void NoteRecentFile(const wxFileName& fn);

    std::vector<wxFileName> GetRecentFiles();

#ifdef __WXOSX__
    /// Hack to make macOS' hack for Open Recent work correctly; must be called from applicationWillFinishLaunching:
    void MacCreateFakeOpenRecentMenu();
    void MacTransferMenuTo(wxMenuBar *bar);
#endif

private:
    RecentFiles();
    ~RecentFiles();

    class impl;
    std::unique_ptr<impl> m_impl;

    friend class RecentFilesCtrl;
};


/// Control with a list of recently opened files
class RecentFilesCtrl : public wxDataViewListCtrl
{
public:
    RecentFilesCtrl(wxWindow *parent);

private:
    void RefreshContent();
    void OnActivate(wxDataViewEvent& event);

    class MultilineTextRenderer;

    struct data;
    std::unique_ptr<data> m_data;
};



#endif // Poedit_recent_files_h
