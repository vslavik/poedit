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

#include "extractor_legacy.h"

#include "gexecute.h"

#include <wx/filename.h>
#include <wx/wfstream.h>
#include <wx/config.h>
#include <wx/tokenzr.h>


void LegacyExtractorsDB::Read(wxConfigBase *cfg)
{
    Data.clear();

    cfg->SetExpandEnvVars(false);

    LegacyExtractorSpec info;
    wxString key, oldpath = cfg->GetPath();
    wxStringTokenizer tkn(cfg->Read("Parsers/List", wxEmptyString), ";");

    while (tkn.HasMoreTokens())
    {
        info.Name = tkn.GetNextToken();
        key = info.Name; key.Replace("/", "_");
        cfg->SetPath("Parsers/" + key);
        info.Enabled = cfg->ReadBool("Enabled", true);
        info.Extensions = cfg->Read("Extensions", wxEmptyString);
        info.Command = cfg->Read("Command", wxEmptyString);
        info.KeywordItem = cfg->Read("KeywordItem", wxEmptyString);
        info.FileItem = cfg->Read("FileItem", wxEmptyString);
        info.CharsetItem = cfg->Read("CharsetItem", wxEmptyString);
        Data.push_back(info);
        cfg->SetPath(oldpath);
    }
}


void LegacyExtractorsDB::Write(wxConfigBase *cfg)
{
#if 0 // asserts on wxGTK, some bug in wx
    if (cfg->HasGroup("Parsers"))
        cfg->DeleteGroup("Parsers");
#endif

    cfg->SetExpandEnvVars(false);

    if (Data.empty())
        return;

    size_t i;
    wxString list;
    list << Data[0].Name;
    for (i = 1; i < Data.size(); i++)
        list << ";" << Data[i].Name;
    cfg->Write("Parsers/List", list);

    wxString oldpath = cfg->GetPath();
    wxString key;
    for (const auto& item: Data)
    {
        key = item.Name; key.Replace("/", "_");
        cfg->SetPath("Parsers/" + key);
        cfg->Write("Enabled", item.Enabled);
        cfg->Write("Extensions", item.Extensions);
        cfg->Write("Command", item.Command);
        cfg->Write("KeywordItem", item.KeywordItem);
        cfg->Write("FileItem", item.FileItem);
        cfg->Write("CharsetItem", item.CharsetItem);
        cfg->SetPath(oldpath);
    }
}


int LegacyExtractorsDB::FindExtractor(const wxString& name) const
{
    for (size_t i = 0; i < Data.size(); i++)
    {
        if (Data[i].Name == name)
            return int(i);
    }
    return -1;
}


void LegacyExtractorsDB::SetupDefaultExtractors(wxConfigBase *cfg)
{
    LegacyExtractorsDB db;
    bool changed = false;
    wxString defaultsVersion = cfg->Read("Parsers/DefaultsVersion",
                                         "1.2.x");
    db.Read(cfg);

    // Add extractors for languages supported by gettext itself (but only if the
    // user didn't already add language with this name himself):
    static struct
    {
        char enableByDefault;
        const char *name;
        const char *exts;
    } s_gettextLangs[] = {
        { 1, "C/C++",      "*.c;*.cpp;*.cc;*.C;*.c++;*.cxx;*.h;*.hpp;*.hxx;*.hh" },
        { 1, "C#",         "*.cs" },
        { 1, "EmacsLisp",  "*.el" },
        { 1, "GSettings",  "*.gschema.xml" },
        { 1, "Glade",      "*.glade;*.glade2;*.ui" },
        { 1, "AppData",    "*.appdata.xml" },
        { 1, "Java",       "*.java" },
        { 1, "JavaScript", "*.js" },
        { 1, "Lisp",       "*.lisp" },
        { 1, "Lua",        "*.lua" },
        { 1, "ObjectiveC", "*.m" },
        { 1, "PHP",        "*.php;*.php3;*.php4;*.phtml" },
        { 1, "Perl",       "*.pl;*.PL;*.pm;*.perl" },
        { 1, "Python",     "*.py" },
        { 0, "RST",        "*.rst" },
        { 1, "Scheme",     "*.scm" },
        { 0, "Shell",      "*.sh;*.bash" },
        { 1, "Smalltalk",  "*.st" },
        { 1, "TCL",        "*.tcl" },
        { 1, "Vala",       "*.vala" },
        { 1, "YCP",        "*.ycp" },
        { 1, "awk",        "*.awk" },
        { 1, "librep",     "*.jl" },
        { 0, NULL, NULL }
    };

    for (size_t i = 0; s_gettextLangs[i].name != NULL; i++)
    {
        // if this lang is already registered, don't overwrite it:
        if (db.FindExtractor(s_gettextLangs[i].name) != -1)
            continue;

        wxString langflag;
        if ( wxStrcmp(s_gettextLangs[i].name, "C/C++") == 0 )
            langflag = " --language=C++";
        else
            langflag = wxString(" --language=") + s_gettextLangs[i].name;

        // otherwise add new extractor:
        LegacyExtractorSpec ex;
        ex.Name = s_gettextLangs[i].name;
        ex.Enabled = (bool)s_gettextLangs[i].enableByDefault;
        ex.Extensions = s_gettextLangs[i].exts;
        ex.Command = wxString("xgettext") + langflag + " --add-comments=TRANSLATORS: --force-po -o %o %C %K %F";
        ex.KeywordItem = "-k%k";
        ex.FileItem = "%f";
        ex.CharsetItem = "--from-code=%c";
        db.Data.push_back(ex);
        changed = true;
    }

    // If upgrading Poedit to 1.2.4, add dxgettext extractor for Delphi:
#ifdef __WINDOWS__
    if (defaultsVersion == "1.2.x")
    {
        LegacyExtractorSpec p;
        p.Name = "Delphi (dxgettext)";
        p.Extensions = "*.pas;*.dpr;*.xfm;*.dfm";
        p.Command = "dxgettext --so %o %F";
        p.KeywordItem = wxEmptyString;
        p.FileItem = "%f";
        db.Data.push_back(p);
        changed = true;
    }
#endif

    // If upgrading Poedit to 1.2.5, update C++ extractor to handle --from-code:
    if (defaultsVersion == "1.2.x" || defaultsVersion == "1.2.4")
    {
        int cpp = db.FindExtractor("C/C++");
        if (cpp != -1)
        {
            if (db.Data[cpp].Command == "xgettext --force-po -o %o %K %F")
            {
                db.Data[cpp].Command = "xgettext --force-po -o %o %C %K %F";
                db.Data[cpp].CharsetItem = "--from-code=%c";
                changed = true;
            }
        }
    }

    // Poedit 1.5.6 had a breakage, it add --add-comments without space in front of it.
    // Repair this automatically:
    for (size_t i = 0; i < db.Data.size(); i++)
    {
        wxString& cmd = db.Data[i].Command;
        if (cmd.Contains("--add-comments=") && !cmd.Contains(" --add-comments="))
        {
            cmd.Replace("--add-comments=", " --add-comments=");
            changed = true;
        }
    }

    if (changed)
    {
        db.Write(cfg);
        cfg->Write("Parsers/DefaultsVersion", "2.0");
    }
}


