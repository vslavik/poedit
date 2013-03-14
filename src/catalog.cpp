/*
 *  This file is part of Poedit (http://www.poedit.net)
 *
 *  Copyright (C) 1999-2013 Vaclav Slavik
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

#include <stdio.h>
#include <wx/utils.h>
#include <wx/tokenzr.h>
#include <wx/log.h>
#include <wx/intl.h>
#include <wx/datetime.h>
#include <wx/config.h>
#include <wx/textfile.h>
#include <wx/strconv.h>
#include <wx/msgdlg.h>
#include <wx/filename.h>

#include <set>
#include <algorithm>

#include "catalog.h"
#include "digger.h"
#include "gexecute.h"
#include "progressinfo.h"
#include "summarydlg.h"
#include "isocodes.h"
#include "utility.h"
#include "version.h"


// ----------------------------------------------------------------------
// Textfile processing utilities:
// ----------------------------------------------------------------------


// Read one line from file, remove all \r and \n characters, ignore empty lines
static wxString ReadTextLine(wxTextFile* f)
{
    wxString s;

    while (s.empty())
    {
        if (f->Eof()) return wxEmptyString;

        // read next line and strip insignificant whitespace from it:
        s = f->GetNextLine().Strip(wxString::both);
    }

    return s;
}


// If input begins with pattern, fill output with end of input (without
// pattern; strips trailing spaces) and return true.  Return false otherwise
// and don't touch output. Is permissive about whitespace in the input:
// a space (' ') in pattern will match any number of any whitespace characters
// on that position in input.
static bool ReadParam(const wxString& input, const wxString& pattern, wxString& output)
{
    if (input.size() < pattern.size())
        return false;

    unsigned in_pos = 0;
    unsigned pat_pos = 0;
    while (pat_pos < pattern.size() && in_pos < input.size())
    {
        const wxChar pat = pattern[pat_pos++];

        if (pat == _T(' '))
        {
            if (!wxIsspace(input[in_pos++]))
                return false;

            while (wxIsspace(input[in_pos]))
            {
                in_pos++;
                if (in_pos == input.size())
                    return false;
            }
        }
        else
        {
            if (input[in_pos++] != pat)
                return false;
        }

    }

    if (pat_pos < pattern.size()) // pattern not fully matched
        return false;

    output = input.Mid(in_pos).Strip(wxString::trailing);
    return true;
}


// Checks if the file was loaded correctly, i.e. that non-empty lines
// ended up non-empty in memory, after doing charset conversion in
// wxTextFile. This detects for example files that claim they are in UTF-8
// while in fact they are not.
bool VerifyFileCharset(const wxTextFile& f, const wxString& filename,
                       const wxString& charset)
{
    wxTextFile f2;

    if (!f2.Open(filename, wxConvISO8859_1))
        return false;

    if (f.GetLineCount() != f2.GetLineCount())
    {
        wxLogError(_("%i lines of file '%s' were not loaded correctly."),
                   (int)f2.GetLineCount() - (int)f.GetLineCount(),
                   filename.c_str());
        return false;
    }

    bool ok = true;
    size_t cnt = f.GetLineCount();
    for (size_t i = 0; i < cnt; i++)
    {
        if (f[i].empty() && !f2[i].empty()) // wxMBConv conversion failed
        {
            wxLogError(
                _("Line %u of file '%s' is corrupted (not valid %s data)."),
                i, filename.c_str(), charset.c_str());
            ok = false;
        }
    }

    return ok;
}


// converts \n into newline character and \\ into \:
static wxString UnescapeCEscapes(const wxString& str)
{
    wxString out;
    size_t len = str.size();
    size_t i;

    if ( len == 0 )
        return str;

    for (i = 0; i < len-1; i++)
    {
        if (str[i] == _T('\\'))
        {
            switch ((wxChar)str[i+1])
            {
                case _T('n'):
                    out << _T('\n');
                    i++;
                    break;
                case _T('\\'):
                    out << _T('\\');
                    i++;
                    break;
                case _T('"'):
                    out << _T('"');
                    i++;
                    break;
                default:
                    out << _T('\\');
            }
        }
        else
        {
            out << str[i];
        }
    }

    // last character:
    if (i < len)
        out << str[i];

    return out;
}


// ----------------------------------------------------------------------
// Catalog::HeaderData
// ----------------------------------------------------------------------

void Catalog::HeaderData::FromString(const wxString& str)
{
    wxString hdr(str);
    hdr = UnescapeCEscapes(hdr);
    wxStringTokenizer tkn(hdr, _T("\n"));
    wxString ln;

    m_entries.clear();

    while (tkn.HasMoreTokens())
    {
        ln = tkn.GetNextToken();
        size_t pos = ln.find(_T(':'));
        if (pos == wxString::npos)
        {
            wxLogError(_("Malformed header: '%s'"), ln.c_str());
        }
        else
        {
            Entry en;
            en.Key = wxString(ln.substr(0, pos)).Strip(wxString::both);
            en.Value = wxString(ln.substr(pos + 1)).Strip(wxString::both);

            // correct some common errors:
            if ( en.Key == _T("Plural-Forms") )
            {
                if ( !en.Value.empty() && !en.Value.EndsWith(_T(";")) )
                    en.Value += _T(";");
            }

            m_entries.push_back(en);
            wxLogTrace(_T("poedit.header"),
                       _T("%s='%s'"), en.Key.c_str(), en.Value.c_str());
        }
    }

    ParseDict();
}

wxString Catalog::HeaderData::ToString(const wxString& line_delim)
{
    UpdateDict();

    wxString hdr;
    for (std::vector<Entry>::const_iterator i = m_entries.begin();
         i != m_entries.end(); i++)
    {
        wxString v(i->Value);
        v.Replace(_T("\\"), _T("\\\\"));
        v.Replace(_T("\""), _T("\\\""));
        hdr << i->Key << _T(": ") << v << _T("\\n") << line_delim;
    }
    return hdr;
}

void Catalog::HeaderData::UpdateDict()
{
    SetHeader(_T("Project-Id-Version"), Project);
    SetHeader(_T("POT-Creation-Date"), CreationDate);
    SetHeader(_T("PO-Revision-Date"), RevisionDate);

    if (TranslatorEmail.empty())
        SetHeader(_T("Last-Translator"), Translator);
    else
        SetHeader(_T("Last-Translator"),
                  Translator + _T(" <") + TranslatorEmail + _T(">"));

    if (TeamEmail.empty())
        SetHeader(_T("Language-Team"), Team);
    else
        SetHeader(_T("Language-Team"),
                  Team + _T(" <") + TeamEmail + _T(">"));

    SetHeader(_T("MIME-Version"), _T("1.0"));
    SetHeader(_T("Content-Type"), _T("text/plain; charset=") + Charset);
    SetHeader(_T("Content-Transfer-Encoding"), _T("8bit"));
    SetHeaderNotEmpty(_T("Language"), LanguageCode);
    SetHeader(_T("X-Generator"), wxString::FromAscii("Poedit " POEDIT_VERSION));

    // Set extended information:

    SetHeaderNotEmpty(_T("X-Poedit-SourceCharset"), SourceCodeCharset);

    if (!Keywords.empty())
    {
        wxString kw;
        for (size_t i = 0; i < Keywords.GetCount(); i++)
            kw += Keywords[i] + _T(';');
        kw.RemoveLast();
        SetHeader(_T("X-Poedit-KeywordsList"), kw);
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
        DeleteHeader(_T("X-Poedit-Bookmarks"));
    else
        SetHeader(_T("X-Poedit-Bookmarks"), bk);

    SetHeaderNotEmpty(_T("X-Poedit-Basepath"), BasePath);

    i = 0;
    wxString path;
    while (true)
    {
        path.Printf(_T("X-Poedit-SearchPath-%i"), i);
        if (!HasHeader(path))
            break;
        DeleteHeader(path);
        i++;
    }

    for (i = 0; i < SearchPaths.GetCount(); i++)
    {
        path.Printf(_T("X-Poedit-SearchPath-%i"), i);
        SetHeader(path, SearchPaths[i]);
    }
}

void Catalog::HeaderData::ParseDict()
{
    wxString dummy;

    Project = GetHeader(_T("Project-Id-Version"));
    CreationDate = GetHeader(_T("POT-Creation-Date"));
    RevisionDate = GetHeader(_T("PO-Revision-Date"));

    dummy = GetHeader(_T("Last-Translator"));
    if (!dummy.empty())
    {
        wxStringTokenizer tkn(dummy, _T("<>"));
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

    dummy = GetHeader(_T("Language-Team"));
    if (!dummy.empty())
    {
        wxStringTokenizer tkn(dummy, _T("<>"));
        if (tkn.CountTokens() != 2)
        {
            Team = dummy;
            TeamEmail = wxEmptyString;
        }
        else
        {
            Team = tkn.GetNextToken().Strip(wxString::trailing);
            TeamEmail = tkn.GetNextToken();
        }
    }

    wxString ctype = GetHeader(_T("Content-Type"));
    int charsetPos = ctype.Find(_T("; charset="));
    if (charsetPos != -1)
    {
        Charset =
            ctype.Mid(charsetPos + strlen("; charset=")).Strip(wxString::both);
    }
    else
    {
        Charset = _T("iso-8859-1");
    }

    // Parse language information, with backwards compatibility with X-Poedit-*:
    LanguageCode = GetHeader(_T("Language"));
    if ( LanguageCode.empty() )
    {
        wxString X_Language = GetHeader(_T("X-Poedit-Language"));
        if ( !X_Language.empty() )
            X_Language = LookupLanguageCode(X_Language);

        wxString X_Country = GetHeader(_T("X-Poedit-Country"));
        if ( !X_Country.empty() )
            X_Country = LookupCountryCode(X_Country);

        if ( !X_Language.empty() )
        {
            LanguageCode = X_Language;
            if ( !X_Country.empty() )
                LanguageCode += _T("_") + X_Country;
        }
    }
    DeleteHeader(_T("X-Poedit-Language"));
    DeleteHeader(_T("X-Poedit-Country"));

    // Parse extended information:
    SourceCodeCharset = GetHeader(_T("X-Poedit-SourceCharset"));
    BasePath = GetHeader(_T("X-Poedit-Basepath"));

    Keywords.Clear();
    wxString kw = GetHeader(_T("X-Poedit-KeywordsList"));
    if (!kw.empty())
    {
        wxStringTokenizer tkn(kw, _T(";"));
        while (tkn.HasMoreTokens())
            Keywords.Add(tkn.GetNextToken());
    }
    else
    {
        // try backward-compatibility version X-Poedit-Keywords. The difference
        // is the separator used, see
        // http://sourceforge.net/tracker/index.php?func=detail&aid=1206579&group_id=27043&atid=389153

        wxString kw = GetHeader(_T("X-Poedit-Keywords"));
        if (!kw.empty())
        {
            wxStringTokenizer tkn(kw, _T(","));
            while (tkn.HasMoreTokens())
                Keywords.Add(tkn.GetNextToken());

            // and remove it, it's not for newer versions:
            DeleteHeader(_T("X-Poedit-Keywords"));
        }
    }

    int i;
    for(i = 0; i < BOOKMARK_LAST; i++)
    {
      Bookmarks[i] = NO_BOOKMARK;
    }
    wxString bk = GetHeader(_T("X-Poedit-Bookmarks"));
    if (!bk.empty())
    {
        wxStringTokenizer tkn(bk, _T(","));
        i=0;
        long int val;
        while (tkn.HasMoreTokens() && i<BOOKMARK_LAST)
        {
            tkn.GetNextToken().ToLong(&val);
            Bookmarks[i] = val;
            i++;
        }
    }

    i = 0;
    wxString path;
    SearchPaths.Clear();
    while (true)
    {
        path.Printf(_T("X-Poedit-SearchPath-%i"), i);
        if (!HasHeader(path))
            break;
        SearchPaths.Add(GetHeader(path));
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
// Parsers
// ----------------------------------------------------------------------

bool CatalogParser::Parse()
{
    if (m_textFile->GetLineCount() == 0)
        return false;

    wxString line, dummy;
    wxString mflags, mstr, msgid_plural, mcomment;
    wxArrayString mrefs, mautocomments, mtranslations;
    wxArrayString msgid_old;
    bool has_plural = false;
    bool has_context = false;
    wxString msgctxt;
    unsigned mlinenum = 0;

    line = m_textFile->GetFirstLine();
    if (line.empty()) line = ReadTextLine(m_textFile);

    while (!line.empty())
    {
        // ignore empty special tags (except for automatic comments which we
        // DO want to preserve):
        while (line == _T("#,") || line == _T("#:") || line == _T("#|"))
            line = ReadTextLine(m_textFile);

        // flags:
        // Can't we have more than one flag, now only the last is kept ...
        if (ReadParam(line, _T("#, "), dummy))
        {
            mflags = _T("#, ") + dummy;
            line = ReadTextLine(m_textFile);
        }

        // auto comments:
        if (ReadParam(line, _T("#. "), dummy) || ReadParam(line, _T("#."), dummy)) // second one to account for empty auto comments
        {
            mautocomments.Add(dummy);
            line = ReadTextLine(m_textFile);
        }

        // references:
        else if (ReadParam(line, _T("#: "), dummy))
        {
            // Just store the references unmodified, we don't modify this
            // data anywhere.
            mrefs.push_back(dummy);
            line = ReadTextLine(m_textFile);
        }

        // previous msgid value:
        else if (ReadParam(line, _T("#| "), dummy))
        {
            msgid_old.Add(dummy);
            line = ReadTextLine(m_textFile);
        }

        // msgctxt:
        else if (ReadParam(line, _T("msgctxt \""), dummy))
        {
            has_context = true;
            msgctxt = dummy.RemoveLast();
            while (!(line = ReadTextLine(m_textFile)).empty())
            {
                if (line[0u] == _T('\t'))
                    line.Remove(0, 1);
                if (line[0u] == _T('"') && line.Last() == _T('"'))
                    msgctxt += line.Mid(1, line.Length() - 2);
                else
                    break;
            }
        }

        // msgid:
        else if (ReadParam(line, _T("msgid \""), dummy))
        {
            mstr = dummy.RemoveLast();
            mlinenum = m_textFile->GetCurrentLine() + 1;
            while (!(line = ReadTextLine(m_textFile)).empty())
            {
                if (line[0u] == _T('\t'))
                    line.Remove(0, 1);
                if (line[0u] == _T('"') && line.Last() == _T('"'))
                    mstr += line.Mid(1, line.Length() - 2);
                else
                    break;
            }
        }

        // msgid_plural:
        else if (ReadParam(line, _T("msgid_plural \""), dummy))
        {
            msgid_plural = dummy.RemoveLast();
            has_plural = true;
            mlinenum = m_textFile->GetCurrentLine() + 1;
            while (!(line = ReadTextLine(m_textFile)).empty())
            {
                if (line[0u] == _T('\t'))
                    line.Remove(0, 1);
                if (line[0u] == _T('"') && line.Last() == _T('"'))
                    msgid_plural += line.Mid(1, line.Length() - 2);
                else
                    break;
            }
        }

        // msgstr:
        else if (ReadParam(line, _T("msgstr \""), dummy))
        {
            if (has_plural)
            {
                wxLogError(_("Broken catalog file: singular form msgstr used together with msgid_plural"));
                return false;
            }

            wxString str = dummy.RemoveLast();
            while (!(line = ReadTextLine(m_textFile)).empty())
            {
                if (line[0u] == _T('\t'))
                    line.Remove(0, 1);
                if (line[0u] == _T('"') && line.Last() == _T('"'))
                    str += line.Mid(1, line.Length() - 2);
                else
                    break;
            }
            mtranslations.Add(str);

            bool shouldIgnore = m_ignoreHeader && mstr.empty();
            if ( !shouldIgnore )
            {
                if (!OnEntry(mstr, wxEmptyString, false,
                             has_context, msgctxt,
                             mtranslations,
                             mflags, mrefs, mcomment, mautocomments, msgid_old,
                             mlinenum))
                {
                    return false;
                }
            }

            mcomment = mstr = msgid_plural = msgctxt = mflags = wxEmptyString;
            has_plural = has_context = false;
            mrefs.Clear();
            mautocomments.Clear();
            mtranslations.Clear();
            msgid_old.Clear();
        }

        // msgstr[i]:
        else if (ReadParam(line, _T("msgstr["), dummy))
        {
            if (!has_plural)
            {
                wxLogError(_("Broken catalog file: plural form msgstr used without msgid_plural"));
                return false;
            }

            wxString idx = dummy.BeforeFirst(_T(']'));
            wxString label = _T("msgstr[") + idx + _T("]");

            while (ReadParam(line, label + _T(" \""), dummy))
            {
                wxString str = dummy.RemoveLast();

                while (!(line=ReadTextLine(m_textFile)).empty())
                {
                    line.Trim(/*fromRight=*/false);
                    if (line[0u] == _T('"') && line.Last() == _T('"'))
                        str += line.Mid(1, line.Length() - 2);
                    else
                    {
                        if (ReadParam(line, _T("msgstr["), dummy))
                        {
                            idx = dummy.BeforeFirst(_T(']'));
                            label = _T("msgstr[") + idx + _T("]");
                        }
                        break;
                    }
                }
                mtranslations.Add(str);
            }

            if (!OnEntry(mstr, msgid_plural, true,
                         has_context, msgctxt,
                         mtranslations,
                         mflags, mrefs, mcomment, mautocomments, msgid_old,
                         mlinenum))
            {
                return false;
            }

            mcomment = mstr = msgid_plural = msgctxt = mflags = wxEmptyString;
            has_plural = has_context = false;
            mrefs.Clear();
            mautocomments.Clear();
            mtranslations.Clear();
            msgid_old.Clear();
        }

        // deleted lines:
        else if (ReadParam(line, _T("#~ "), dummy))
        {
            wxArrayString deletedLines;
            deletedLines.Add(line);
            mlinenum = m_textFile->GetCurrentLine() + 1;
            while (!(line = ReadTextLine(m_textFile)).empty())
            {
                // if line does not start with "#~ " anymore, stop reading
                if (!ReadParam(line, _T("#~ "), dummy))
                    break;
                // if the line starts with "#~ msgid", we skipped an empty line
                // and it's a new entry, so stop reading too (see bug #329)
                if (ReadParam(line, _T("#~ msgid"), dummy))
                    break;

                deletedLines.Add(line);
            }
            if (!OnDeletedEntry(deletedLines,
                                mflags, mrefs, mcomment, mautocomments, mlinenum))
            {
                return false;
            }

            mcomment = mstr = msgid_plural = mflags = wxEmptyString;
            has_plural = false;
            mrefs.Clear();
            mautocomments.Clear();
            mtranslations.Clear();
            msgid_old.Clear();
        }

        // comment:
        else if (line[0u] == _T('#'))
        {
            bool readNewLine = false;

            while (!line.empty() &&
                    line[0u] == _T('#') &&
                   (line.Length() < 2 || (line[1u] != _T(',') && line[1u] != _T(':') && line[1u] != _T('.') )))
            {
                mcomment << line << _T('\n');
                readNewLine = true;
                line = ReadTextLine(m_textFile);
            }

            if (!readNewLine)
                line = ReadTextLine(m_textFile);
        }

        else
        {
            line = ReadTextLine(m_textFile);
        }
    }

    return true;
}



