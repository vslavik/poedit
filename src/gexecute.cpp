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
#include <boost/algorithm/string.hpp>

#include "concurrency.h"
#include "gexecute.h"
#include "errors.h"
#include "str_helpers.h"
#include "subprocess.h"


namespace
{

#define GETTEXT_VERSION_NUM(x, y, z)  ((x*1000*1000) + (y*1000) + (z))

// Determine gettext version, return it in the form of XXXYYYZZZ number for version x.y.z
uint32_t gettext_version()
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


/// Extracts error that are possibly multiline from the output
template<typename Functor>
void process_error_output(const std::wstring& program, const subprocess::Output& output, Functor&& handle)
{
    wxString prefix1, prefix2;
    if (!program.empty())
    {
        prefix1 = wxString::Format("%s: ", program);
        prefix2 = wxString::Format("%s: ", GetGettextBinaryPath(program));
    }

    wxString pending;
    for (auto ln: output.std_err_lines())
    {
        if (!program.empty())
        {
            if (ln.starts_with(prefix1))
                ln = ln.substr(prefix1.length());
            else if (ln.starts_with(prefix2))
                ln = ln.substr(prefix2.length());
        }

        // special handling of multiline errors
        if (ln[0] == ' ' || ln[0] == '\t')
        {
            pending += '\n';
            pending += ln.Strip(wxString::both);
        }
        else
        {
            if (!pending.empty())
                handle(pending);
            pending = ln;
        }
    }

    if (!pending.empty())
        handle(pending);
}

template<typename Functor>
void log_errors_with_filter(const ParsedGettextErrors& errors, Functor&& filter)
{
    for (const auto& e: errors.items)
    {
        if (!filter(e))
            continue;

        wxString prefix;
        if (e.has_location())
            prefix += wxString::Format("%s:%d: ", e.file, e.line);
        if (e.level == ParsedGettextErrors::Warning)
            prefix += _("warning: ");

        wxLogError("%s", e.pretty_print());
    }
}

} // anonymous namespace


wxString ParsedGettextErrors::Item::pretty_print() const
{
    wxString prefix;
    if (has_location())
        prefix = wxString::Format("%s:%d: ", file, line);
    if (level == ParsedGettextErrors::Warning)
        prefix += _("warning: ");
    return prefix + text;
}


void ParsedGettextErrors::log_errors()
{
    log_errors_with_filter(*this, [](const auto& e){ return e.level == Error; });
}


void ParsedGettextErrors::log_all()
{
    log_errors_with_filter(*this, [](const auto&){ return true; });
}


ParsedGettextErrors parse_gettext_stderr(const subprocess::Output& output, const std::wstring& program)
{
    ParsedGettextErrors out;

    struct StdPrefixEater
    {
        StdPrefixEater(const std::wstring& warning, const std::wstring& error)
            : m_prefixWarning(warning), m_prefixError(error)
        {}

        inline bool eat_std_prefixes(std::wstring& err, ParsedGettextErrors::Item& rec)
        {
            if (boost::starts_with(err, m_prefixError))
            {
                err = err.substr(m_prefixError.length());
                // this is the default already, but let's be explicit:
                rec.level = ParsedGettextErrors::Error;
                return true;
            }
            else if (boost::starts_with(err, m_prefixWarning))
            {
                err = err.substr(m_prefixWarning.length());
                rec.level = ParsedGettextErrors::Warning;
                return true;
            }
            return false;
        }

        std::wstring m_prefixWarning, m_prefixError;
    };

    StdPrefixEater eaterEnglish(L"warning: ", L"error: ");
    StdPrefixEater eaterLocalized(str::to_wstring(wxGetTranslation("warning: ", "gettext-tools")),
                                  str::to_wstring(wxGetTranslation("error: ", "gettext-tools")));

    process_error_output
    (
        program,
        output,
        [&](const wxString& err_)
        {
            wxLogTrace("poedit.execute", "  stderr: %s", err_);
            auto err = str::to_wstring(err_);
            ParsedGettextErrors::Item rec;

            // recognizing standard prefixes first simplifies differentiating between then and file:line locations:
            if (!eaterLocalized.eat_std_prefixes(err, rec))
                eaterEnglish.eat_std_prefixes(err, rec);

            static const std::wregex RE_LOCATION(L"^([^\r\n]+):([0-9]+): ", std::regex_constants::ECMAScript | std::regex_constants::optimize);
            std::wsmatch match;
            if (std::regex_search(err, match, RE_LOCATION) && match.position() == 0)
            {
                rec.file = match.str(1);
                rec.line = std::stoi(match.str(2));
                err = err.substr(match.length());
            }

            // have to do again, as the standard format is e.g. "test.c:7: warning: text..."
            if (!eaterLocalized.eat_std_prefixes(err, rec))
                eaterEnglish.eat_std_prefixes(err, rec);

            rec.text = err;

            wxLogTrace("poedit.execute",
                       _T("        => parsed %s = \"%s\" at %s:%d"),
                       rec.level == ParsedGettextErrors::Error ? _T("error") : _T("warning"),
                       rec.text, rec.file, rec.line);

            if (!rec.has_location() && err.find(L'\n') != std::string::npos)
            {
                // Handle this special case:
                //
                // xgettext: warning: msgid '%d foo' is used without plural and with plural.
                //                    test.c:13: Here is the occurrence without plural.
                //                    test.c:12: Here is the occurrence with plural.
                //                    Workaround: If the msgid is a sentence, change the wording of the sentence; otherwise, use contexts for disambiguation.
                //
                // Pre-parsing already put all this into a single string, removed the xgettext: and warning: prefixes,
                // but didn't find any location. Let's try to extract the locations from the text.
                std::vector<std::wstring> lines;
                boost::split(lines, err, [](auto& x){ return x == L'\n'; });
                bool found = false;
                for (auto& line: lines)
                {
                    if (std::regex_search(line, match, RE_LOCATION) && match.position() == 0)
                    {
                        found = true;
                        rec.file = match.str(1);
                        rec.line = std::stoi(match.str(2));
                        // keep the text as-is
                        out.items.push_back(rec);
                    }
                }
                if (found)
                    return;  // don't add the error again below
            }

            out.items.push_back(rec);
        }
    );

    return out;
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
    m_program = args.program();

    // gettext 0.22 started converting MO files to UTF-8 by default. Don't do that.
    // See https://git.savannah.gnu.org/gitweb/?p=gettext.git;a=commit;h=5412a4f79929004cb6db15d545e07dc953330e8d
    if (args.program() == L"msgfmt" && gettext_version() >= GETTEXT_VERSION_NUM(0, 22, 0))
    {
        args.insert(1, L"--no-convert");
    }

    Runner::preprocess_args(args);
}
