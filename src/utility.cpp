/*
 *  This file is part of Poedit (http://www.poedit.net)
 *
 *  Copyright (C) 2010-2012 Vaclav Slavik
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
    s.Replace(_T("&"), _T("&amp;"));
    s.Replace(_T("<"), _T("&lt;"));
    s.Replace(_T(">"), _T("&gt;"));
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
    path += _T("/poeditXXXXXX");
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
        wxString name = wxFileName::CreateTempFileName(_T("poedit"));
        if ( name.empty() )
        {
            wxLogError(_("Cannot create temporary directory."));
            return;
        }

        wxLogNull null;
        if ( wxRemoveFile(name) && wxMkdir(name, 0700) )
        {
            m_dir = name;
            wxLogTrace(_T("poedit.tmp"), _T("created temp dir %s"), name.c_str());
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
        wxLogTrace(_T("poedit.tmp"), _T("keeping temp files in %s"), m_dir.c_str());
        return;
    }

    for ( wxArrayString::const_iterator i = m_files.begin(); i != m_files.end(); ++i )
    {
        if ( wxFileName::FileExists(*i) )
        {
            wxLogTrace(_T("poedit.tmp"), _T("removing temp file %s"), i->c_str());
            wxRemoveFile(*i);
        }
    }

    wxLogTrace(_T("poedit.tmp"), _T("removing temp dir %s"), m_dir.c_str());
    wxFileName::Rmdir(m_dir);
}


wxString TempDirectory::CreateFileName(const wxString& suffix)
{
    wxString s = wxString::Format(_T("%s%c%d%s"),
                                  m_dir.c_str(), wxFILE_SEP_PATH,
                                  m_counter++,
                                  suffix.c_str());
    m_files.push_back(s);
    wxLogTrace(_T("poedit.tmp"), _T("new temp file %s"), s.c_str());
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
#ifndef __WXGTK__
            if ( flags & WinState_Pos )
            {
                const wxPoint pos = win->GetPosition();
                cfg->Write(path + _T("x"), (long)pos.x);
                cfg->Write(path + _T("y"), (long)pos.y);
            }
#endif // !__WXGTK__
            if ( flags &  WinState_Size )
            {
                const wxSize sz = win->GetClientSize();
                cfg->Write(path + _T("w"), (long)sz.x);
                cfg->Write(path + _T("h"), (long)sz.y);
            }
        }

        if ( flags &  WinState_Size )
            cfg->Write(path + _T("maximized"), (long)win->IsMaximized());
    }
}


void RestoreWindowState(wxTopLevelWindow *win, const wxSize& defaultSize, int flags)
{
    wxConfigBase *cfg = wxConfig::Get();
    const wxString path = WindowStatePath(win);

    if ( flags & WinState_Size )
    {
        int width = cfg->Read(path + _T("w"), defaultSize.x);
        int height = cfg->Read(path + _T("h"), defaultSize.y);
        if ( width != -1 || height != -1 )
            win->SetClientSize(width, height);
    }

#ifndef __WXGTK__
    if ( flags & WinState_Pos )
    {
        int posx = cfg->Read(path + _T("x"), -1);
        int posy = cfg->Read(path + _T("y"), -1);
        if ( posx != -1 || posy != -1 )
            win->Move(posx, posy);
    }

    // If the window is completely out of all screens (e.g. because
    // screens configuration changed), move it to primary screen:
    if ( wxDisplay::GetFromWindow(win) == wxNOT_FOUND )
        win->Move(0, 0);
#endif // !__WXGTK__

    // If the window is larger than current screen, resize it to fit:
    int display = wxDisplay::GetFromWindow(win);
    wxCHECK_RET( display != wxNOT_FOUND, _T("window not on screen") );
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
    if ( cfg->Read(path + _T("maximized"), long(0)) )
    {
        win->Maximize();
    }
}