class CharsetInfoFinder : public CatalogParser
{
    public:
        CharsetInfoFinder(wxTextFile *f)
                : CatalogParser(f), m_charset(_T("iso-8859-1")) {}
        wxString GetCharset() const { return m_charset; }

    protected:
        wxString m_charset;

        virtual bool OnEntry(const wxString& msgid,
                             const wxString& /*msgid_plural*/,
                             bool /*has_plural*/,
                             bool /*has_context*/,
                             const wxString& /*context*/,
                             const wxArrayString& mtranslations,
                             const wxString& /*flags*/,
                             const wxArrayString& /*references*/,
                             const wxString& /*comment*/,
                             const wxArrayString& /*autocomments*/,
                             const wxArrayString& /*msgid_old*/,
                             unsigned /*lineNumber*/)
        {
            if (msgid.empty())
            {
                // gettext header:
                Catalog::HeaderData hdr;
                hdr.FromString(mtranslations[0]);
                m_charset = hdr.Charset;
                if (m_charset == _T("CHARSET"))
                    m_charset = _T("iso-8859-1");
                return false; // stop parsing
            }
            return true;
        }
};



class LoadParser : public CatalogParser
{
    public:
        LoadParser(Catalog *c, wxTextFile *f)
              : CatalogParser(f), m_catalog(c) {}

