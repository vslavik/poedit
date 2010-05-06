/*
 *  This file is part of Poedit (http://www.poedit.net)
 *
 *  Copyright (C) 2010 Vaclav Slavik
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

#ifndef _UTILITY_H_
#define _UTILITY_H_

#include <wx/string.h>
#include <wx/arrstr.h>
#include <wx/toplevel.h>

// ----------------------------------------------------------------------
// TempDirectory
// ----------------------------------------------------------------------

// Helper class for managing temporary directories.
// Cleans the directory when destroyed.
class TempDirectory
{
public:
    // creates randomly-named temp directory with "poedit" name prefix
    TempDirectory();
    ~TempDirectory();

    bool IsOk() const { return !m_dir.empty(); }

    // creates new file name in that directory
    wxString CreateFileName(const wxString& suffix);

    // whether to keep temporary files
    static void KeepFiles(bool keep = true) { ms_keepFiles = keep; }

private:
    int m_counter;
    wxString m_dir;
    wxArrayString m_files;

    static bool ms_keepFiles;
};

// ----------------------------------------------------------------------
// Helpers for persisting windows' state
// ----------------------------------------------------------------------

enum WinStateFlags
{
    WinState_Pos  = 1,
    WinState_Size = 2,
    WinState_All  = WinState_Pos | WinState_Size
};

void SaveWindowState(const wxTopLevelWindow *win, int flags = WinState_All);
void RestoreWindowState(wxTopLevelWindow *win, const wxSize& defaultSize,
                        int flags = WinState_All);

inline wxString WindowStatePath(const wxWindow *win)
{
    return wxString::Format(_T("/windows/%s/"), win->GetName().c_str());
}

#endif // _UTILITY_H_
