/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 1999-2018 Vaclav Slavik
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

#include "catalog.h"

#include "catalog_po.h"
#include "catalog_xliff.h"

#include "configuration.h"
#include "errors.h"
#include "extractors/extractor.h"
#include "gexecute.h"
#include "qa_checks.h"
#include "str_helpers.h"
#include "utility.h"
#include "version.h"
#include "language.h"

#include <stdio.h>
#include <wx/utils.h>
#include <wx/tokenzr.h>
#include <wx/log.h>
#include <wx/intl.h>
#include <wx/datetime.h>
#include <wx/config.h>
#include <wx/textfile.h>
#include <wx/stdpaths.h>
#include <wx/strconv.h>
#include <wx/memtext.h>
#include <wx/filename.h>

#include <set>
#include <algorithm>


// ----------------------------------------------------------------------
// Textfile processing utilities:
// ----------------------------------------------------------------------

namespace
{

// Fixup some common issues with filepaths in PO files, due to old Poedit versions,
// user misunderstanding or Poedit bugs:
wxString FixBrokenSearchPathValue(wxString p)
{
    if (p.empty())
        return p;
    // no DOS paths please:
    p.Replace("\\", "/");
    if (p.Last() == '/')
        p.RemoveLast();
    return p;
}

} // anonymous namespace


// ----------------------------------------------------------------------
// Catalog::HeaderData
// ----------------------------------------------------------------------

void Catalog::HeaderData::FromString(const wxString& str)
{
    wxStringTokenizer tkn(str, "\n");
    wxString ln;

    m_entries.clear();

    while (tkn.HasMoreTokens())
    {
        ln = tkn.GetNextToken();
        size_t pos = ln.find(_T(':'));
        if (pos == wxString::npos)
        {
            wxLogError(_(L"Malformed header: “%s”"), ln.c_str());
        }
        else
        {
            Entry en;
            en.Key = wxString(ln.substr(0, pos)).Strip(wxString::both);
            en.Value = wxString(ln.substr(pos + 1)).Strip(wxString::both);

            m_entries.push_back(en);
            wxLogTrace("poedit.header",
                       "%s='%s'", en.Key.c_str(), en.Value.c_str());
        }
    }

    ParseDict();
}

wxString Catalog::HeaderData::ToString(const wxString& line_delim)
{
    UpdateDict();

    wxString hdr;
    for (auto& e: m_entries)
    {
        hdr << EscapeCString(e.Key) << ": " << EscapeCString(e.Value) << "\\n" << line_delim;
    }
    return hdr;
}

void Catalog::HeaderData::UpdateDict()
{
    SetHeader("Project-Id-Version", Project);
    SetHeader("POT-Creation-Date", CreationDate);
    SetHeader("PO-Revision-Date", RevisionDate);

    if (TranslatorEmail.empty())
    {
        if (!Translator.empty() || !HasHeader("Last-Translator"))
            SetHeader("Last-Translator", Translator);
        // else: don't modify the header, leave as-is
    }
    else
    {
        if (Translator.empty())
            SetHeader("Last-Translator", TranslatorEmail);
        else
            SetHeader("Last-Translator", Translator + " <" + TranslatorEmail + ">");
    }

    SetHeader("Language-Team", LanguageTeam);
    SetHeader("MIME-Version", "1.0");
    SetHeader("Content-Type", "text/plain; charset=" + Charset);
    SetHeader("Content-Transfer-Encoding", "8bit");
    SetHeaderNotEmpty("Language", Lang.Code());
    SetHeader("X-Generator", wxString::FromAscii("Poedit " POEDIT_VERSION));

    // Set extended information:

    SetHeaderNotEmpty("X-Poedit-SourceCharset", SourceCodeCharset);

    if (!Keywords.empty())
    {
        wxString kw;
        for (size_t i = 0; i < Keywords.GetCount(); i++)
            kw += Keywords[i] + _T(';');
        kw.RemoveLast();
        SetHeader("X-Poedit-KeywordsList", kw);
    }

    unsigned i;
    bool noBookmarkSet = true;
    wxString bk;
    for (i = 0; i < BOOKMARK_LAST ; i++)
    {
        noBookmarkSet = noBookmarkSet && (Bookmarks[i] == NO_BOOKMARK);
        bk += wxString() << Bookmarks[i] << _T(',');
    }
    bk.RemoveLast();
    if (noBookmarkSet)
        DeleteHeader("X-Poedit-Bookmarks");
    else
        SetHeader("X-Poedit-Bookmarks", bk);

    SetHeaderNotEmpty("X-Poedit-Basepath", BasePath);

    i = 0;
    while (true)
    {
        wxString path;
        path.Printf("X-Poedit-SearchPath-%i", i);
        if (!HasHeader(path))
            break;
        DeleteHeader(path);
        i++;
    }

    i = 0;
    while (true)
    {
        wxString path;
        path.Printf("X-Poedit-SearchPathExcluded-%i", i);
        if (!HasHeader(path))
            break;
        DeleteHeader(path);
        i++;
    }

    for (i = 0; i < SearchPaths.size(); i++)
    {
        wxString path;
        path.Printf("X-Poedit-SearchPath-%i", i);
        SetHeader(path, SearchPaths[i]);
    }

    for (i = 0; i < SearchPathsExcluded.size(); i++)
    {
        wxString path;
        path.Printf("X-Poedit-SearchPathExcluded-%i", i);
        SetHeader(path, SearchPathsExcluded[i]);
    }
}

