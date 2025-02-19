/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 2000-2025 Vaclav Slavik
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
#include "subprocess.h"


namespace
{

#define GETTEXT_VERSION_NUM(x, y, z)  ((x*1000*1000) + (y*1000) + (z))

// Determine gettext version, return it in the form of XXXYYYZZZ number for version x.y.z
uint32_t GetGettextVersion()
{
    static uint32_t s_version = 0;
    if (!s_version)
    {
        // set old enough fallback version
        s_version = GETTEXT_VERSION_NUM(0, 18, 0);

        auto p = GettextRunner().run_sync("msgcat", "--version");

        if (p.exit_code == 0 && !p.std_out.empty())
        {
            std::smatch m;
            if (std::regex_search(p.std_out, m, std::regex(R"( (([0-9]+)\.([0-9]+)(\.([0-9]+))?)\s)")))
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

} // anonymous namespace


bool ExecuteGettext(const wxString& cmdline)
{
    auto output = GettextRunner().run_command_sync(cmdline);

    wxString pending;
    for (auto& ln: output.std_err_lines())
    {
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

    return output.exit_code == 0;
}


bool ExecuteGettextAndParseOutput(const wxString& cmdline, GettextErrors& errors)
{
    auto output = GettextRunner().run_command_sync(cmdline);

    static const std::wregex RE_ERROR(L".*\\.po:([0-9]+)(:[0-9]+)?: (.*)");

    for (const auto& ewx: output.std_err_lines())
    {
        const auto e = ewx.ToStdWstring();
        wxLogTrace("poedit.execute", "  stderr: %s", e.c_str());

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

    return output.exit_code == 0;
}


wxString QuoteCmdlineArg(const wxString& s)
{
    if (s.find_first_of(L" \t\\\"'") == std::string::npos)
            return s;  // no quoting needed

    wxString s2(s);
    s2.Replace("\\", "\\\\");
    s2.Replace("\"", "\\\"");
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

wxString GetGettextBinariesPath()
{
    return GetGettextPackagePath() + "/bin";
}

#endif // __WXOSX__ || __WXMSW__


GettextRunner::GettextRunner()
{
#if defined(__WXOSX__) || defined(__WXMSW__)
    set_primary_path(GetGettextBinariesPath());
#endif

    env("OUTPUT_CHARSET", "UTF-8");

    wxString lang = wxTranslations::Get()->GetBestTranslation("gettext-tools");
    if ( lang.starts_with("en@") )
        lang = "en"; // don't want things like en@blockquot
    if ( !lang.empty() )
        env("LANG", lang);
}


void GettextRunner::preprocess_args(subprocess::Arguments& args) const
{
    // gettext 0.22 started converting MO files to UTF-8 by default. Don't do that.
    // See https://git.savannah.gnu.org/gitweb/?p=gettext.git;a=commit;h=5412a4f79929004cb6db15d545e07dc953330e8d
    if (args.args()[0] == L"msgfmt" && GetGettextVersion() >= GETTEXT_VERSION_NUM(0, 22, 0))
    {
        args.insert(1, L"--no-convert");
    }

    Runner::preprocess_args(args);
}
