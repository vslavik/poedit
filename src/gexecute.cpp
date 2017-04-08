/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 2000-2017 Vaclav Slavik
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
#include <wx/stdpaths.h>
#include <wx/translation.h>
#include <wx/filename.h>

#include <boost/throw_exception.hpp>

#include "gexecute.h"
#include "errors.h"

// GCC's libstdc++ didn't have functional std::regex implementation until 4.9
#if (defined(__GNUC__) && !defined(__clang__) && !wxCHECK_GCC_VERSION(4,9))
    #include <boost/regex.hpp>
    using boost::wregex;
    using boost::wsmatch;
    using boost::regex_match;
#else
    #include <regex>
    using std::wregex;
    using std::wsmatch;
    using std::regex_match;
#endif

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
                   "%s doesn't exist, falling back to %s",
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
    wxTextInputStream tis(s, " ", wxConvUTF8);

    while (true)
    {
        wxString line = tis.ReadLine();
        if ( !line.empty() )
            out.push_back(line);
        if (s.Eof())
            break;
        if ( !s )
            return false;
    }

    return true;
}

long DoExecuteGettext(const wxString& cmdline_, wxArrayString& gstderr)
{
    wxExecuteEnv env;
    wxString cmdline(cmdline_);

#if defined(__WXOSX__) || defined(__WXMSW__)
    wxString binary = cmdline.BeforeFirst(_T(' '));
    cmdline = GetPathToAuxBinary(binary) + cmdline.Mid(binary.length());
    wxGetEnvMap(&env.env);
    env.env["GETTEXTIOENCODING"] = "UTF-8";

    wxString lang = wxTranslations::Get()->GetBestTranslation("gettext-tools");
	if ( !lang.empty() )
        env.env["LANG"] = lang;
#endif // __WXOSX__ || __WXMSW__

#ifdef __WXOSX__
    // Hack alert! On Windows, relocation works, but building with it is too
    // messy/broken on macOS, so just use some custom hacks instead:
    auto sharedir = GetGettextPackagePath() + "/share";
    env.env["POEDIT_LOCALEDIR"] = sharedir + "/locale";
    env.env["GETTEXTDATADIR"] = sharedir + "/gettext";
#endif

    wxLogTrace("poedit.execute", "executing: %s", cmdline.c_str());

    wxScopedPtr<wxProcess> process(new wxProcess);
    process->Redirect();

    long retcode = wxExecute(cmdline, wxEXEC_BLOCK | wxEXEC_NODISABLE | wxEXEC_NOEVENTS, process.get(), &env);

	wxInputStream *std_err = process->GetErrorStream();
    if ( std_err && !ReadOutput(*std_err, gstderr) )
        retcode = -1;

    if ( retcode == -1 )
    {
        BOOST_THROW_EXCEPTION(Exception(wxString::Format(_("Cannot execute program: %s"), cmdline.c_str())));
    }

    return retcode;
}

} // anonymous namespace


bool ExecuteGettext(const wxString& cmdline)
{
    wxArrayString gstderr;
    long retcode = DoExecuteGettext(cmdline, gstderr);

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
                wxLogError("%s", pending);

            pending = ln;
        }
    }

    if (!pending.empty())
        wxLogError("%s", pending);

    return retcode == 0;
}


bool ExecuteGettextAndParseOutput(const wxString& cmdline, GettextErrors& errors)
{
    wxArrayString gstderr;
    long retcode = DoExecuteGettext(cmdline, gstderr);

    static const wregex RE_ERROR(L".*\\.po:([0-9]+)(:[0-9]+)?: (.*)");

    for (const auto& ewx: gstderr)
    {
        const auto e = ewx.ToStdWstring();
        wxLogTrace("poedit", "  stderr: %s", e.c_str());
        if ( e.empty() )
            continue;

        GettextError rec;

        wsmatch match;
        if (regex_match(e, match, RE_ERROR))
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