    protected:
        Catalog *m_catalog;

        virtual bool OnEntry(const wxString& msgid,
                             const wxString& msgid_plural,
                             bool has_plural,
                             bool has_context,
                             const wxString& context,
                             const wxArrayString& mtranslations,
                             const wxString& flags,
                             const wxArrayString& references,
                             const wxString& comment,
                             const wxArrayString& autocomments,
                             const wxArrayString& msgid_old,
                             unsigned lineNumber);

        virtual bool OnDeletedEntry(const wxArrayString& deletedLines,
                                    const wxString& flags,
                                    const wxArrayString& references,
                                    const wxString& comment,
                                    const wxArrayString& autocomments,
                                    unsigned lineNumber);
};


bool LoadParser::OnEntry(const wxString& msgid,
                         const wxString& msgid_plural,
                         bool has_plural,
                         bool has_context,
                         const wxString& context,
                         const wxArrayString& mtranslations,
                         const wxString& flags,
                         const wxArrayString& references,
                         const wxString& comment,
                         const wxArrayString& autocomments,
                         const wxArrayString& msgid_old,
                         unsigned lineNumber)
{
    if (msgid.empty())
    {
        // gettext header:
        m_catalog->m_header.FromString(mtranslations[0]);
        m_catalog->m_header.Comment = comment;
    }
    else
    {
        CatalogItem d;
        if (!flags.empty()) d.SetFlags(flags);
        d.SetString(msgid);
        if (has_plural)
            d.SetPluralString(msgid_plural);
        if (has_context)
            d.SetContext(context);
        d.SetTranslations(mtranslations);
        d.SetComment(comment);
        d.SetLineNumber(lineNumber);
        for (size_t i = 0; i < references.GetCount(); i++)
            d.AddReference(references[i]);
        for (size_t i = 0; i < autocomments.GetCount(); i++)
            d.AddAutoComments(autocomments[i]);
        d.SetOldMsgid(msgid_old);
        m_catalog->AddItem(d);
    }
    return true;
}

