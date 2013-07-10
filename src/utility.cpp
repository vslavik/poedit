/*
 *  This file is part of Poedit (http://www.poedit.net)
 *
 *  Copyright (C) 2010-2013 Vaclav Slavik
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

#include "utility.h"

#include <stdio.h>
#include <wx/filename.h>
#include <wx/log.h>
#include <wx/config.h>
#include <wx/display.h>

wxString EscapeMarkup(const wxString& str)
{
    wxString s(str);
    s.Replace("&", "&amp;");
    s.Replace("<", "&lt;");
    s.Replace(">", "&gt;");
    return s;
}

// ----------------------------------------------------------------------
// TempDirectory
// ----------------------------------------------------------------------

bool TempDirectory::ms_keepFiles = false;

TempDirectory::TempDirectory() : m_counter(0)
{
#ifdef HAVE_MKDTEMP
    wxString path = wxFileName::GetTempDir();
    path += "/poeditXXXXXX";
    wxCharBuffer buf(path.fn_str());
    if ( mkdtemp(buf.data()) == NULL )
    {
        wxLogError(_("Cannot create temporary directory."));
        return;
    }
    m_dir = wxConvFile.cMB2WX(buf.data());
#else
    for ( ;; )
    {
        wxString name = wxFileName::CreateTempFileName("poedit");
        if ( name.empty() )
        {
            wxLogError(_("Cannot create temporary directory."));
            return;
        }

        wxLogNull null;
        if ( wxRemoveFile(name) && wxMkdir(name, 0700) )
        {
            m_dir = name;
            wxLogTrace("poedit.tmp", "created temp dir %s", name.c_str());
            break;
        }
        // else: try again
    }
#endif
}


TempDirectory::~TempDirectory()
{
    if ( m_dir.empty() )
        return;

    if ( ms_keepFiles )
    {
        wxLogTrace("poedit.tmp", "keeping temp files in %s", m_dir.c_str());
        return;
    }

    for ( wxArrayString::const_iterator i = m_files.begin(); i != m_files.end(); ++i )
    {
        if ( wxFileName::FileExists(*i) )
        {
            wxLogTrace("poedit.tmp", "removing temp file %s", i->c_str());
            wxRemoveFile(*i);
        }
    }

    wxLogTrace("poedit.tmp", "removing temp dir %s", m_dir.c_str());
    wxFileName::Rmdir(m_dir);
}


wxString TempDirectory::CreateFileName(const wxString& suffix)
{
    wxString s = wxString::Format("%s%c%d%s",
                                  m_dir.c_str(), wxFILE_SEP_PATH,
                                  m_counter++,
                                  suffix.c_str());
    m_files.push_back(s);
    wxLogTrace("poedit.tmp", "new temp file %s", s.c_str());
    return s;
}


// ----------------------------------------------------------------------
// Helpers for persisting windows' state
// ----------------------------------------------------------------------

void SaveWindowState(const wxTopLevelWindow *win, int flags)
{
    wxConfigBase *cfg = wxConfig::Get();
    const wxString path = WindowStatePath(win);

    if ( !win->IsIconized() )
    {
        if ( !win->IsMaximized() )
        {
#ifdef __WXMSW__
            if ( flags & WinState_Pos )
            {
                const wxPoint pos = win->GetPosition();
                cfg->Write(path + "x", (long)pos.x);
                cfg->Write(path + "y", (long)pos.y);
            }
#endif // __WXMSW__
            if ( flags &  WinState_Size )
            {
                const wxSize sz = win->GetClientSize();
                cfg->Write(path + "w", (long)sz.x);
                cfg->Write(path + "h", (long)sz.y);
            }
        }

        if ( flags &  WinState_Size )
            cfg->Write(path + "maximized", (long)win->IsMaximized());
    }
}


void RestoreWindowState(wxTopLevelWindow *win, const wxSize& defaultSize, int flags)
{
    wxConfigBase *cfg = wxConfig::Get();
    const wxString path = WindowStatePath(win);

    if ( flags & WinState_Size )
    {
        int width = (int)cfg->Read(path + "w", defaultSize.x);
        int height = (int)cfg->Read(path + "h", defaultSize.y);
        if ( width != -1 || height != -1 )
            win->SetClientSize(width, height);
    }

#ifdef __WXMSW__
    if ( flags & WinState_Pos )
    {
        int posx = (int)cfg->Read(path + "x", -1);
        int posy = (int)cfg->Read(path + "y", -1);
        if ( posx != -1 || posy != -1 )
            win->Move(posx, posy);
    }

    // If the window is completely out of all screens (e.g. because
    // screens configuration changed), move it to primary screen:
    if ( wxDisplay::GetFromWindow(win) == wxNOT_FOUND )
        win->Move(0, 0);
#endif // __WXMSW__

    // If the window is larger than current screen, resize it to fit:
    int display = wxDisplay::GetFromWindow(win);
    wxCHECK_RET( display != wxNOT_FOUND, "window not on screen" );
    wxRect screenRect = wxDisplay(display).GetClientArea();

    wxRect winRect = win->GetRect();
    if ( winRect.GetPosition() == wxDefaultPosition )
        winRect.SetPosition(screenRect.GetPosition()); // not place yet, fake it

    if ( !screenRect.Contains(winRect) )
    {
        winRect.Intersect(screenRect);
        win->SetSize(winRect);
    }

    // Maximize if it should be
    if ( cfg->Read(path + "maximized", long(0)) )
    {
        win->Maximize();
    }
}
