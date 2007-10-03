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
 *  $Id$
 *
 *  Gettext execution code
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
#include <wx/mac/corefoundation/cfstring.h>
#include <CoreFoundation/CFBundle.h>
#endif

#include "gexecute.h"

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


// we have to do this because otherwise xgettext might
// speak in native language, not English, and we cannot parse
// it correctly (not yet)
class TempLocaleSwitcher
{
    public:
        TempLocaleSwitcher(const wxString& locale)
        {
            wxGetEnv(_T("LC_ALL"), &m_all);
            wxGetEnv(_T("LC_MESSAGES"), &m_messages);
            wxGetEnv(_T("LANG"), &m_lang);
            wxGetEnv(_T("LANGUAGE"), &m_language);

            wxSetEnv(_T("LC_ALL"), locale);
            wxSetEnv(_T("LC_MESSAGES"), locale);
            wxSetEnv(_T("LANG"), locale);
            wxSetEnv(_T("LANGUAGE"), locale);
        }

        ~TempLocaleSwitcher()
        {
            wxSetEnv(_T("LC_ALL"), m_all);
            wxSetEnv(_T("LC_MESSAGES"), m_messages);
            wxSetEnv(_T("LANG"), m_lang);
            wxSetEnv(_T("LANGUAGE"), m_language);
        }

    private:
        wxString m_all, m_messages, m_lang, m_language;
};

#ifdef __WXMAC__
static wxString MacGetPathToBinary(const wxString& program)
{
    wxMacUniCharBuffer programbuf(program);
    wxMacCFStringHolder programstr(
        CFStringCreateWithCharacters(NULL,
                                     programbuf.GetBuffer(),
                                     programbuf.GetChars()));

    CFBundleRef bundle = CFBundleGetMainBundle();
    CFURLRef urlRel = CFBundleCopyAuxiliaryExecutableURL(bundle, programstr);

    if ( urlRel == NULL )
        return program;

    CFURLRef urlAbs = CFURLCopyAbsoluteURL(urlRel);

    wxMacCFStringHolder path(
            CFURLCopyFileSystemPath(urlAbs, kCFURLPOSIXPathStyle));

    CFRelease(urlRel);
    CFRelease(urlAbs);

    return path.AsString(wxLocale::GetSystemEncoding());
}
#endif // __WXMAC__

bool ExecuteGettext(const wxString& cmdline_, wxString *stderrOutput)
{
    wxString cmdline(cmdline_);

#ifdef __WXMAC__
    wxString binary = cmdline.BeforeFirst(_T(' '));
    cmdline = MacGetPathToBinary(binary) + cmdline.Mid(binary.length());
#endif // __WXMAC__

    wxLogTrace(_T("poedit.execute"), _T("executing '%s'"), cmdline.c_str());

    TempLocaleSwitcher localeSwitcher(_T("C"));

    size_t i;
    GettextProcessData pdata;
    GettextProcess *process;

    process = new GettextProcess(&pdata, NULL);
    int pid = wxExecute(cmdline, false, process);

    if (pid == 0)
    {
        wxLogError(_("Cannot execute program: ") + cmdline.BeforeFirst(_T(' ')));
        return false;
    }

    while (pdata.Running)
    {
        process->HasInput();
        wxMilliSleep(50);
        wxYield();
    }

    bool isMsgmerge = (cmdline.BeforeFirst(_T(' ')) == _T("msgmerge"));
    wxString dummy;

    for (i = 0; i < pdata.Stderr.GetCount(); i++) 
    {
        if (pdata.Stderr[i].empty()) continue;
        if (isMsgmerge)
        {
            dummy = pdata.Stderr[i];
            dummy.Replace(_T("."), wxEmptyString);
            if (dummy.empty() || dummy == _T(" done")) continue;
            //msgmerge outputs *progress* to stderr, damn it!
        }
        if (stderrOutput)
            *stderrOutput += pdata.Stderr[i] + _T("\n");
        else
            wxLogError(_T("%s"), pdata.Stderr[i].c_str());
    }

    return pdata.ExitCode == 0;
}


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

    TempLocaleSwitcher localeSwitcher(_T("C"));

    GettextProcess *process;

    process = new GettextProcess(data, parent);
    int pid = wxExecute(cmdline, false, process);

    if (pid == 0)
    {
        wxLogError(_("Cannot execute program: ") + cmdline.BeforeFirst(_T(' ')));
        return NULL;
    }

    return process;
}
