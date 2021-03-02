/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 2000-2021 Vaclav Slavik
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
#include <wx/thread.h>
#include <wx/txtstrm.h>
#include <wx/string.h>
#include <wx/intl.h>
#include <wx/stdpaths.h>
#include <wx/translation.h>
#include <wx/filename.h>

#include <regex>
#include <boost/throw_exception.hpp>

#include "concurrency.h"
#include "gexecute.h"
#include "errors.h"

namespace
{

#ifdef __WXOSX__
wxString GetGettextPluginPath()
{
    return wxStandardPaths::Get().GetPluginsDir() + "/GettextTools.bundle";
}
#endif // __WXOSX__

#if defined(__WXOSX__) || defined(__WXMSW__)

inline wxString GetAuxBinariesDir()
{
    return GetGettextPackagePath() + "/bin";
}

wxString GetPathToAuxBinary(const wxString& program)
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
                   L"%s doesn’t exist, falling back to %s",
                   path.GetFullPath().c_str(),
                   program.c_str());
        return program;
    }
}
#endif // __WXOSX__ || __WXMSW__


bool ReadOutput(wxInputStream& s, wxArrayString& out)
{
    // the stream could be already at EOF or in wxSTREAM_BROKEN_PIPE state
    s.Reset();

    // Read the input as Latin1, even though we know it's UTF-8. This is terrible,
    // terrible thing to do, but gettext tools may sometimes output invalid UTF-8
    // (e.g. when using non-ASCII, non-UTF8 msgids) and wxTextInputStream logic
    // can't cope well with failing conversions. To make this work, we read the
    // input as Latin1 and later re-encode it back and re-parse as UTF-8.
    wxTextInputStream tis(s, " ", wxConvISO8859_1);

    while (true)
    {
        const wxString line = tis.ReadLine();
        if ( !line.empty() )
        {
            // reconstruct the UTF-8 text if we can
            wxString line2(line.mb_str(wxConvISO8859_1), wxConvUTF8);
            if (line2.empty())
                line2 = line;
            out.push_back(line2);
        }
        if (s.Eof())
            break;
        if ( !s )
            return false;
    }

    return true;
}

std::pair<long, wxArrayString> DoExecuteGettextImpl(const wxString& cmdline_)
{
    wxArrayString gstderr;
    wxExecuteEnv env;
    wxString cmdline(cmdline_);

#if defined(__WXOSX__) || defined(__WXMSW__)
    wxString binary = cmdline.BeforeFirst(_T(' '));
    cmdline = GetPathToAuxBinary(binary) + cmdline.Mid(binary.length());
    wxGetEnvMap(&env.env);
    env.env["OUTPUT_CHARSET"] = "UTF-8";

    wxString lang = wxTranslations::Get()->GetBestTranslation("gettext-tools");
	if ( !lang.empty() )
        env.env["LANG"] = lang;
#endif // __WXOSX__ || __WXMSW__

    wxLogTrace("poedit.execute", "executing: %s", cmdline.c_str());

    wxScopedPtr<wxProcess> process(new wxProcess);
    process->Redirect();

    long retcode = wxExecute(cmdline, wxEXEC_BLOCK | wxEXEC_NODISABLE | wxEXEC_NOEVENTS, process.get(), &env);
    if (retcode != 0)
    {
        wxLogTrace("poedit.execute", "  execution of command failed with exit code %d: %s", (int)retcode, cmdline.c_str());
    }

	wxInputStream *std_err = process->GetErrorStream();
    if ( std_err && !ReadOutput(*std_err, gstderr) )
        retcode = -1;

    if ( retcode == -1 )
    {
        BOOST_THROW_EXCEPTION(Exception(wxString::Format(_("Cannot execute program: %s"), cmdline.c_str())));
    }

    return std::make_pair(retcode, gstderr);
}

std::pair<long, wxArrayString> DoExecuteGettext(const wxString& cmdline)
{
#if wxUSE_GUI
    if (wxThread::IsMain())
    {
        return DoExecuteGettextImpl(cmdline);
    }
    else
    {
        return dispatch::on_main([=]{ return DoExecuteGettextImpl(cmdline); }).get();
    }
#else
    return DoExecuteGettextImpl(cmdline);
#endif
}

void LogUnrecognizedError(const wxString& err)
{
#ifdef __WXOSX__
    // gettext-0.20 started showing setlocale() warnings under what are
    // normal circumstances when running from GUI; filter them out.
    //
    //   Warning: Failed to set locale category LC_NUMERIC to de.
    //   Warning: Failed to set locale category LC_TIME to de.
    //   ...etc...
    if (err.StartsWith("Warning: Failed to set locale category"))
        return;
#endif // __WXOSX__

    wxLogError("%s", err);
}

} // anonymous namespace


bool ExecuteGettext(const wxString& cmdline)
{
    wxArrayString gstderr;
    long retcode;
    std::tie(retcode, gstderr) = DoExecuteGettext(cmdline);

    wxString pending;
    for (auto& ln: gstderr)
    {
        if (ln.empty())
            continue;

        // special handling of multiline errors
        if (ln[0] == ' ' || ln[0] == '\t')
        {
            pending += "\n\t" + ln.Strip(wxString::both);
        }
        else
        {
            if (!pending.empty())
                LogUnrecognizedError(pending);

            pending = ln;
        }
    }

    if (!pending.empty())
        LogUnrecognizedError(pending);

    return retcode == 0;
}


bool ExecuteGettextAndParseOutput(const wxString& cmdline, GettextErrors& errors)
{
    wxArrayString gstderr;
    long retcode;
    std::tie(retcode, gstderr) = DoExecuteGettext(cmdline);

    static const std::wregex RE_ERROR(L".*\\.po:([0-9]+)(:[0-9]+)?: (.*)");

    for (const auto& ewx: gstderr)
    {
        const auto e = ewx.ToStdWstring();
        wxLogTrace("poedit", "  stderr: %s", e.c_str());
        if ( e.empty() )
            continue;

        GettextError rec;

        std::wsmatch match;
        if (std::regex_match(e, match, RE_ERROR))
        {
            rec.line = std::stoi(match.str(1));
            rec.text = match.str(3);
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


wxString QuoteCmdlineArg(const wxString& s)
{
    wxString s2(s);
#ifdef __UNIX__
    s2.Replace("\"", "\\\"");
#endif
    return "\"" + s2 + "\"";
}


#if defined(__WXOSX__) || defined(__WXMSW__)
wxString GetGettextPackagePath()
{
#if defined(__WXOSX__)
    return GetGettextPluginPath() + "/Contents/MacOS";
#elif defined(__WXMSW__)
    return wxStandardPaths::Get().GetDataDir() + wxFILE_SEP_PATH + "GettextTools";
#endif
}
#endif // __WXOSX__ || __WXMSW__