void Catalog::HeaderData::ParseDict()
{
    wxString dummy;

    Project = GetHeader("Project-Id-Version");
    CreationDate = GetHeader("POT-Creation-Date");
    RevisionDate = GetHeader("PO-Revision-Date");

    dummy = GetHeader("Last-Translator");
    if (!dummy.empty())
    {
        wxStringTokenizer tkn(dummy, "<>");
        if (tkn.CountTokens() != 2)
        {
            Translator = dummy;
            TranslatorEmail = wxEmptyString;
        }
        else
        {
            Translator = tkn.GetNextToken().Strip(wxString::trailing);
            TranslatorEmail = tkn.GetNextToken();
        }
    }

    LanguageTeam = GetHeader("Language-Team");

    wxString ctype = GetHeader("Content-Type");
    int charsetPos = ctype.Find("; charset=");
    if (charsetPos != -1)
    {
        Charset =
            ctype.Mid(charsetPos + strlen("; charset=")).Strip(wxString::both);
    }
    else
    {
        Charset = "ISO-8859-1";
    }

    // Parse language information, with backwards compatibility with X-Poedit-*:
    Lang = Language();
    wxString languageCode = GetHeader("Language");
    if (!languageCode.empty())
    {
        Lang = Language::TryParse(languageCode.ToStdWstring());
    }

    if (!Lang.IsValid())
    {
        // try looking for non-standard Qt extension
        languageCode = GetHeader("X-Language");
        if (!languageCode.empty())
            Lang = Language::TryParse(languageCode.ToStdWstring());
    }

    if (!Lang.IsValid())
    {
        wxString X_Language = GetHeader("X-Poedit-Language");
        wxString X_Country = GetHeader("X-Poedit-Country");
        if ( !X_Language.empty() )
            Lang = Language::FromLegacyNames(X_Language.ToStdString(), X_Country.ToStdString());
    }

    DeleteHeader("X-Poedit-Language");
    DeleteHeader("X-Poedit-Country");

    // Parse extended information:
    SourceCodeCharset = GetHeader("X-Poedit-SourceCharset");
    BasePath = FixBrokenSearchPathValue(GetHeader("X-Poedit-Basepath"));

    Keywords.Clear();
    wxString kwlist = GetHeader("X-Poedit-KeywordsList");
    if (!kwlist.empty())
    {
        wxStringTokenizer tkn(kwlist, ";");
        while (tkn.HasMoreTokens())
            Keywords.Add(tkn.GetNextToken());
    }
    else
    {
        // try backward-compatibility version X-Poedit-Keywords. The difference
        // is the separator used, see
        // http://sourceforge.net/tracker/index.php?func=detail&aid=1206579&group_id=27043&atid=389153

        wxString kw = GetHeader("X-Poedit-Keywords");
        if (!kw.empty())
        {
            wxStringTokenizer tkn(kw, ",");
            while (tkn.HasMoreTokens())
                Keywords.Add(tkn.GetNextToken());

            // and remove it, it's not for newer versions:
            DeleteHeader("X-Poedit-Keywords");
        }
    }

    int i;
    for(i = 0; i < BOOKMARK_LAST; i++)
    {
      Bookmarks[i] = NO_BOOKMARK;
    }
    wxString bk = GetHeader("X-Poedit-Bookmarks");
    if (!bk.empty())
    {
        wxStringTokenizer tkn(bk, ",");
        i=0;
        long int val;
        while (tkn.HasMoreTokens() && i<BOOKMARK_LAST)
        {
            tkn.GetNextToken().ToLong(&val);
            Bookmarks[i] = (int)val;
            i++;
        }
    }

    SearchPaths.clear();
    i = 0;
    while (true)
    {
        wxString path;
        path.Printf("X-Poedit-SearchPath-%i", i);
        if (!HasHeader(path))
            break;
        wxString p = FixBrokenSearchPathValue(GetHeader(path));
        if (!p.empty())
            SearchPaths.push_back(p);
        i++;
    }

    SearchPathsExcluded.clear();
    i = 0;
    while (true)
    {
        wxString path;
        path.Printf("X-Poedit-SearchPathExcluded-%i", i);
        if (!HasHeader(path))
            break;
        wxString p = FixBrokenSearchPathValue(GetHeader(path));
        if (!p.empty())
            SearchPathsExcluded.push_back(p);
        i++;
    }
}

