/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 2017-2025 Vaclav Slavik
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

#include "extractor.h"

#include "gexecute.h"

#include <wx/textfile.h>

namespace
{

// This list is synced with EXTENSIONS_* macros in deps/gettext/gettext-tools/src/x-*.h files:
const char * const GETTEXT_EXTENSIONS[] = {
    "appdata.xml",                                        // appdata - ITS

    "awk", "gawk", "twjr",                                // awk

    "c", "h",                                             // C
    "C", "c++", "cc", "cxx", "cpp", "hh", "hxx", "hpp",   // C++
    "m",                                                  // ObjectiveC
    // FIXME: handling of .h files as C++? (req. separate pass)
    // FIXME: .mm files for Objective-C++ (add gettext-tools support first)

    "cs",                                                 // C#

    "desktop",                                            // Desktop

    "el",                                                 // EmacsLisp

    "glade", "glade2", "ui",                              // glade - ITS

    "gschema.xml",                                        // GSettings - ITS

    "java",                                               // Java

    "js",                                                 // JavaScript

    "jl",                                                 // librep

    "lisp",                                               // Lisp

    "lua",                                                // Lua

    "pl", "PL", "pm", "perl", /*"cgi" - too generic,*/    // perl

    "php", "php3", "php4",                                // PHP

    "py",                                                 // Python

    // "rst",                                             // RST
    // NOTE: conflicts with restructured text, dangerous

    "scm",                                                // Scheme

    // "sh", "bash",                                      // Shell
    // NOTE: disabled in Poedit, rarely if ever used

    "st",                                                 // Smalltalk

    "tcl",                                                // Tcl

    "vala",                                               // Vala

    "ycp",                                                // YCP

    nullptr
};


} // anonymous namespace


/// Extractor implementation for standard GNU gettext
class GettextExtractorBase : public Extractor
{
public:
    ExtractionOutput Extract(TempDirectory& tmpdir,
                             const SourceCodeSpec& sourceSpec,
                             const std::vector<wxString>& files) const override
    {
        using subprocess::quote_arg;

        auto basepath = sourceSpec.BasePath;
#ifdef __WXMSW__
        basepath = CliSafeFileName(basepath);
        basepath.Replace("\\", "/");
#endif

        wxTextFile filelist;
        filelist.Create(tmpdir.CreateFileName("gettext_filelist.txt"));
        for (auto fn: files)
        {
#ifdef __WXMSW__
            // Gettext tools can't handle Unicode filenames well (due to using
            // char* arguments), so work around this by using the short names.
            if (!fn.IsAscii())
            {
                fn = CliSafeFileName(fn);
                fn.Replace("\\", "/");
            }
#endif
            filelist.AddLine(fn);
        }
        filelist.Write(wxTextFileType_Unix, wxConvFile);

        auto outfile = tmpdir.CreateFileName("gettext.pot");

        wxString cmdline;
        cmdline.Printf
        (
            "xgettext --force-po -o %s --directory=%s --files-from=%s --from-code=%s",
            quote_arg(outfile),
            quote_arg(basepath),
            quote_arg(filelist.GetName()),
            quote_arg(!sourceSpec.Charset.empty() ? sourceSpec.Charset : "UTF-8")
        );

        auto additional = GetAdditionalFlags();
        if (!additional.empty())
            cmdline += " " + additional;

        for (auto& kw: sourceSpec.Keywords)
        {
            cmdline += wxString::Format(" -k%s", quote_arg(kw));
        }

        wxString extraFlags;
        try
        {
            extraFlags = sourceSpec.XHeaders.at("X-Poedit-Flags-xgettext");
        }
        catch (std::out_of_range&) {}

        if (!extraFlags.Contains("--add-comments"))
            cmdline += " --add-comments=TRANSLATORS:";

        if (!extraFlags.empty())
            cmdline += " " + extraFlags;

        GettextRunner runner;
        auto output = runner.run_command_sync(cmdline);
        auto err = runner.parse_stderr(output);

        if (output.failed())
        {
            err.log_all();
            BOOST_THROW_EXCEPTION(ExtractionException(ExtractionError::Unspecified));
        }

        return {outfile, err};
    }
    
protected:
    virtual wxString GetAdditionalFlags() const = 0;
};


/// Extractor implementation for standard GNU gettext
class GettextExtractor : public GettextExtractorBase
{
public:
    GettextExtractor()
    {
        for (const char * const *e = GETTEXT_EXTENSIONS; *e != nullptr; e++)
            RegisterExtension(*e);

        if (check_gettext_version(0, 24))
        {
            RegisterExtension("rs"); // Rust
        }
    }

    wxString GetId() const override { return "gettext"; }

protected:
    wxString GetAdditionalFlags() const override { return ""; }
};


/// Dedicated extractor for non-standard PHP extensions (*.phtml etc.)
class CustomGettextExtractor : public GettextExtractorBase
{
public:
    CustomGettextExtractor(const wxString& language) : m_language(language) {}
    wxString GetId() const override { return "gettext-" + m_language; }

protected:
    wxString GetAdditionalFlags() const override { return "-L " + m_language; }

    wxString m_language;
};


/// Dedicated extractor for non-standard PHP extensions (*.phtml etc.)
class NonstandardPHPGettextExtractor : public CustomGettextExtractor
{
public:
    NonstandardPHPGettextExtractor() : CustomGettextExtractor("php")
    {
        RegisterExtension("phtml");  // Zend Framework
        RegisterExtension("ctp");    // CakePHP
    }
};



void Extractor::CreateGettextExtractors(Extractor::ExtractorsList& into, const SourceCodeSpec& sources)
{
    into.push_back(std::make_shared<GettextExtractor>());
    into.push_back(std::make_shared<NonstandardPHPGettextExtractor>());

    for (auto& m : sources.TypeMapping)
    {
        if (m.second.starts_with("gettext:"))  // e.g. gettext:php
        {
            auto e = std::make_shared<CustomGettextExtractor>(m.second.AfterFirst(':'));
            if (wxIsWild(m.first))
                e->RegisterWildcard(m.first);
            else
                e->RegisterExtension(m.first);

            e->SetPriority(Priority::CustomExtension);
            into.push_back(e);
        }
    }
}
