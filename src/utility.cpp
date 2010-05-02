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

#include "utility.h"

#include <stdio.h>
#include <wx/filename.h>
#include <wx/log.h>

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
