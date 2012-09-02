/*
 *  This file is part of Poedit (http://www.poedit.net)
 *
 *  Copyright (C) 2000-2012 Vaclav Slavik
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

#include <wx/wxprec.h>

#include <wx/utils.h>
#include <wx/log.h>
#include <wx/process.h>
#include <wx/txtstrm.h>
#include <wx/string.h>
#include <wx/intl.h>
#include <wx/regex.h>

#ifdef __WXMAC__
#if wxCHECK_VERSION(2,9,0)
#include <wx/osx/core/cfstring.h>
#else
#include <wx/mac/corefoundation/cfstring.h>
#endif
#include <CoreFoundation/CFBundle.h>
#endif

#include "gexecute.h"

#ifdef __WXMAC__
static wxString MacGetPathToBinary(const wxString& program)
{
#if !wxCHECK_VERSION(2,9,0)
    #define wxCFStringRef wxMacCFStringHolder
#endif
    wxCFStringRef programstr(program);

    CFBundleRef bundle = CFBundleGetMainBundle();
    CFURLRef urlRel = CFBundleCopyAuxiliaryExecutableURL(bundle, programstr);

    if ( urlRel == NULL )
    {
        wxLogTrace(_T("poedit.execute"), _T("failed to locate '%s'"), program.c_str());
        return program;
    }

    CFURLRef urlAbs = CFURLCopyAbsoluteURL(urlRel);

    wxCFStringRef path(CFURLCopyFileSystemPath(urlAbs, kCFURLPOSIXPathStyle));

    CFRelease(urlRel);
    CFRelease(urlAbs);

    wxString full = path.AsString(wxLocale::GetSystemEncoding());
    wxLogTrace(_T("poedit.execute"), _T("using '%s'"), full.c_str());

    return wxString::Format(_T("\"%s\""), full.c_str());
}
#endif // __WXMAC__


int DoExecuteGettext(const wxString& cmdline_,
                     wxArrayString& gstdout,
                     wxArrayString& gstderr)
{
    wxString cmdline(cmdline_);

#ifdef __WXMAC__
    wxString binary = cmdline.BeforeFirst(_T(' '));
    cmdline = MacGetPathToBinary(binary) + cmdline.Mid(binary.length());
#endif // __WXMAC__

    wxLogTrace(_T("poedit.execute"), _T("executing '%s'"), cmdline.c_str());

#if wxCHECK_VERSION(2,9,0)
    int retcode = wxExecute(cmdline, gstdout, gstderr, wxEXEC_BLOCK);
#else
    int retcode = wxExecute(cmdline, gstdout, gstderr);
#endif

    if ( retcode == -1 )
    {
        wxLogError(_("Cannot execute program: %s"),
                   cmdline.BeforeFirst(_T(' ')).c_str());
    }

    return retcode;
}


bool ExecuteGettext(const wxString& cmdline)
{
    wxArrayString gstdout;
    wxArrayString gstderr;
    int retcode = DoExecuteGettext(cmdline, gstdout, gstderr);

    for ( size_t i = 0; i < gstderr.size(); i++ )
    {
        if ( gstderr[i].empty() )
            continue;
        wxLogError(_T("%s"), gstderr[i].c_str());
    }

    return retcode == 0;
}


bool ExecuteGettextAndParseOutput(const wxString& cmdline, GettextErrors& errors)
{
    wxArrayString gstdout;
    wxArrayString gstderr;
    int retcode = DoExecuteGettext(cmdline, gstdout, gstderr);

    wxRegEx reError(_T(".*\\.po:([0-9]+)(:[0-9]+)?: (.*)"));

    for ( size_t i = 0; i < gstderr.size(); i++ )
    {
        const wxString e = gstderr[i];
        wxLogTrace(_T("poedit.execute"), _T("  stderr: %s"), e.c_str());
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
            wxLogTrace(_T("poedit.execute"),
                       _T("        => parsed error = \"%s\" at %d"),
                       rec.text.c_str(), rec.line);
        }
        else
        {
            wxLogTrace(_T("poedit.execute"), _T("        (unrecognized line!)"));
            // FIXME: handle the rest of output gracefully too
        }
    }

    return retcode == 0;
}