wxString Catalog::HeaderData::GetHeader(const wxString& key) const
{
    const Entry *e = Find(key);
    if (e)
        return e->Value;
    else
        return wxEmptyString;
}

bool Catalog::HeaderData::HasHeader(const wxString& key) const
{
    return Find(key) != NULL;
}

void Catalog::HeaderData::SetHeader(const wxString& key, const wxString& value)
{
    Entry *e = (Entry*) Find(key);
    if (e)
    {
        e->Value = value;
    }
    else
    {
        Entry en;
        en.Key = key;
        en.Value = value;
        m_entries.push_back(en);
    }
}

void Catalog::HeaderData::SetHeaderNotEmpty(const wxString& key,
                                            const wxString& value)
{
    if (value.empty())
        DeleteHeader(key);
    else
        SetHeader(key, value);
}

void Catalog::HeaderData::DeleteHeader(const wxString& key)
{
    if (HasHeader(key))
    {
        Entries enew;

        for (Entries::const_iterator i = m_entries.begin();
                i != m_entries.end(); i++)
        {
            if (i->Key != key)
                enew.push_back(*i);
        }

        m_entries = enew;
    }
}

const Catalog::HeaderData::Entry *
Catalog::HeaderData::Find(const wxString& key) const
{
    size_t size = m_entries.size();
    for (size_t i = 0; i < size; i++)
    {
        if (m_entries[i].Key == key)
            return &(m_entries[i]);
    }
    return NULL;
}


// ----------------------------------------------------------------------
// Catalog class
// ----------------------------------------------------------------------

Catalog::Catalog(Type type)
{
    m_sourceLanguage = Language::English();
    m_fileType = type;

    m_isOk = true;
    m_header.BasePath = wxEmptyString;
    for(int i = BOOKMARK_0; i < BOOKMARK_LAST; i++)
    {
        m_header.Bookmarks[i] = -1;
    }
}


bool Catalog::HasCapability(Catalog::Cap cap) const
{
    switch (cap)
    {
        case Cap::Translations:
        case Cap::LanguageSetting:
        case Cap::UserComments:
            return m_fileType == Type::PO;
    }
    return false; // silence VC++ warning
}


static inline wxString GetCurrentTimeString()
{
    return wxDateTime::Now().Format("%Y-%m-%d %H:%M%z");
}