bool LoadParser::OnDeletedEntry(const wxArrayString& deletedLines,
                                const wxString& flags,
                                const wxArrayString& /*references*/,
                                const wxString& comment,
                                const wxArrayString& autocomments,
                                unsigned lineNumber)
{
    CatalogDeletedData d;
    if (!flags.empty()) d.SetFlags(flags);
    d.SetDeletedLines(deletedLines);
    d.SetComment(comment);
    d.SetLineNumber(lineNumber);
    for (size_t i = 0; i < autocomments.GetCount(); i++)
      d.AddAutoComments(autocomments[i]);
    m_catalog->AddDeletedItem(d);

    return true;
}



// ----------------------------------------------------------------------
// Catalog class
// ----------------------------------------------------------------------

Catalog::Catalog()
{
    m_isOk = true;
    m_header.BasePath = wxEmptyString;
    for(int i = BOOKMARK_0; i < BOOKMARK_LAST; i++)
    {
        m_header.Bookmarks[i] = -1;
    }
}


Catalog::~Catalog()
{
    Clear();
}


Catalog::Catalog(const wxString& po_file, int flags)
{
    m_isOk = Load(po_file, flags);
}


static wxString GetCurrentTimeRFC822()
{
    wxDateTime timenow = wxDateTime::Now();
    int offs = wxDateTime::TimeZone(wxDateTime::Local).GetOffset();
    wxString s;
    s.Printf(_T("%s%s%02i%02i"),
             timenow.Format(_T("%Y-%m-%d %H:%M")).c_str(),
             (offs > 0) ? _T("+") : _T("-"),
             abs(offs) / 3600, (abs(offs) / 60) % 60);
    return s;
}

void Catalog::CreateNewHeader()
{
    HeaderData &dt = Header();

    dt.CreationDate = GetCurrentTimeRFC822();
    dt.RevisionDate = dt.CreationDate;

    dt.LanguageCode = wxEmptyString;
    dt.Project = wxEmptyString;
    dt.Team = wxEmptyString;
    dt.TeamEmail = wxEmptyString;
    dt.Charset = _T("UTF-8");
    dt.Translator = wxConfig::Get()->Read(_T("translator_name"), wxEmptyString);
    dt.TranslatorEmail = wxConfig::Get()->Read(_T("translator_email"), wxEmptyString);
    dt.SourceCodeCharset = wxEmptyString;

    // NB: keep in sync with Catalog::Update!
    dt.Keywords.Add(_T("_"));
    dt.Keywords.Add(_T("gettext"));
    dt.Keywords.Add(_T("gettext_noop"));

    dt.BasePath = _T(".");

    dt.UpdateDict();
}

void Catalog::CreateNewHeader(const Catalog::HeaderData& pot_header)
{
    HeaderData &dt = Header();
    dt = pot_header;

    // UTF-8 should be used by default no matter what the POT uses
    dt.Charset = _T("UTF-8");

    // clear the fields that are translation-specific:
    dt.LanguageCode.clear();
    dt.Team.clear();
    dt.TeamEmail.clear();

    // translator should be pre-filled
    dt.Translator = wxConfig::Get()->Read(_T("translator_name"), wxEmptyString);
    dt.TranslatorEmail = wxConfig::Get()->Read(_T("translator_email"), wxEmptyString);

    dt.UpdateDict();
}


