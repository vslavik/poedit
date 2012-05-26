/*
 *  This file is part of Poedit (http://www.poedit.net)
 *
 *  Copyright (C) 2000-2005 Vaclav Slavik
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

#ifdef __WXMAC__
#if wxCHECK_VERSION(2, 9, 0)
#include <wx/osx/core/cfstring.h>
#else
#include <wx/mac/corefoundation/cfstring.h>
#endif
#include <CoreFoundation/CFBundle.h>
#endif

#include "gexecute.h"

#ifdef USE_GETTEXT_VALIDATION
class GettextProcess : public wxProcess
{
    public:
        GettextProcess(GettextProcessData *data, wxEvtHandler *parent) 
            : wxProcess(parent), m_data(data)
        {
            m_data->Running = true;
            m_data->Stderr.Empty();
            m_data->Stdout.Empty();
            Redirect();
        }

        bool HasInput()
        {
            bool hasInput = false;

            wxInputStream* is = GetInputStream();
            if (is && is->CanRead() && !is->Eof()) 
            {
                wxTextInputStream tis(*is);
                m_data->Stdout.Add(tis.ReadLine());
                hasInput = true;
            }

            wxInputStream* es = GetErrorStream();
            if (es && es->CanRead() && !es->Eof()) 
            {
                wxTextInputStream tis(*es);
                m_data->Stderr.Add(tis.ReadLine());
                hasInput = true;
            }

            return hasInput;
        }

        void OnTerminate(int pid, int status)
        {
            while (HasInput()) {}
            m_data->Running = false;
            m_data->ExitCode = status;
            wxProcess::OnTerminate(pid, status);
        }

    private:
        GettextProcessData *m_data;
};
#endif // USE_GETTEXT_VALIDATION


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

bool ExecuteGettext(const wxString& cmdline_)
{
    wxString cmdline(cmdline_);

#ifdef __WXMAC__
    wxString binary = cmdline.BeforeFirst(_T(' '));
    cmdline = MacGetPathToBinary(binary) + cmdline.Mid(binary.length());
#endif // __WXMAC__

    wxLogTrace(_T("poedit.execute"), _T("executing '%s'"), cmdline.c_str());

    wxArrayString gstdout;
    wxArrayString gstderr;

#if wxCHECK_VERSION(2,9,0)
    int retcode = wxExecute(cmdline, gstdout, gstderr, wxEXEC_BLOCK);
#else
    int retcode = wxExecute(cmdline, gstdout, gstderr);
#endif

    if ( retcode == -1 )
    {
        wxLogError(_("Cannot execute program: %s"),
                   cmdline.BeforeFirst(_T(' ')).c_str());
        return false;
    }

    for ( size_t i = 0; i < gstderr.size(); i++ )
    {
        if ( gstderr[i].empty() )
            continue;
        wxLogError(_T("%s"), gstderr[i].c_str());
    }

    return retcode == 0;
}


#ifdef USE_GETTEXT_VALIDATION
wxProcess *ExecuteGettextNonblocking(const wxString& cmdline_,
                                     GettextProcessData *data,
                                     wxEvtHandler *parent)
{
    wxString cmdline(cmdline_);

#ifdef __WXMAC__
    wxString binary = cmdline.BeforeFirst(_T(' '));
    cmdline = MacGetPathToBinary(binary) + cmdline.Mid(binary.length());
#endif // __WXMAC__

    wxLogTrace(_T("poedit.execute"), _T("executing '%s'"), cmdline.c_str());

    GettextProcess *process;

    process = new GettextProcess(data, parent);
    int pid = wxExecute(cmdline, wxEXEC_ASYNC, process);

    if (pid == 0)
    {
        wxLogError(_("Cannot execute program: ") + cmdline.BeforeFirst(_T(' ')));
        return NULL;
    }

    return process;
}
#endif // USE_GETTEXT_VALIDATION