void Catalog::CreateNewHeader()
{
    HeaderData &dt = Header();

    dt.CreationDate = GetCurrentTimeString();
    dt.RevisionDate = dt.CreationDate;

    dt.Lang = Language();
    if (m_fileType == Type::POT)
        dt.SetHeader("Plural-Forms", "nplurals=INTEGER; plural=EXPRESSION;"); // default invalid value

    dt.Project = wxEmptyString;
    dt.LanguageTeam = wxEmptyString;
    dt.Charset = "UTF-8";
    dt.Translator = wxConfig::Get()->Read("translator_name", wxEmptyString);
    dt.TranslatorEmail = wxConfig::Get()->Read("translator_email", wxEmptyString);
    dt.SourceCodeCharset = wxEmptyString;

    dt.BasePath = ".";

    dt.UpdateDict();
}

void Catalog::CreateNewHeader(const Catalog::HeaderData& pot_header)
{
    HeaderData &dt = Header();
    dt = pot_header;

    // UTF-8 should be used by default no matter what the POT uses
    dt.Charset = "UTF-8";

    // clear the fields that are translation-specific:
    dt.Lang = Language();
    if (dt.LanguageTeam == "LANGUAGE <LL@li.org>")
        dt.LanguageTeam.clear();

    // translator should be pre-filled & not the default "FULL NAME <EMAIL@ADDRESS>"
    dt.DeleteHeader("Last-Translator");
    dt.Translator = wxConfig::Get()->Read("translator_name", wxEmptyString);
    dt.TranslatorEmail = wxConfig::Get()->Read("translator_email", wxEmptyString);

    dt.UpdateDict();
}


CatalogItemPtr Catalog::FindItemByLine(int lineno)
{
    int i = FindItemIndexByLine(lineno);
    return i == -1 ? CatalogItemPtr() : m_items[i];
}

int Catalog::FindItemIndexByLine(int lineno)
{
    int last = -1;

    for (auto& i: m_items)
    {
        if (i->GetLineNumber() > lineno)
            return last;
        last++;
    }

    return last;
}

int Catalog::SetBookmark(int id, Bookmark bookmark)
{
    int result = (bookmark==NO_BOOKMARK)?-1:m_header.Bookmarks[bookmark];

    // unset previous bookmarks, if any
    Bookmark bk = m_items[id]->GetBookmark();
    if (bk != NO_BOOKMARK)
        m_header.Bookmarks[bk] = -1;
    if (result > -1)
        m_items[result]->SetBookmark(NO_BOOKMARK);

    // set new bookmark
    m_items[id]->SetBookmark(bookmark);
    if (bookmark != NO_BOOKMARK)
        m_header.Bookmarks[bookmark] = id;

    // return id of previous item for that bookmark
    return result;
}


namespace
{

wxString MaskForType(Catalog::Type t)
{
    switch (t)
    {
        case Catalog::Type::PO:
            return MaskForType("*.po", _("PO Translation Files"));
        case Catalog::Type::POT:
            return MaskForType("*.pot", _("POT Translation Templates"));
    }
    return ""; // silence stupid warning
}

} // anonymous namespace

wxString Catalog::GetTypesFileMask(std::initializer_list<Type> types)
{
    if (types.size() == 0)
        return "";
    wxString out;
    auto t = types.begin();
    out += MaskForType(*t);
    for (++t; t != types.end(); ++t)
    {
        out += "|";
        out += MaskForType(*t);
    }
    return out;
}

wxString Catalog::GetAllTypesFileMask()
{
    return MaskForType("*.po;*.pot", _("All Translation Files"), /*showExt=*/false) +
           "|" +
           GetTypesFileMask({Type::PO, Type::POT});
}


void Catalog::SetFileName(const wxString& fn)
{
    wxFileName f(fn);
    f.Normalize(wxPATH_NORM_DOTS | wxPATH_NORM_ABSOLUTE);
    m_fileName = f.GetFullPath();
}