bool Catalog::Load(const wxString& po_file, int flags)
{
    wxTextFile f;

    Clear();
    m_isOk = false;
    m_fileName = po_file;
    m_header.BasePath = wxEmptyString;

    /* Load the .po file: */

    if (!f.Open(po_file, wxConvISO8859_1))
        return false;

    {
        wxLogNull null; // don't report parsing errors from here, report them later
        CharsetInfoFinder charsetFinder(&f);
        charsetFinder.Parse();
        m_header.Charset = charsetFinder.GetCharset();
    }

    f.Close();
    wxCSConv encConv(m_header.Charset);
    if (!f.Open(po_file, encConv))
        return false;

    if (!VerifyFileCharset(f, po_file, m_header.Charset))
    {
        wxLogError(_("There were errors when loading the catalog. Some data may be missing or corrupted as the result."));
    }

    LoadParser parser(this, &f);
    parser.IgnoreHeader(flags & CreationFlag_IgnoreHeader);
    if (!parser.Parse())
    {
        wxLogError(
            wxString::Format(
                _("Couldn't load file %s, it is probably corrupted."),
                po_file.c_str()));
        return false;
    }

    // now that the catalog is loaded, update its items with the bookmarks
    for (unsigned i = BOOKMARK_0; i < BOOKMARK_LAST; i++)
    {
        if (m_header.Bookmarks[i] != -1 &&
            m_header.Bookmarks[i] < (int)m_items.size())
        {
            m_items[m_header.Bookmarks[i]].SetBookmark(
                    static_cast<Bookmark>(i));
        }
    }


    m_isOk = true;

    f.Close();

    if ( flags & CreationFlag_IgnoreHeader )
        CreateNewHeader();

    return true;
}

void Catalog::AddItem(const CatalogItem& data)
{
    m_items.push_back(data);
}

void Catalog::AddDeletedItem(const CatalogDeletedData& data)
{
    m_deletedItems.push_back(data);
}

bool Catalog::HasDeletedItems()
{
    return !m_deletedItems.empty();
}

void Catalog::RemoveDeletedItems()
{
    m_deletedItems.clear();
}

CatalogItem *Catalog::FindItemByLine(int lineno)
{
    CatalogItem *last = NULL;

    for ( CatalogItemArray::iterator i = m_items.begin();
          i != m_items.end(); ++i )
    {
        if ( i->GetLineNumber() > lineno )
            return last;
        last = &(*i);
    }

    return last;
}

void Catalog::Clear()
{
    m_items.clear();
    m_deletedItems.clear();
    m_isOk = true;
    for(int i = BOOKMARK_0; i < BOOKMARK_LAST; i++)
    {
        m_header.Bookmarks[i] = -1;
    }
}

int Catalog::SetBookmark(int id, Bookmark bookmark)
{
    int result = (bookmark==NO_BOOKMARK)?-1:m_header.Bookmarks[bookmark];

    // unset previous bookmarks, if any
    Bookmark bk = m_items[id].GetBookmark();
    if (bk != NO_BOOKMARK)
        m_header.Bookmarks[bk] = -1;
    if (result > -1)
        m_items[result].SetBookmark(NO_BOOKMARK);

    // set new bookmark
    m_items[id].SetBookmark(bookmark);
    if (bookmark != NO_BOOKMARK)
        m_header.Bookmarks[bookmark] = id;

    // return id of previous item for that bookmark
    return result;
}


// misc file-saving helpers
namespace
{

inline bool CanEncodeStringToCharset(const wxString& s, wxMBConv& conv)
{
    if (s.empty())
        return true;
    const wxCharBuffer converted(s.mb_str(conv));
#if wxCHECK_VERSION(2,9,0)
    if ( converted.length() == 0 )
#else
    const char *cs = (const char*)converted;
    if ( cs == NULL || strlen(cs) == 0 )
#endif
        return false;
    return true;
}

bool CanEncodeToCharset(const wxTextFile& f, const wxString& charset)
{
    if (charset.Lower() == _T("utf-8") || charset.Lower() == _T("utf8"))
        return true;

    wxCSConv conv(charset);

    const size_t lines = f.GetLineCount();
    for ( size_t i = 0; i < lines; i++ )
    {
        if ( !CanEncodeStringToCharset(f.GetLine(i), conv) )
            return false;
    }

    return true;
}


void GetCRLFBehaviour(wxTextFileType& type, bool& preserve)
{
    wxString format = wxConfigBase::Get()->Read(_T("crlf_format"), _T("unix"));

    if (format == _T("win")) type = wxTextFileType_Dos;
    else /* _T("unix") or obsolete settings */ type = wxTextFileType_Unix;

    preserve = (bool)(wxConfigBase::Get()->Read(_T("keep_crlf"), true));
}


wxTextFileType GetDesiredCRLFFormat(const wxString& po_file)
{
    wxTextFileType crlfDefault, crlf;
    bool crlfPreserve;
    GetCRLFBehaviour(crlfDefault, crlfPreserve);

    wxTextFile f;
    if ( crlfPreserve && wxFileExists(po_file) &&
         f.Open(po_file, wxConvISO8859_1) )
    {
        wxLogNull null;
        crlf = f.GuessType();

        // Discard any unsupported setting. In particular, we ignore "Mac"
        // line endings, because the ancient OS 9 systems aren't used anymore,
        // OSX uses Unix ending *and* "Mac" endings break gettext tools. So if
        // we encounter a catalog with "Mac" line endings, we silently convert
        // it into Unix endings (i.e. the modern Mac).
        if (crlf == wxTextFileType_Mac)
            crlf = wxTextFileType_Unix;
        if (crlf != wxTextFileType_Dos && crlf != wxTextFileType_Unix)
            crlf = crlfDefault;

        f.Close();
    }
    else
    {
        crlf = crlfDefault;
    }

    return crlf;
}


void SaveMultiLines(wxTextFile &f, const wxString& text)
{
    wxStringTokenizer tkn(text, _T('\n'));
    while (tkn.HasMoreTokens())
        f.AddLine(tkn.GetNextToken());
}

/** Adds \n characters as neccesary for good-looking output
 */
wxString FormatStringForFile(const wxString& text)
{
    wxString s;
    unsigned n_cnt = 0;
    int len = text.length();

    s.Alloc(len + 16);
    // Scan the string up to len-2 because we don't want to account for the
    // very last \n on the line:
    //       "some\n string \n"
    //                      ^
    //                      |
    //                      \--- = len-2
    int i;
    for (i = 0; i < len-2; i++)
    {
        if (text[i] == _T('\\') && text[i+1] == _T('n'))
        {
            n_cnt++;
            s << _T("\\n\"\n\"");
            i++;
        }
        else
            s << text[i];
    }
    // ...and add not yet processed characters to the string...
    for (; i < len; i++)
        s << text[i];

    if (n_cnt >= 1)
        return _T("\"\n\"") + s;
    else
        return s;
}

} // anonymous namespace