wxString LegacyExtractorSpec::BuildCommand(const std::vector<wxString>& files,
                                           const wxArrayString& keywords,
                                           const wxString& output,
                                           const wxString& charset) const
{
    wxString cmdline, kline, fline;

    cmdline = Command;
    cmdline.Replace("%o", QuoteCmdlineArg(output));

    for (auto&kw: keywords)
    {
        wxString dummy = KeywordItem;
        dummy.Replace("%k", kw);
        kline << " " << dummy;
    }

    for (auto fn: files)
    {
#ifdef __WXMSW__
        // Gettext tools can't handle Unicode filenames well (due to using
        // char* arguments), so work around this by using the short names.
        if (!fn.IsAscii())
        {
            fn = wxFileName(fn).GetShortPath();
            fn.Replace("\\", "/");
        }
#endif

        wxString dummy = FileItem;
        dummy.Replace("%f", QuoteCmdlineArg(fn));
        fline << " " << dummy;
    }

    wxString charsetline;
    if (!charset.empty())
    {
        charsetline = CharsetItem;
        charsetline.Replace("%c", charset);
    }

    cmdline.Replace("%C", charsetline);
    cmdline.Replace("%K", kline);
    cmdline.Replace("%F", fline);

    return cmdline;
}



LegacyExtractor::LegacyExtractor(const LegacyExtractorSpec& spec)
    : m_spec(spec)
{
    m_id = "legacy_";
    for (auto c: spec.Name)
    {
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9'))
            m_id << c;
        else
            m_id << '_';
    }

    wxStringTokenizer tkn(spec.Extensions, ";, \t", wxTOKEN_STRTOK);
    while (tkn.HasMoreTokens())
    {
        RegisterWildcard(tkn.GetNextToken());
    }
}


wxString LegacyExtractor::Extract(TempDirectory& tmpdir,
                                  const SourceCodeSpec& sourceSpec,
                                  const std::vector<wxString>& files) const
{
    // cmdline's length is limited by OS/shell, this is maximal number
    // of files we'll pass to the parser at one run:
    const int BATCH_SIZE = 16;

    std::vector<wxString> batchfiles, tempfiles;
    size_t i, last = 0;

    while (last < files.size())
    {
        batchfiles.clear();
        for (i = last; i < last + BATCH_SIZE && i < files.size(); i++)
            batchfiles.push_back(files[i]);
        last = i;

        wxString tempfile = tmpdir.CreateFileName(GetId() + "_extracted.pot");
        if (!ExecuteGettext(m_spec.BuildCommand(batchfiles, sourceSpec.Keywords, tempfile, sourceSpec.Charset)))
        {
            return "";
        }

        tempfiles.push_back(tempfile);
    }

    return ConcatCatalogs(tmpdir, tempfiles);
}


Extractor::ExtractorsList Extractor::CreateAllLegacyExtractors()
{
    // Extractors must be created anew to pick up any changes in defitinions

    ExtractorsList all;

    // FIXME: Make this MT-safe
    LegacyExtractorsDB db;
    db.Read(wxConfig::Get());

    for (auto& ex: db.Data)
    {
        if (ex.Enabled)
            all.push_back(std::make_shared<LegacyExtractor>(ex));
    }

    return all;
}