namespace
{

enum class SourcesPath
{
    Base,
    Root
};

wxString GetSourcesPath(const wxString& fileName, const Catalog::HeaderData& header, SourcesPath kind)
{
    if (fileName.empty())
        return wxString();

    if (header.BasePath.empty())
        return wxString();

    wxString basepath;
    if (wxIsAbsolutePath(header.BasePath))
    {
        basepath = header.BasePath;
    }
    else
    {
        wxString path = wxPathOnly(fileName);
        if (path.empty())
            path = ".";
        basepath = path + wxFILE_SEP_PATH + header.BasePath + wxFILE_SEP_PATH;
    }

    wxFileName root = wxFileName::DirName(basepath);
    root.Normalize(wxPATH_NORM_DOTS | wxPATH_NORM_ABSOLUTE);

    if (kind == SourcesPath::Root)
    {
        // Deal with misconfigured catalogs where the basepath isn't the root.
        for (auto& p : header.SearchPaths)
        {
            wxString path = (p == ".") ? basepath : basepath + wxFILE_SEP_PATH + p;
            root = CommonDirectory(root, MakeFileName(path));
        }
    }

    return root.GetFullPath();
}

} // anonymous namespace

wxString Catalog::GetSourcesBasePath() const
{
    return GetSourcesPath(m_fileName, m_header, SourcesPath::Base);
}

wxString Catalog::GetSourcesRootPath() const
{
    return GetSourcesPath(m_fileName, m_header, SourcesPath::Root);
}

bool Catalog::HasSourcesConfigured() const
{
    return !m_fileName.empty() &&
           !m_header.BasePath.empty() &&
           !m_header.SearchPaths.empty();
}

bool Catalog::HasSourcesAvailable() const
{
    if (!HasSourcesConfigured())
        return false;

    auto basepath = GetSourcesBasePath();
    if (!wxFileName::DirExists(basepath))
        return false;

    for (auto& p: m_header.SearchPaths)
    {
        auto fullp = wxIsAbsolutePath(p) ? p : basepath + p;
        if (!wxFileName::Exists(fullp))
            return false;
    }

    auto wpfile = m_header.GetHeader("X-Poedit-WPHeader");
    if (!wpfile.empty())
    {
        // The following tests in this function are heuristics, so don't run
        // them in presence of X-Poedit-WPHeader and consider the existence
        // of that file a confirmation of correct setup (even though strictly
        // speaking only its absence proves anything).
        return wxFileName::FileExists(basepath + wpfile);
    }

    if (m_header.SearchPaths.size() == 1)
    {
        // A single path doesn't give us much in terms of detection. About the
        // only thing we can do is to check if it is is a well known directory
        // that is unlikely to be the root.
        auto root = GetSourcesRootPath();
        if (root == wxGetUserHome() ||
            root == wxStandardPaths::Get().GetDocumentsDir() ||
            root.EndsWith(wxString(wxFILE_SEP_PATH) + "Desktop" + wxFILE_SEP_PATH))
        {
            return false;
        }
    }

    return true;
}

std::shared_ptr<SourceCodeSpec> Catalog::GetSourceCodeSpec() const
{
    if (!m_isOk)
        return nullptr;

    auto path = GetSourcesBasePath();
    if (!path.empty())
    {
        if (!wxFileName::DirExists(path))
            return nullptr;
    }

    auto spec = std::make_shared<SourceCodeSpec>();
    spec->BasePath = !path.empty() ? path : ".";
    spec->SearchPaths = m_header.SearchPaths;
    spec->ExcludedPaths = m_header.SearchPathsExcluded;
    spec->Charset = m_header.SourceCodeCharset;
    spec->Keywords = m_header.Keywords;
    for (auto& kv: m_header.GetAllHeaders())
        spec->XHeaders[kv.Key] = kv.Value;

    return spec;
}


static unsigned GetCountFromPluralFormsHeader(const Catalog::HeaderData& header)
{
    if ( header.HasHeader("Plural-Forms") )
    {
        // e.g. "Plural-Forms: nplurals=3; plural=(n%10==1 && n%100!=11 ?
        //       0 : n%10>=2 && n%10<=4 && (n%100<10 || n%100>=20) ? 1 : 2);\n"

        wxString form = header.GetHeader("Plural-Forms");
        form = form.BeforeFirst(_T(';'));
        if (form.BeforeFirst(_T('=')) == "nplurals")
        {
            wxString vals = form.AfterFirst('=');
            if (vals == "INTEGER") // POT default
                return 2;
            long val;
            if (vals.ToLong(&val))
                return (unsigned)val;
        }
    }

    // fallback value for plural forms count should be 2, as in English:
    return 2;
}