bool Catalog::Save(const wxString& po_file, bool save_mo, int& validation_errors)
{
    if ( wxFileExists(po_file) && !wxFile::Access(po_file, wxFile::write) )
    {
        wxLogError(_("File '%s' is read-only and cannot be saved.\nPlease save it under different name."),
                   po_file.c_str());
        return false;
    }

    // Update information about last modification time. But if the header
    // was empty previously, the author apparently doesn't want this header
    // set, so don't mess with it. See https://sourceforge.net/tracker/?func=detail&atid=389156&aid=1900298&group_id=27043
    // for motivation:
    if ( !m_header.RevisionDate.empty() )
        m_header.RevisionDate = GetCurrentTimeRFC822();

    const wxString po_file_temp = po_file + _T(".temp.po");
    if ( wxFileExists(po_file_temp) )
        wxRemoveFile(po_file_temp);

    if ( !DoSaveOnly(po_file_temp) )
    {
        wxLogError(_("Couldn't save file %s."), po_file.c_str());
        return false;
    }

    validation_errors = DoValidate(po_file_temp);

    // Now that the file was written, run msgcat to re-format it according
    // to the usual format. This is a (barely) passable fix for #25 until
    // proper preservation of formatting is implemented.

    int msgcat_ok = false;
    {
        // Ignore msgcat errors output (but not exit code), because it
        //   a) complains about things DoValidate() already complained above
        //   b) issues warnings about source-extraction things (e.g. using non-ASCII
        //      msgids) that, while correct, are not something a *translator* can
        //      do anything about.
        wxLogNull null;
        msgcat_ok =
              ExecuteGettext
              (
                  wxString::Format(_T("msgcat --force-po -o \"%s\" \"%s\""),
                                   po_file.c_str(),
                                   po_file_temp.c_str())
              )
              && wxFileExists(po_file);
    }

    if ( msgcat_ok )
    {
        wxRemoveFile(po_file_temp);
    }
    else
    {
        if ( !wxRenameFile(po_file_temp, po_file, /*overwrite=*/true) )
        {
            wxLogError(_("Couldn't save file %s."), po_file.c_str());
        }
        else
        {
            // Only shows msgcat's failure warning if we don't also get
            // validation errors, because if we do, the cause is likely the
            // same.
            if ( !validation_errors )
                wxLogWarning(_("There was a problem formatting the file nicely (but it was saved all right)."));
        }
    }

    /* If the user wants it, compile .mo file right now: */

    if (save_mo && wxConfig::Get()->Read(_T("compile_mo"), (long)true))
    {
        const wxString mofile = po_file.BeforeLast(_T('.')) + _T(".mo");
        if ( !ExecuteGettext
              (
                  wxString::Format(_T("msgfmt -o \"%s\" \"%s\""),
                                   mofile.c_str(),
                                   po_file.c_str())
              ) )
        {
            // Don't report errors, they were reported as part of validation
            // step above.  Notice that we run msgfmt *without* the -c flag
            // here to create the MO file in as many cases as possible, even if
            // it has some errors.
        }
    }

    m_fileName = po_file;

    return true;
}


bool Catalog::DoSaveOnly(const wxString& po_file)
{
    wxTextFileType crlf = GetDesiredCRLFFormat(po_file);

    wxTextFile f;
    if (!f.Create(po_file))
        return false;

    return DoSaveOnly(f, crlf);
}

bool Catalog::DoSaveOnly(wxTextFile& f, wxTextFileType crlf)
{
    /* Save .po file: */
    if (!m_header.Charset || m_header.Charset == _T("CHARSET"))
        m_header.Charset = _T("UTF-8");

    SaveMultiLines(f, m_header.Comment);
    f.AddLine(_T("msgid \"\""));
    f.AddLine(_T("msgstr \"\""));
    wxString pohdr = wxString(_T("\"")) + m_header.ToString(_T("\"\n\""));
    pohdr.RemoveLast();
    SaveMultiLines(f, pohdr);
    f.AddLine(wxEmptyString);

    for (unsigned i = 0; i < m_items.size(); i++)
    {
        CatalogItem& data = m_items[i];
        data.SetLineNumber(f.GetLineCount()+1);
        SaveMultiLines(f, data.GetComment());
        for (unsigned i = 0; i < data.GetAutoComments().GetCount(); i++)
        {
            if (data.GetAutoComments()[i].empty())
              f.AddLine(_T("#."));
            else
              f.AddLine(_T("#. ") + data.GetAutoComments()[i]);
        }
        for (unsigned i = 0; i < data.GetRawReferences().GetCount(); i++)
            f.AddLine(_T("#: ") + data.GetRawReferences()[i]);
        wxString dummy = data.GetFlags();
        if (!dummy.empty())
            f.AddLine(dummy);
        for (unsigned i = 0; i < data.GetOldMsgid().GetCount(); i++)
            f.AddLine(_T("#| ") + data.GetOldMsgid()[i]);
        if ( data.HasContext() )
        {
            SaveMultiLines(f, _T("msgctxt \"") + FormatStringForFile(data.GetContext()) + _T("\""));
        }
        dummy = FormatStringForFile(data.GetString());
        SaveMultiLines(f, _T("msgid \"") + dummy + _T("\""));
        if (data.HasPlural())
        {
            dummy = FormatStringForFile(data.GetPluralString());
            SaveMultiLines(f, _T("msgid_plural \"") + dummy + _T("\""));

            for (size_t i = 0; i < data.GetNumberOfTranslations(); i++)
            {
                dummy = FormatStringForFile(data.GetTranslation(i));
                wxString hdr = wxString::Format(_T("msgstr[%u] \""), i);
                SaveMultiLines(f, hdr + dummy + _T("\""));
            }
        }
        else
        {
            dummy = FormatStringForFile(data.GetTranslation());
            SaveMultiLines(f, _T("msgstr \"") + dummy + _T("\""));
        }
        f.AddLine(wxEmptyString);
    }

    // Write back deleted items in the file so that they're not lost
    for (unsigned i = 0; i < m_deletedItems.size(); i++)
    {
        if ( i != 0 )
            f.AddLine(wxEmptyString);

        CatalogDeletedData& deletedItem = m_deletedItems[i];
        deletedItem.SetLineNumber(f.GetLineCount()+1);
        SaveMultiLines(f, deletedItem.GetComment());
        for (unsigned i = 0; i < deletedItem.GetAutoComments().GetCount(); i++)
            f.AddLine(_T("#. ") + deletedItem.GetAutoComments()[i]);
        for (unsigned i = 0; i < deletedItem.GetRawReferences().GetCount(); i++)
            f.AddLine(_T("#: ") + deletedItem.GetRawReferences()[i]);
        wxString dummy = deletedItem.GetFlags();
        if (!dummy.empty())
            f.AddLine(dummy);

        for (size_t j = 0; j < deletedItem.GetDeletedLines().GetCount(); j++)
            f.AddLine(deletedItem.GetDeletedLines()[j]);
    }

    if (!CanEncodeToCharset(f, m_header.Charset))
    {
        wxString msg;
        msg.Printf(_("The catalog couldn't be saved in '%s' charset as specified in catalog settings.\n\nIt was saved in UTF-8 instead and the setting was modified accordingly."),
                   m_header.Charset.c_str());
        wxMessageBox(msg, _("Error saving catalog"),
                     wxOK | wxICON_EXCLAMATION);
        m_header.Charset = _T("UTF-8");

        // Re-do the save again because we modified a header:
        f.Clear();
        return DoSaveOnly(f, crlf);
    }

    // Otherwise everything can be safely saved:
    return f.Write(crlf, wxCSConv(m_header.Charset));
}


