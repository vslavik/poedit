/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 2000-2024 Vaclav Slavik
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
#include <wx/scopedptr.h>
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
#include "str_helpers.h"

namespace
{

struct CommandInvocation
{
    CommandInvocation(const wxString& command, const wxString& arguments) : exe(command), args(arguments)
    {
    }

    CommandInvocation(const wxString& cmdline)
    {
        exe = cmdline.BeforeFirst(_T(' '));
        args = cmdline.Mid(exe.length() + 1);
    }

    wxString cmdline() const
    {
#if defined(__WXOSX__) || defined(__WXMSW__)
        const auto fullexe = "\"" + GetGettextBinaryPath(exe) + "\"";
#else
        const auto& fullexe(exe);
#endif
        return args.empty() ? fullexe : fullexe + " " + args;
    }

    wxString exe;
    wxString args;
};


struct ProcessOutput
{
    long return_code = -1;
    std::vector<wxString> std_out;
    std::vector<wxString> std_err;
};


#ifdef MACOS_BUILD_WITHOUT_APPKIT

ProcessOutput ExecuteCommandAndCaptureOutput(const CommandInvocation&, const wxExecuteEnv*)
{
    // wxExecute() uses NSWorkspace, which is unavailable in extensions; it's the only AppKit
    // dependency in wxBase and can be avoided
    wxFAIL_MSG("attempt to execute commands in non-exec binaries: %s");
    return {666};
}

#else

bool ReadOutput(wxInputStream& s, std::vector<wxString>& out)
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


ProcessOutput ExecuteCommandAndCaptureOutput(const CommandInvocation& cmd, const wxExecuteEnv *env)
{
    ProcessOutput pout;
    auto cmdline = cmd.cmdline();

    wxLogTrace("poedit.execute", "executing: %s", cmdline.c_str());

    wxScopedPtr<wxProcess> process(new wxProcess);
    process->Redirect();

    pout.return_code = wxExecute(cmdline, wxEXEC_BLOCK | wxEXEC_NODISABLE | wxEXEC_NOEVENTS, process.get(), env);
    if (pout.return_code != 0)
    {
        wxLogTrace("poedit.execute", "  execution of command failed with exit code %d: %s", (int)pout.return_code, cmdline.c_str());
    }

    wxInputStream *std_out = process->GetInputStream();
    if ( std_out && !ReadOutput(*std_out, pout.std_out) )
        pout.return_code = -1;

    wxInputStream *std_err = process->GetErrorStream();
    if ( std_err && !ReadOutput(*std_err, pout.std_err) )
        pout.return_code = -1;

    if ( pout.return_code == -1 )
    {
        BOOST_THROW_EXCEPTION(Exception(wxString::Format(_("Cannot execute program: %s"), cmdline.c_str())));
    }

    return pout;
}

#endif


#define GETTEXT_VERSION_NUM(x, y, z)  ((x*1000*1000) + (y*1000) + (z))

// Determine gettext version, return it in the form of XXXYYYZZZ number for version x.y.z
uint32_t GetGettextVersion()
{
    static uint32_t s_version = 0;
    if (!s_version)
    {
        // set old enough fallback version
        s_version = GETTEXT_VERSION_NUM(0, 18, 0);

        // then determine actual version
        CommandInvocation msgfmt("msgfmt", "--version");
        auto p = ExecuteCommandAndCaptureOutput(msgfmt, nullptr);
        if (p.return_code == 0 && p.std_out.size() > 1)
        {
            auto line = str::to_utf8(p.std_out[0]);
            std::smatch m;
            if (std::regex_match(line, m, std::regex(R"(.* (([0-9]+)\.([0-9]+)(\.([0-9]+))?)$)")))
            {
                const int x = std::stoi(m.str(2));
                const int y = std::stoi(m.str(3));
                const int z = m[5].matched ? std::stoi(m.str(5)) : 0;
                s_version = GETTEXT_VERSION_NUM(x, y, z);
            }
        }
    }
    return s_version;
}


ProcessOutput DoExecuteGettextImpl(CommandInvocation cmd)
{
    // gettext 0.22 started converting MO files to UTF-8 by default. Don't do that.
    // See https://git.savannah.gnu.org/gitweb/?p=gettext.git;a=commit;h=5412a4f79929004cb6db15d545e07dc953330e8d
    if (cmd.exe == "msgfmt" && GetGettextVersion() >= GETTEXT_VERSION_NUM(0, 22, 0))
    {
        cmd.args = "--no-convert " + cmd.args;
    }

#if defined(__WXOSX__) || defined(__WXMSW__)
    wxExecuteEnv env;
    wxGetEnvMap(&env.env);
    env.env["OUTPUT_CHARSET"] = "UTF-8";

    wxString lang = wxTranslations::Get()->GetBestTranslation("gettext-tools");
    if ( lang.starts_with("en@") )
        lang = "en"; // don't want things like en@blockquot
	if ( !lang.empty() )
        env.env["LANG"] = lang;
    return ExecuteCommandAndCaptureOutput(cmd, &env);
#else // Unix
    return ExecuteCommandAndCaptureOutput(cmd, nullptr);
#endif
}


ProcessOutput DoExecuteGettext(const wxString& cmdline)
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
    if (err.starts_with("Warning: Failed to set locale category"))
        return;
#endif // __WXOSX__

    wxLogError("%s", err);
}

} // anonymous namespace


bool ExecuteGettext(const wxString& cmdline)
{
    auto output = DoExecuteGettext(cmdline);

    wxString pending;
    for (auto& ln: output.std_err)
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

    return output.return_code == 0;
}


bool ExecuteGettextAndParseOutput(const wxString& cmdline, GettextErrors& errors)
{
    auto output = DoExecuteGettext(cmdline);

    static const std::wregex RE_ERROR(L".*\\.po:([0-9]+)(:[0-9]+)?: (.*)");

    for (const auto& ewx: output.std_err)
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

    return output.return_code == 0;
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
    auto plugin = wxStandardPaths::Get().GetPluginsDir() + "/GettextTools.bundle";
    return plugin + "/Contents/MacOS";
#elif defined(__WXMSW__)
    return wxStandardPaths::Get().GetDataDir() + wxFILE_SEP_PATH + "GettextTools";
#endif
}

wxString GetGettextBinaryPath(const wxString& program)
{
    wxFileName path;
    path.SetPath(GetGettextPackagePath() + "/bin");
    path.SetName(program);
#ifdef __WXMSW__
    path.SetExt("exe");
#endif
    if ( path.IsFileExecutable() )
    {
        return path.GetFullPath();
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