unsigned Catalog::GetPluralFormsCount() const
{
    unsigned count = GetCountFromPluralFormsHeader(m_header);

    for (auto& i: m_items)
    {
        count = std::max(count, i->GetPluralFormsCount());
    }

    return count;
}

bool Catalog::HasWrongPluralFormsCount() const
{
    unsigned count = 0;

    for (auto& i: m_items)
    {
        count = std::max(count, i->GetPluralFormsCount());
    }

    if ( count == 0 )
        return false; // nothing translated, so we can't tell

    // if 'count' is less than the count from header, it may simply mean there
    // are untranslated strings
    if ( count > GetCountFromPluralFormsHeader(m_header) )
        return true;

    return false;
}

bool Catalog::HasPluralItems() const
{
    for (auto& i: m_items)
    {
        if ( i->HasPlural() )
            return true;
    }
    return false;
}

void Catalog::SetLanguage(Language lang)
{
    m_header.Lang = lang;
    m_header.SetHeaderNotEmpty("Plural-Forms", lang.DefaultPluralFormsExpr().str());
}

void Catalog::GetStatistics(int *all, int *fuzzy, int *badtokens,
                            int *untranslated, int *unfinished)
{
    if (all) *all = 0;
    if (fuzzy) *fuzzy = 0;
    if (badtokens) *badtokens = 0;
    if (untranslated) *untranslated = 0;
    if (unfinished) *unfinished = 0;

    for (auto& i: m_items)
    {
        bool ok = true;

        if (all)
            (*all)++;

        if (i->IsFuzzy())
        {
            if (fuzzy)
                (*fuzzy)++;
            ok = false;
        }
        if (i->HasError())
        {
            if (badtokens)
                (*badtokens)++;
            ok = false;
        }
        if (!i->IsTranslated())
        {
            if (untranslated)
                (*untranslated)++;
            ok = false;
        }

        if ( !ok && unfinished )
            (*unfinished)++;
    }
}


void CatalogItem::SetFlags(const wxString& flags)
{
    static const wxString flag_fuzzy(wxS(", fuzzy"));

    m_moreFlags = flags;

    if (flags.find(flag_fuzzy) != wxString::npos)
    {
        m_isFuzzy = true;
        m_moreFlags.Replace(flag_fuzzy, wxString());
    }
    else
    {
        m_isFuzzy = false;
    }
}


wxString CatalogItem::GetFlags() const
{
    if (m_isFuzzy)
    {
        static const wxString flag_fuzzy(wxS(", fuzzy"));
        if (m_moreFlags.empty())
            return flag_fuzzy;
        else
            return flag_fuzzy + m_moreFlags;
    }
    else
    {
        return m_moreFlags;
    }
}

wxString CatalogItem::GetFormatFlag() const
{
    auto pos = m_moreFlags.find(wxS("-format"));
    if (pos == wxString::npos)
        return wxString();
    auto space = m_moreFlags.find_last_of(" \t", pos);
    if (space == wxString::npos)
        return m_moreFlags.substr(0, pos);
    else
        return m_moreFlags.substr(space+1, pos-space-1);
}

void CatalogItem::SetFuzzy(bool fuzzy)
{
    if (!fuzzy && m_isFuzzy)
        m_oldMsgid.clear();
    m_isFuzzy = fuzzy;

    UpdateInternalRepresentation();
}

wxString CatalogItem::GetTranslation(unsigned idx) const
{
    if (idx >= GetNumberOfTranslations())
        return wxEmptyString;
    else
        return m_translations[idx];
}

void CatalogItem::SetTranslation(const wxString &t, unsigned idx)
{
    while (idx >= m_translations.GetCount())
        m_translations.Add(wxEmptyString);
    m_translations[idx] = t;

    ClearIssue();

    m_isTranslated = true;
    for (size_t i = 0; i < m_translations.GetCount(); i++)
    {
        if (m_translations[i].empty())
        {
            m_isTranslated = false;
            break;
        }
    }

    UpdateInternalRepresentation();
}

