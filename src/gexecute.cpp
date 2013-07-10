/*
 *  This file is part of Poedit (http://www.poedit.net)
 *
 *  Copyright (C) 2000-2013 Vaclav Slavik
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

#include <wx/utils.h>
#include <wx/log.h>
#include <wx/process.h>
#include <wx/txtstrm.h>
#include <wx/string.h>
#include <wx/intl.h>
#include <wx/regex.h>
#include <wx/stdpaths.h>
#include <wx/filename.h>

#include "gexecute.h"
#include "errors.h"

#if defined(__WXMAC__) || defined(__WXMSW__)
static wxString GetAuxBinariesDir()
{
#ifdef __WXMAC__
    wxString dir = wxStandardPaths::Get().GetPluginsDir();
    return dir + "/GettextTools.bundle/Contents/MacOS";
#else
    return wxStandardPaths::Get().GetExecutablePath();
#endif
}

static wxString GetPathToAuxBinary(const wxString& program)
{
    wxFileName path;
    path.SetPath(GetAuxBinariesDir());
    path.SetName(program);
#ifdef __WXMSW__
    path.SetExt("exe");
#endif
    if ( path.IsFileExecutable() )
    {
        return wxString::Format(_T("\"%s\""), path.GetFullPath().c_str());
    }
    else
    {
        wxLogTrace("poedit.execute",
                   "%s doesn't exist, falling back to %s",
                   path.GetFullPath().c_str(),
                   program.c_str());
        return program;
    }
}
#endif // __WXMAC__ || __WXMSW__


long DoExecuteGettext(const wxString& cmdline_,
                      wxArrayString& gstdout,
                      wxArrayString& gstderr)
{
    wxString cmdline(cmdline_);

#if defined(__WXMAC__) || defined(__WXMSW__)
    wxString binary = cmdline.BeforeFirst(_T(' '));
    cmdline = GetPathToAuxBinary(binary) + cmdline.Mid(binary.length());
#endif

    wxLogTrace("poedit.execute", "executing: %s", cmdline.c_str());

    long retcode = wxExecute(cmdline, gstdout, gstderr, wxEXEC_BLOCK);

    if ( retcode == -1 )
        throw Exception(wxString::Format(_("Cannot execute program: %s"), cmdline.c_str()));

    return retcode;
}


bool ExecuteGettext(const wxString& cmdline)
{
    wxArrayString gstdout;
    wxArrayString gstderr;
    long retcode = DoExecuteGettext(cmdline, gstdout, gstderr);

    for ( size_t i = 0; i < gstderr.size(); i++ )
    {
        if ( gstderr[i].empty() )
            continue;
        wxLogError("%s", gstderr[i].c_str());
    }

    return retcode == 0;
}


bool ExecuteGettextAndParseOutput(const wxString& cmdline, GettextErrors& errors)
{
    wxArrayString gstdout;
    wxArrayString gstderr;
    long retcode = DoExecuteGettext(cmdline, gstdout, gstderr);

    wxRegEx reError(".*\\.po:([0-9]+)(:[0-9]+)?: (.*)");

    for ( size_t i = 0; i < gstderr.size(); i++ )
    {
        const wxString e = gstderr[i];
        wxLogTrace("poedit.execute", "  stderr: %s", e.c_str());
        if ( e.empty() )
            continue;

        GettextError rec;

        if ( reError.Matches(e) )
        {
            long num = -1;
            reError.GetMatch(e, 1).ToLong(&num);
            rec.line = (int)num;
            rec.text = reError.GetMatch(e, 3);
            errors.push_back(rec);
            wxLogTrace("poedit.execute",
                       _T("        => parsed error = \"%s\" at %d"),
                       rec.text.c_str(), rec.line);
        }
        else
        {
            wxLogTrace("poedit.execute", "        (unrecognized line!)");
            // FIXME: handle the rest of output gracefully too
        }
    }

    return retcode == 0;
}
