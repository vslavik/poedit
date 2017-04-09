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

namespace
{

inline LegacyExtractorSpec LoadExtractorSpec(const wxString& name, wxConfigBase *cfg)
{
    LegacyExtractorSpec info;
    info.Name = name;
    info.Enabled = cfg->ReadBool("Enabled", true);
    info.Extensions = cfg->Read("Extensions", wxEmptyString);
    info.Command = cfg->Read("Command", wxEmptyString);
    info.KeywordItem = cfg->Read("KeywordItem", wxEmptyString);
    info.FileItem = cfg->Read("FileItem", wxEmptyString);
    info.CharsetItem = cfg->Read("CharsetItem", wxEmptyString);
    return info;
}

template<typename F>
inline void DoReadLegacyExtractors(wxConfigBase *cfg, F&& action)
{
    cfg->SetExpandEnvVars(false);

    wxString list = cfg->Read("/Parsers/CustomExtractorsList", "");
    if (list.empty())
        list = cfg->Read("/Parsers/List", "");

    wxString oldpath = cfg->GetPath();
    wxStringTokenizer tkn(list, ";");

    while (tkn.HasMoreTokens())
    {
        wxString name = tkn.GetNextToken();
        wxString key = name; key.Replace("/", "_");
        cfg->SetPath("/Parsers/" + key);
        action(cfg, name);
    }

    cfg->SetPath(oldpath);
}

// FIXME: Do this in subprocess, avoid changing CWD altogether in main process
class CurrentWorkingDirectoryChanger
{
public:
    CurrentWorkingDirectoryChanger(const wxString& path)
    {
        if (!path.empty() && path != ".")
        {
            m_old = wxGetCwd();
            wxSetWorkingDirectory(path);
        }
    }

    ~CurrentWorkingDirectoryChanger()
    {
        if (!m_old.empty())
            wxSetWorkingDirectory(m_old);
    }

private:
    wxString m_old;
};

} // anonymous namespace

void LegacyExtractorsDB::Read(wxConfigBase *cfg)
{
    Data.clear();

    DoReadLegacyExtractors(cfg, [=](wxConfigBase *c, wxString name)
    {
        if (!c->ReadBool("DontUseIn20", false))
            Data.push_back(LoadExtractorSpec(name, c));
    });
}


void LegacyExtractorsDB::Write(wxConfigBase *cfg)
{
#if 0 // asserts on wxGTK, some bug in wx
    if (cfg->HasGroup("/Parsers"))
        cfg->DeleteGroup("/Parsers");
#endif

    cfg->SetExpandEnvVars(false);

    size_t i;
    wxString list;
    if (!Data.empty())
    {
        list << Data[0].Name;
        for (i = 1; i < Data.size(); i++)
            list << ";" << Data[i].Name;
    }
    cfg->Write("/Parsers/CustomExtractorsList", list);

    wxString oldpath = cfg->GetPath();
    wxString key;
    for (const auto& item: Data)
    {
        key = item.Name; key.Replace("/", "_");
        cfg->SetPath("/Parsers/" + key);
        cfg->Write("Enabled", item.Enabled);
        cfg->Write("Extensions", item.Extensions);
        cfg->Write("Command", item.Command);
        cfg->Write("KeywordItem", item.KeywordItem);
        cfg->Write("FileItem", item.FileItem);
        cfg->Write("CharsetItem", item.CharsetItem);
        cfg->SetPath(oldpath);
    }
}


void LegacyExtractorsDB::RemoveObsoleteExtractors(wxConfigBase *cfg)
{
    // only run the migration once
    if (cfg->ReadBool("/Parsers/MigratedTo20", false))
        return;

    // Legacy extractor definitions. Now replaced with GettextExtractor.
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

    DoReadLegacyExtractors(cfg, [=](wxConfigBase *c, wxString name)
    {
        // check if it is a known default extractor; if not, keep it
        for (size_t i = 0; s_gettextLangs[i].name != NULL; i++)
        {
            if (name == s_gettextLangs[i].name)
            {
                // build previously used default extractor definition:
                wxString langflag;
                if ( wxStrcmp(s_gettextLangs[i].name, "C/C++") == 0 )
                    langflag = " --language=C++";
                else
                    langflag = wxString(" --language=") + s_gettextLangs[i].name;

                LegacyExtractorSpec ex;
                ex.Name = s_gettextLangs[i].name;
                ex.Enabled = (bool)s_gettextLangs[i].enableByDefault;
                ex.Extensions = s_gettextLangs[i].exts;
                ex.Command = wxString("xgettext") + langflag + " --add-comments=TRANSLATORS: --force-po -o %o %C %K %F";
                ex.KeywordItem = "-k%k";
                ex.FileItem = "%f";
                ex.CharsetItem = "--from-code=%c";

                // load what is stored in the settings:
                LegacyExtractorSpec loaded = LoadExtractorSpec(name, c);
                loaded.Enabled = ex.Enabled;

                if (loaded != ex)
                {
                    // this bad, but mostly harmless, config was used for ~2 years, so check for it too
                    ex.Command.Replace(" --add-comments=TRANSLATORS: ", " --add-comments=TRANSLATORS: --add-comments=translators: ");
                }

                if (loaded == ex)
                {
                    // mark the extractor as not to be used, but keep it around
                    // to make it possible to downgrade to Poedit 1.8:
                    c->Write("DontUseIn20", true);
                }
                // else: keep customized extractor

                break;
            }
        }
    });

    cfg->Write("/Parsers/MigratedTo20", true);
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
            fn = CliSafeFileName(fn);
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

        CurrentWorkingDirectoryChanger cwd(sourceSpec.BasePath);
        if (!ExecuteGettext(m_spec.BuildCommand(batchfiles, sourceSpec.Keywords, tempfile, sourceSpec.Charset)))
        {
            return "";
        }

        tempfiles.push_back(tempfile);
    }

    return ConcatCatalogs(tmpdir, tempfiles);
}


void Extractor::CreateAllLegacyExtractors(Extractor::ExtractorsList& into)
{
    // Extractors must be created anew to pick up any changes in definitions

    // FIXME: Make this MT-safe
    LegacyExtractorsDB db;
    db.Read(wxConfig::Get());

    for (auto& ex: db.Data)
    {
        if (ex.Enabled)
            into.push_back(std::make_shared<LegacyExtractor>(ex));
    }
}