void CatalogItem::SetTranslations(const wxArrayString &t)
{
    m_translations = t;

    ClearIssue();

    m_isTranslated = true;
    for (size_t i = 0; i < m_translations.GetCount(); i++)
    {
        if (m_translations[i].empty())
        {
            m_isTranslated = false;
            break;
        }
    }

    UpdateInternalRepresentation();
}

void CatalogItem::SetTranslationFromSource()
{
    ClearIssue();
    m_isFuzzy = false;
    m_isPreTranslated = false;
    m_isTranslated = true;

    auto iter = m_translations.begin();
    if (*iter != m_string)
    {
        *iter = m_string;
        m_isModified = true;
    }

    if (m_hasPlural)
    {
        ++iter;
        for ( ; iter != m_translations.end(); ++iter )
        {
            if (*iter != m_plural)
            {
                *iter = m_plural;
                m_isModified = true;
            }
        }
    }

    UpdateInternalRepresentation();
}

void CatalogItem::ClearTranslation()
{
    m_isFuzzy = false;
    m_isPreTranslated = false;
    m_isTranslated = false;
    for (auto& t: m_translations)
    {
        if (!t.empty())
            m_isModified = true;
        t.clear();
    }

    UpdateInternalRepresentation();
}

unsigned CatalogItem::GetPluralFormsCount() const
{
    unsigned trans = GetNumberOfTranslations();
    if ( !HasPlural() || !trans )
        return 0;

    return trans - 1;
}

wxArrayString CatalogItem::GetReferences() const
{
    // A line may contain several references, separated by white-space.
    // Each reference is in the form "path_name:line_number"
    // (path_name may contain spaces)

    wxArrayString refs;

    for ( wxArrayString::const_iterator i = m_references.begin(); i != m_references.end(); ++i )
    {
        wxString line = *i;

        line = line.Strip(wxString::both);
        while (!line.empty())
        {
            size_t idx = 0;
            while (idx < line.length() && line[idx] != _T(':')) { idx++; }
            while (idx < line.length() && !wxIsspace(line[idx])) { idx++; }

            refs.push_back(line.Left(idx));
            line = line.Mid(idx).Strip(wxString::both);
        }
    }

    return refs;
}

wxString CatalogItem::GetOldMsgid() const
{
    wxString s;
    for (auto line: m_oldMsgid)
    {
        if (line.length() < 2)
            continue;
        if (line.Last() == '"')
            line.RemoveLast();
        if (line[0] == '"')
            line.Remove(0, 1);
        if (line.StartsWith("msgid \""))
            line.Remove(0, 7);
        else if (line.StartsWith("msgid_plural \""))
            line.replace(0, 14, "\n");
        s += UnescapeCString(line);
    }
    return s;
}


// Catalog file creation factories:

CatalogPtr Catalog::Create(Type type)
{
    switch (type)
    {
        case Type::PO:
        case Type::POT:
            return std::make_shared<POCatalog>(type);
    }

    return CatalogPtr(); // silence VC++ warning
}

CatalogPtr Catalog::Create(const wxString& filename, int flags)
{
    wxString ext;
    wxFileName::SplitPath(filename, nullptr, nullptr, nullptr, &ext);
    ext.MakeLower();

    CatalogPtr cat;
    if (POCatalog::CanLoadFile(ext))
    {
        cat = std::make_shared<POCatalog>(filename, flags);
        flags = 0; // don't do the stuff below that is already handled by POCatalog's parser
    }
    else if (XLIFFCatalog::CanLoadFile(ext))
    {
        cat = XLIFFCatalog::Open(filename);
    }

    if (!cat)
        throw Exception(wxString::Format(_(L"File “%s” is in unsupported format."), filename));

    if (flags & CreationFlag_IgnoreTranslations)
    {
        for (auto item: cat->m_items)
            item->ClearTranslation();
    }

    return cat;
}

bool Catalog::CanLoadFile(const wxString& extension)
{
    return POCatalog::CanLoadFile(extension) ||
           XLIFFCatalog::CanLoadFile(extension);
}