int Catalog::Validate()
{
    TempDirectory tmpdir;
    if ( !tmpdir.IsOk() )
        return 0;

    wxString tmp_po = tmpdir.CreateFileName(_T("validated.po"));
    if ( !DoSaveOnly(tmp_po) )
        return 0;

    return DoValidate(tmp_po);
}

int Catalog::DoValidate(const wxString& po_file)
{
    GettextErrors err;
    ExecuteGettextAndParseOutput
    (
        wxString::Format(_T("msgfmt -o /dev/null -c \"%s\""), po_file.c_str()),
        err
    );

    for ( CatalogItemArray::iterator i = m_items.begin();
          i != m_items.end(); ++i )
    {
        i->SetValidity(CatalogItem::Val_Valid);
    }

    for ( GettextErrors::const_iterator i = err.begin(); i != err.end(); ++i )
    {
        if ( i->line != -1 )
        {
            CatalogItem *item = FindItemByLine(i->line);
            if ( item )
            {
                item->SetValidity(CatalogItem::Val_Invalid);
                item->SetErrorString(i->text);
                continue;
            }
        }
        // if not matched to an item:
        wxLogError(i->text);
    }

    return err.size();
}


bool Catalog::Update(ProgressInfo *progress, bool summary)
{
    if (!m_isOk) return false;

    wxString cwd = wxGetCwd();
    if (m_fileName != wxEmptyString)
    {
        wxString path;

        if (wxIsAbsolutePath(m_header.BasePath))
        {
            path = m_header.BasePath;
        }
        else
        {
            path = wxPathOnly(m_fileName);
            if (path.empty())
                path = _T(".");
            path = path + _T("/") + m_header.BasePath;
        }

        if (!wxIsAbsolutePath(path))
            path = cwd + _T("/") + path;

        if (!wxFileName::DirExists(path))
        {
            wxLogError(_("Source code directory '%s' doesn't exist."), path.c_str());
            return false;
        }

        wxSetWorkingDirectory(path);
    }

    SourceDigger dig(progress);

    wxArrayString keywords;
    if (m_header.Keywords.empty())
    {
        // NB: keep in sync with Catalog::CreateNewHeader!
        keywords.Add(_T("_"));
        keywords.Add(_T("gettext"));
        keywords.Add(_T("gettext_noop"));
    }
    else
    {
        WX_APPEND_ARRAY(keywords, m_header.Keywords);
    }

    Catalog *newcat = dig.Dig(m_header.SearchPaths,
                              keywords,
                              m_header.SourceCodeCharset);

    if (newcat != NULL)
    {
        bool succ = false;
        if ( progress )
            progress->UpdateMessage(_("Merging differences..."));

        if (!summary || ShowMergeSummary(newcat))
            succ = Merge(newcat);
        if (!succ)
        {
            delete newcat;
            newcat = NULL;
        }
    }

    wxSetWorkingDirectory(cwd);

    if (newcat == NULL) return false;

    delete newcat;
    return true;
}


bool Catalog::UpdateFromPOT(const wxString& pot_file, bool summary,
                            bool replace_header)
{
    if (!m_isOk) return false;

    Catalog newcat(pot_file);

    if (!newcat.IsOk())
    {
        wxLogError(_("'%s' is not a valid POT file."), pot_file.c_str());
        return false;
    }

    if (!summary || ShowMergeSummary(&newcat))
    {
        if ( !Merge(&newcat) )
            return false;
        if ( replace_header )
            CreateNewHeader(newcat.Header());
        return true;
    }
    else
    {
        return false;
    }
}


bool Catalog::Merge(Catalog *refcat)
{
    wxString oldname = m_fileName;

    TempDirectory tmpdir;
    if ( !tmpdir.IsOk() )
        return false;

    wxString tmp1 = tmpdir.CreateFileName(_T("ref.pot"));
    wxString tmp2 = tmpdir.CreateFileName(_T("input.po"));
    wxString tmp3 = tmpdir.CreateFileName(_T("output.po"));

    refcat->DoSaveOnly(tmp1);
    DoSaveOnly(tmp2);

    bool succ = ExecuteGettext
                (
                    wxString::Format
                    (
                        _T("msgmerge -q --force-po -o \"%s\" \"%s\" \"%s\""),
                        tmp3.c_str(), tmp2.c_str(), tmp1.c_str()
                    )
                );

    if (succ)
    {
        const wxString charset = m_header.Charset;

        Load(tmp3);

        // msgmerge doesn't always preserve the charset, it tends to pick
        // the most generic one of the charsets used, so if we are merging with
        // UTF-8 catalog, it will become UTF-8. Some people hate this.
        m_header.Charset = charset;
    }

    m_fileName = oldname;

    return succ;
}


