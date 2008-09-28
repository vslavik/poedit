/*
 *  This file is part of Poedit (http://www.poedit.net)
 *
 *  Copyright (C) 1999-2008 Vaclav Slavik
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
 *  $Id$
 *
 *  Translations catalog
 *
 */

#include <wx/wxprec.h>

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
// and don't touch output
static bool ReadParam(const wxString& input, const wxString& pattern, wxString& output)
{
    if (input.Length() < pattern.Length()) return false;

    for (unsigned i = 0; i < pattern.Length(); i++)
        if (input[i] != pattern[i]) return false;
    output = input.Mid(pattern.Length()).Strip(wxString::trailing);
    return true;
}

static bool ReadParamIfNotSet(const wxString& input,
                              const wxString& pattern, wxString& output)
{
    wxString dummy;
    if (ReadParam(input, pattern, dummy))
    {
        if (output.empty())
            output = dummy;
        return true;
    }
    return false;
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


    // Set extended information:

    SetHeaderNotEmpty(_T("X-Poedit-Language"), Language);
    SetHeaderNotEmpty(_T("X-Poedit-Country"), Country);
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


    // Parse extended information:

    Language = GetHeader(_T("X-Poedit-Language"));
    Country = GetHeader(_T("X-Poedit-Country"));
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
    unsigned mlinenum;

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
            // A line may contain several references, separated by white-space.
            // Each reference is in the form "path_name:line_number"
            // (path_name may contain spaces)
            dummy = dummy.Strip(wxString::both);
            while (!dummy.empty())
            {
                size_t i = 0;
                while (i < dummy.length() && dummy[i] != _T(':')) { i++; }
                while (i < dummy.length() && !wxIsspace(dummy[i])) { i++; }

                mrefs.Add(dummy.Left(i));
                dummy = dummy.Mid(i).Strip(wxString::both);
            }

            line = ReadTextLine(m_textFile);
        }

        // previous msgid value:
        else if (ReadParam(line, _T("#| "), dummy))
        {
            msgid_old.Add(dummy);
            line = ReadTextLine(m_textFile);
        }

        // msgctxt:
        else if (ReadParam(line, _T("msgctxt \""), dummy) ||
                 ReadParam(line, _T("msgctxt\t\""), dummy))
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
        else if (ReadParam(line, _T("msgid \""), dummy) ||
                 ReadParam(line, _T("msgid\t\""), dummy))
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
        else if (ReadParam(line, _T("msgid_plural \""), dummy) ||
                 ReadParam(line, _T("msgid_plural\t\""), dummy))
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
        else if (ReadParam(line, _T("msgstr \""), dummy) ||
                 ReadParam(line, _T("msgstr\t\""), dummy))
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

            if (!OnEntry(mstr, wxEmptyString, false,
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

            while (ReadParam(line, label + _T(" \""), dummy) ||
                   ReadParam(line, label + _T("\t\""), dummy))
            {
                wxString str = dummy.RemoveLast();

                while (!(line=ReadTextLine(m_textFile)).empty())
                {
                    if (line[0u] == _T('\t'))
                        line.Remove(0, 1);
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
            line = ReadTextLine(m_textFile);
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
                                const wxArrayString& references,
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


Catalog::Catalog(const wxString& po_file)
{
    m_isOk = Load(po_file);
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

    dt.Language = wxEmptyString;
    dt.Country = wxEmptyString;
    dt.Project = wxEmptyString;
    dt.Team = wxEmptyString;
    dt.TeamEmail = wxEmptyString;
    dt.Charset = _T("utf-8");
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


bool Catalog::Load(const wxString& po_file)
{
    wxTextFile f;

    Clear();
    m_isOk = false;
    m_fileName = po_file;
    m_header.BasePath = wxEmptyString;

    /* Load the .po file: */

    if (!f.Open(po_file, wxConvISO8859_1))
        return false;

    CharsetInfoFinder charsetFinder(&f);
    charsetFinder.Parse();
    m_header.Charset = charsetFinder.GetCharset();

    f.Close();
    wxCSConv encConv(m_header.Charset);
    if (!f.Open(po_file, encConv))
        return false;

    if (!VerifyFileCharset(f, po_file, m_header.Charset))
    {
        wxLogError(_("There were errors when loading the catalog. Some data may be missing or corrupted as the result."));
    }

    LoadParser parser(this, &f);
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

    /* Load extended information from .po.poedit file, if present:
       (NB: this is deprecated, poedit >= 1.3.0 stores the data in
            .po file's header as X-Poedit-Foo) */

    if (wxFileExists(po_file + _T(".poedit")) &&
        f.Open(po_file + _T(".poedit"), wxConvISO8859_1))
    {
        wxString dummy;
        // poedit header (optional, we should be able to read any catalog):
        f.GetFirstLine();
        if (ReadParam(ReadTextLine(&f),
                      _T("#. Number of items: "), dummy))
        {
            // not used anymore
        }
        ReadParamIfNotSet(ReadTextLine(&f),
                  _T("#. Language: "), m_header.Language);
        dummy = ReadTextLine(&f);
        if (ReadParamIfNotSet(dummy, _T("#. Country: "), m_header.Country))
            dummy = ReadTextLine(&f);
        if (ReadParamIfNotSet(dummy, _T("#. Basepath: "), m_header.BasePath))
            dummy = ReadTextLine(&f);
        ReadParamIfNotSet(dummy, _T("#. SourceCodeCharSet: "), m_header.SourceCodeCharset);

        if (ReadParam(ReadTextLine(&f), _T("#. Paths: "), dummy))
        {
            bool setPaths = m_header.SearchPaths.empty();
            long sz;
            dummy.ToLong(&sz);
            for (; sz > 0; sz--)
            {
                if (ReadParam(ReadTextLine(&f), _T("#.     "), dummy))
                {
                    if (setPaths)
                        m_header.SearchPaths.Add(dummy);
                }
            }
        }

        if (ReadParam(ReadTextLine(&f), _T("#. Keywords: "), dummy))
        {
            bool setKeyw = m_header.Keywords.empty();
            long sz;
            dummy.ToLong(&sz);
            for (; sz > 0; sz--)
            {
                if (ReadParam(ReadTextLine(&f), _T("#.     "), dummy))
                {
                    if (setKeyw)
                        m_header.Keywords.Add(dummy);
                }
            }
        }

        f.Close();
    }

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


static bool CanEncodeStringToCharset(const wxString& s, wxMBConv& conv)
{
    if (s.empty())
        return true;
    if (!s.mb_str(conv))
        return false;
    return true;
}

static bool CanEncodeToCharset(Catalog& catalog, const wxString& charset)
{
    if (charset == _T("utf-8") || charset == _T("UTF-8"))
        return true;

    wxCSConv conv(charset);

    catalog.Header().UpdateDict();
    const Catalog::HeaderData::Entries& hdr(catalog.Header().GetAllHeaders());

    for (Catalog::HeaderData::Entries::const_iterator i = hdr.begin();
            i != hdr.end(); i++)
    {
        if (!CanEncodeStringToCharset(i->Value, conv))
            return false;
    }

    size_t cnt = catalog.GetCount();
    for (size_t i = 0; i < cnt; i++)
    {
        if (!CanEncodeStringToCharset(catalog[i].GetTranslation(), conv) ||
            !CanEncodeStringToCharset(catalog[i].GetString(), conv))
        {
            return false;
        }
    }
    return true;
}


static void GetCRLFBehaviour(wxTextFileType& type, bool& preserve)
{
    wxString format = wxConfigBase::Get()->Read(_T("crlf_format"), _T("unix"));

    if (format == _T("win")) type = wxTextFileType_Dos;
    else if (format == _T("mac")) type = wxTextFileType_Mac;
    else if (format == _T("native")) type = wxTextFile::typeDefault;
    else /* _T("unix") */ type = wxTextFileType_Unix;

    preserve = (bool)(wxConfigBase::Get()->Read(_T("keep_crlf"), true));
}

static void SaveMultiLines(wxTextFile &f, const wxString& text)
{
    wxStringTokenizer tkn(text, _T('\n'));
    while (tkn.HasMoreTokens())
        f.AddLine(tkn.GetNextToken());
}

/** Adds \n characters as neccesary for good-looking output
 */
static wxString FormatStringForFile(const wxString& text)
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

bool Catalog::Save(const wxString& po_file, bool save_mo)
{
    wxTextFileType crlfOrig, crlf;
    bool crlfPreserve;
    wxTextFile f;

    if ( wxFileExists(po_file) && !wxFile::Access(po_file, wxFile::write) )
    {
        wxLogError(_("File '%s' is read-only and cannot be saved.\nPlease save it under different name."),
                   po_file.c_str());
        return false;
    }

    GetCRLFBehaviour(crlfOrig, crlfPreserve);

    // Update information about last modification time. But if the header
    // was empty previously, the author apparently doesn't want this header
    // set, so don't mess with it. See https://sourceforge.net/tracker/?func=detail&atid=389156&aid=1900298&group_id=27043
    // for motivation:
    if ( !m_header.RevisionDate.empty() )
        m_header.RevisionDate = GetCurrentTimeRFC822();

    /* Detect CRLF format: */

    if ( crlfPreserve && wxFileExists(po_file) &&
         f.Open(po_file, wxConvISO8859_1) )
    {
        {
        wxLogNull null;
        crlf = f.GuessType();
        }
        if (crlf == wxTextFileType_None || crlf == wxTextFile::typeDefault)
            crlf = crlfOrig;
        f.Close();
    }
    else
        crlf = crlfOrig;

    /* Save .po file: */

    wxString charset(m_header.Charset);
    if (!charset || charset == _T("CHARSET"))
        charset = _T("utf-8");

    if (!CanEncodeToCharset(*this, charset))
    {
        wxString msg;
        msg.Printf(_("The catalog couldn't be saved in '%s' charset as\nspecified in catalog settings. It was saved in UTF-8 instead\nand the setting was modified accordingly."), charset.c_str());
        wxMessageBox(msg, _("Error saving catalog"),
                     wxOK | wxICON_EXCLAMATION);
        charset = _T("utf-8");
    }
    m_header.Charset = charset;

    if (!wxFileExists(po_file) || !f.Open(po_file, wxConvISO8859_1))
        if (!f.Create(po_file))
            return false;
    for (int j = f.GetLineCount() - 1; j >= 0; j--)
        f.RemoveLine(j);


    wxCSConv encConv(charset);

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
        SaveMultiLines(f, data.GetComment());
        for (unsigned i = 0; i < data.GetAutoComments().GetCount(); i++)
        {
            if (data.GetAutoComments()[i].empty())
              f.AddLine(_T("#."));
            else
              f.AddLine(_T("#. ") + data.GetAutoComments()[i]);
        }
        for (unsigned i = 0; i < data.GetReferences().GetCount(); i++)
            f.AddLine(_T("#: ") + data.GetReferences()[i]);
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
        data.SetLineNumber(f.GetLineCount()+1);
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
        CatalogDeletedData& deletedItem = m_deletedItems[i];
        SaveMultiLines(f, deletedItem.GetComment());
        for (unsigned i = 0; i < deletedItem.GetAutoComments().GetCount(); i++)
            f.AddLine(_T("#. ") + deletedItem.GetAutoComments()[i]);
        for (unsigned i = 0; i < deletedItem.GetReferences().GetCount(); i++)
            f.AddLine(_T("#: ") + deletedItem.GetReferences()[i]);
        wxString dummy = deletedItem.GetFlags();
        if (!dummy.empty())
            f.AddLine(dummy);
        deletedItem.SetLineNumber(f.GetLineCount()+1);

        for (size_t j = 0; j < deletedItem.GetDeletedLines().GetCount(); j++)
            f.AddLine(deletedItem.GetDeletedLines()[j]);

        f.AddLine(wxEmptyString);
    }


    f.Write(crlf, encConv);
    f.Close();

    /* Poedit < 1.3.0 used to save additional info in .po.poedit file. It's
       not used anymore, so delete the file if it exists: */
    if (wxFileExists(po_file + _T(".poedit")))
    {
        wxRemoveFile(po_file + _T(".poedit"));
    }

    /* If the user wants it, compile .mo file right now: */

    if (save_mo && wxConfig::Get()->Read(_T("compile_mo"), (long)false))
    {
        const wxString mofile = po_file.BeforeLast(_T('.')) + _T(".mo");
        if ( !ExecuteGettext
              (
                  wxString::Format(_T("msgfmt -c -o \"%s\" \"%s\""),
                                   mofile.c_str(),
                                   po_file.c_str())
              ) )
        {
            wxLogWarning(_("There were errors while compiling the saved catalog into MO file."));
        }
    }

    m_fileName = po_file;

    return true;
}


bool Catalog::Update(bool summary)
{
    if (!m_isOk) return false;

    ProgressInfo pinfo;
    pinfo.SetTitle(_("Updating catalog..."));

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

        wxSetWorkingDirectory(path);
    }

    SourceDigger dig(&pinfo);

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
        pinfo.UpdateMessage(_("Merging differences..."));

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


bool Catalog::UpdateFromPOT(const wxString& pot_file, bool summary)
{
    if (!m_isOk) return false;

    Catalog newcat(pot_file);

    if (!newcat.IsOk())
    {
        wxLogError(_("'%s' is not a valid POT file."), pot_file.c_str());
        return false;
    }

    if (!summary || ShowMergeSummary(&newcat))
        return Merge(&newcat);
    else
        return false;
}


bool Catalog::Merge(Catalog *refcat)
{
    wxString oldname = m_fileName;

    wxString tmpdir = wxGetTempFileName(_T("poedit"));
    wxRemoveFile(tmpdir);
    if (!wxMkdir(tmpdir, 0700))
        return false;

    wxString tmp1 = tmpdir + wxFILE_SEP_PATH + _T("ref.pot");
    wxString tmp2 = tmpdir + wxFILE_SEP_PATH + _T("input.po");
    wxString tmp3 = tmpdir + wxFILE_SEP_PATH + _T("output.po");

    refcat->Save(tmp1, false);
    Save(tmp2, false);

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

    wxRemoveFile(tmp1);
    wxRemoveFile(tmp2);
    wxRemoveFile(tmp3);
    wxRmdir(tmpdir);

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

static unsigned ParsePluralFormsHeader(const Catalog::HeaderData& header)
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
    unsigned count = ParsePluralFormsHeader(m_header);

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
    if ( count > ParsePluralFormsHeader(m_header) )
        return true;

    return false;
}

const bool Catalog::HasPluralItems() const
{
    for ( CatalogItemArray::const_iterator i = m_items.begin();
          i != m_items.end(); ++i )
    {
        if ( i->HasPlural() )
            return true;
    }

    return false;
}


void Catalog::GetStatistics(int *all, int *fuzzy, int *badtokens, int *untranslated)
{
    if (all) *all = 0;
    if (fuzzy) *fuzzy = 0;
    if (badtokens) *badtokens = 0;
    if (untranslated) *untranslated = 0;
    for (size_t i = 0; i < GetCount(); i++)
    {
        if (all) (*all)++;
        if ((*this)[i].IsFuzzy())
            (*fuzzy)++;
        if ((*this)[i].GetValidity() == CatalogItem::Val_Invalid)
            (*badtokens)++;
        if (!(*this)[i].IsTranslated())
            (*untranslated)++;
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
    if (!m_header.Language.empty())
    {
        lang = LookupLanguageCode(m_header.Language.c_str());
        if (!m_header.Country.empty())
        {
            lang += _T('_');
            lang += LookupCountryCode(m_header.Country.c_str());
        }
    }

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