void Catalog::GetMergeSummary(Catalog *refcat,
                              wxArrayString& snew, wxArrayString& sobsolete)
{
    wxASSERT( snew.empty() );
    wxASSERT( sobsolete.empty() );

    std::set<wxString> strsThis, strsRef;

    for ( unsigned i = 0; i < GetCount(); i++ )
        strsThis.insert((*this)[i].GetString());
    for ( unsigned i = 0; i < refcat->GetCount(); i++ )
        strsRef.insert((*refcat)[i].GetString());

    unsigned i;

    for (i = 0; i < GetCount(); i++)
    {
        if (strsRef.find((*this)[i].GetString()) == strsRef.end())
            sobsolete.Add((*this)[i].GetString());
    }

    for (i = 0; i < refcat->GetCount(); i++)
    {
        if (strsThis.find((*refcat)[i].GetString()) == strsThis.end())
            snew.Add((*refcat)[i].GetString());
    }
}

bool Catalog::ShowMergeSummary(Catalog *refcat)
{
    if (wxConfig::Get()->Read(_T("show_summary"), true))
    {
        wxArrayString snew, sobsolete;
        GetMergeSummary(refcat, snew, sobsolete);
        MergeSummaryDialog sdlg;
        sdlg.TransferTo(snew, sobsolete);
        return (sdlg.ShowModal() == wxID_OK);
    }
    else
        return true;
}

static unsigned GetCountFromPluralFormsHeader(const Catalog::HeaderData& header)
{
    if ( header.HasHeader(_T("Plural-Forms")) )
    {
        // e.g. "Plural-Forms: nplurals=3; plural=(n%10==1 && n%100!=11 ?
        //       0 : n%10>=2 && n%10<=4 && (n%100<10 || n%100>=20) ? 1 : 2);\n"

        wxString form = header.GetHeader(_T("Plural-Forms"));
        form = form.BeforeFirst(_T(';'));
        if (form.BeforeFirst(_T('=')) == _T("nplurals"))
        {
            long val;
            if (form.AfterFirst(_T('=')).ToLong(&val))
                return val;
        }
    }

    return 0;
}

unsigned Catalog::GetPluralFormsCount() const
{
    unsigned count = GetCountFromPluralFormsHeader(m_header);

    for ( CatalogItemArray::const_iterator i = m_items.begin();
          i != m_items.end(); ++i )
    {
        count = std::max(count, i->GetPluralFormsCount());
    }

    return count;
}

bool Catalog::HasWrongPluralFormsCount() const
{
    unsigned count = 0;

    for ( CatalogItemArray::const_iterator i = m_items.begin();
          i != m_items.end(); ++i )
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
    for ( CatalogItemArray::const_iterator i = m_items.begin();
          i != m_items.end(); ++i )
    {
        if ( i->HasPlural() )
            return true;
    }

    return false;
}


void Catalog::GetStatistics(int *all, int *fuzzy, int *badtokens,
                            int *untranslated, int *unfinished)
{
    if (all) *all = 0;
    if (fuzzy) *fuzzy = 0;
    if (badtokens) *badtokens = 0;
    if (untranslated) *untranslated = 0;
    if (unfinished) *unfinished = 0;

    for (size_t i = 0; i < GetCount(); i++)
    {
        bool ok = true;

        if (all)
            (*all)++;

        if ((*this)[i].IsFuzzy())
        {
            (*fuzzy)++;
            ok = false;
        }
        if ((*this)[i].GetValidity() == CatalogItem::Val_Invalid)
        {
            (*badtokens)++;
            ok = false;
        }
        if (!(*this)[i].IsTranslated())
        {
            (*untranslated)++;
            ok = false;
        }

        if ( !ok && unfinished )
            (*unfinished)++;
    }
}


void CatalogItem::SetFlags(const wxString& flags)
{
    m_isFuzzy = false;
    m_moreFlags.Empty();

    if (flags.empty()) return;
    wxStringTokenizer tkn(flags.Mid(1), _T(" ,"), wxTOKEN_STRTOK);
    wxString s;
    while (tkn.HasMoreTokens())
    {
        s = tkn.GetNextToken();
        if (s == _T("fuzzy")) m_isFuzzy = true;
        else m_moreFlags << _T(", ") << s;
    }
}


wxString CatalogItem::GetFlags() const
{
    wxString f;
    if (m_isFuzzy) f << _T(", fuzzy");
    f << m_moreFlags;
    if (!f.empty())
        return _T("#") + f;
    else
        return wxEmptyString;
}

bool CatalogItem::IsInFormat(const wxString& format)
{
    wxString lookingFor;
    lookingFor.Printf(_T("%s-format"), format.c_str());
    wxStringTokenizer tkn(m_moreFlags, _T(" ,"), wxTOKEN_STRTOK);
    while (tkn.HasMoreTokens())
    {
        if (tkn.GetNextToken() == lookingFor)
            return true;
    }
    return false;
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

    m_validity = Val_Unknown;

    m_isTranslated = true;
    for (size_t i = 0; i < m_translations.GetCount(); i++)
    {
        if (m_translations[i].empty())
        {
            m_isTranslated = false;
            break;
        }
    }
}

void CatalogItem::SetTranslations(const wxArrayString &t)
{
    m_translations = t;

    m_validity = Val_Unknown;

    m_isTranslated = true;
    for (size_t i = 0; i < m_translations.GetCount(); i++)
    {
        if (m_translations[i].empty())
        {
            m_isTranslated = false;
            break;
        }
    }
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
            size_t i = 0;
            while (i < line.length() && line[i] != _T(':')) { i++; }
            while (i < line.length() && !wxIsspace(line[i])) { i++; }

            refs.push_back(line.Left(i));
            line = line.Mid(i).Strip(wxString::both);
        }
    }

    return refs;
}

static wxString TryIfStringIsLangCode(const wxString& s)
{
    if (s.length() == 2)
    {
        if (IsKnownLanguageCode(s))
            return s;
    }
    else if (s.length() == 5 && s[2u] == _T('_'))
    {
        if (IsKnownLanguageCode(s.Mid(0, 2)) &&
            IsKnownCountryCode(s.Mid(3, 2)))
        {
            return s;
        }
    }

    return wxEmptyString;
}

wxString Catalog::GetLocaleCode() const
{
    wxString lang;

    // was the language explicitly specified?
    if ( !m_header.LanguageCode.empty() )
        lang = m_header.LanguageCode;

    // if not, can we deduce it from filename?
    if (lang.empty() && !m_fileName.empty())
    {
        wxString name;
        wxFileName::SplitPath(m_fileName, NULL, &name, NULL);

        lang = TryIfStringIsLangCode(name);
        if ( lang.empty() )
        {
            wxString afterDot = name.AfterLast('.');
            if ( afterDot != name )
                lang = TryIfStringIsLangCode(afterDot);
        }
    }

    wxLogTrace(_T("poedit"), _T("catalog lang is '%s'"), lang.c_str());

    return lang;
}
