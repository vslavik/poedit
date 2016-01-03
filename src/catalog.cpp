/*
 *  This file is part of Poedit (http://poedit.net)
 *
 *  Copyright (C) 1999-2015 Vaclav Slavik
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
#include <wx/stdpaths.h>
#include <wx/strconv.h>
#include <wx/memtext.h>
#include <wx/filename.h>

#include <set>
#include <algorithm>

#include "catalog.h"
#include "digger.h"
#include "gexecute.h"
#include "progressinfo.h"
#include "str_helpers.h"
#include "utility.h"
#include "version.h"
#include "language.h"

#ifdef __WXOSX__
#import <Foundation/Foundation.h>
#endif

// TODO: split into different file
#if wxUSE_GUI
    #include <wx/msgdlg.h>
    #include "summarydlg.h"
#endif

// ----------------------------------------------------------------------
// Textfile processing utilities:
// ----------------------------------------------------------------------

namespace
{

// If input begins with pattern, fill output with end of input (without
// pattern; strips trailing spaces) and return true.  Return false otherwise
// and don't touch output. Is permissive about whitespace in the input:
// a space (' ') in pattern will match any number of any whitespace characters
// on that position in input.
bool ReadParam(const wxString& input, const wxString& pattern, wxString& output)
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
        int linesCount = (int)f2.GetLineCount() - (int)f.GetLineCount();
        wxLogError(wxPLURAL("%i line of file '%s' was not loaded correctly.",
                            "%i lines of file '%s' were not loaded correctly.",
                            linesCount),
                   linesCount,
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
                _("Line %d of file '%s' is corrupted (not valid %s data)."),
                int(i), filename.c_str(), charset.c_str());
            ok = false;
        }
    }

    return ok;
}


wxTextFileType GetFileCRLFFormat(wxTextFile& po_file)
{
    wxLogNull null;
    auto crlf = po_file.GuessType();

    // Discard any unsupported setting. In particular, we ignore "Mac"
    // line endings, because the ancient OS 9 systems aren't used anymore,
    // OSX uses Unix ending *and* "Mac" endings break gettext tools. So if
    // we encounter a catalog with "Mac" line endings, we silently convert
    // it into Unix endings (i.e. the modern Mac).
    if (crlf == wxTextFileType_Mac)
        crlf = wxTextFileType_Unix;
    if (crlf != wxTextFileType_Dos && crlf != wxTextFileType_Unix)
        crlf = wxTextFileType_None;
    return crlf;
}

wxTextFileType GetDesiredCRLFFormat(wxTextFileType existingCRLF)
{
    if (existingCRLF != wxTextFileType_None && wxConfigBase::Get()->ReadBool("keep_crlf", true))
    {
        return existingCRLF;
    }
    else
    {
        wxString format = wxConfigBase::Get()->Read("crlf_format", "unix");
        if (format == "win")
            return wxTextFileType_Dos;
        else /* "unix" or obsolete settings */
            return wxTextFileType_Unix;
    }
}

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
            wxLogError(_("Malformed header: '%s'"), ln.c_str());
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

    if (TeamEmail.empty())
    {
        SetHeader("Language-Team", Team);
    }
    else
    {
        if (Team.empty())
            SetHeader("Language-Team", TeamEmail);
        else
            SetHeader("Language-Team", Team + " <" + TeamEmail + ">");
    }

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

    dummy = GetHeader("Language-Team");
    if (!dummy.empty())
    {
        wxStringTokenizer tkn(dummy, "<>");
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

    wxString ctype = GetHeader("Content-Type");
    int charsetPos = ctype.Find("; charset=");
    if (charsetPos != -1)
    {
        Charset =
            ctype.Mid(charsetPos + strlen("; charset=")).Strip(wxString::both);
    }
    else
    {
        Charset = "iso-8859-1";
    }

    // Parse language information, with backwards compatibility with X-Poedit-*:
    wxString languageCode = GetHeader("Language");
    if ( !languageCode.empty() )
    {
        Lang = Language::TryParse(languageCode.ToStdWstring());
    }
    else
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
// Parsers
// ----------------------------------------------------------------------

bool CatalogParser::Parse()
{
    if (m_textFile->GetLineCount() == 0)
        return false;

    wxString line, dummy;
    wxString mflags, mstr, msgid_plural, mcomment;
    wxArrayString mrefs, mextractedcomments, mtranslations;
    wxArrayString msgid_old;
    bool has_plural = false;
    bool has_context = false;
    wxString msgctxt;
    unsigned mlinenum = 0;

    line = m_textFile->GetFirstLine();
    if (line.empty()) line = ReadTextLine();

    while (!line.empty())
    {
        // ignore empty special tags (except for extracted comments which we
        // DO want to preserve):
        while (line == "#," || line == "#:" || line == "#|")
            line = ReadTextLine();

        // flags:
        // Can't we have more than one flag, now only the last is kept ...
        if (ReadParam(line, "#, ", dummy))
        {
            mflags = "#, " + dummy;
            line = ReadTextLine();
        }

        // auto comments:
        if (ReadParam(line, "#. ", dummy) || ReadParam(line, "#.", dummy)) // second one to account for empty auto comments
        {
            mextractedcomments.Add(dummy);
            line = ReadTextLine();
        }

        // references:
        else if (ReadParam(line, "#: ", dummy))
        {
            // Just store the references unmodified, we don't modify this
            // data anywhere.
            mrefs.push_back(dummy);
            line = ReadTextLine();
        }

        // previous msgid value:
        else if (ReadParam(line, "#| ", dummy))
        {
            msgid_old.Add(dummy);
            line = ReadTextLine();
        }

        // msgctxt:
        else if (ReadParam(line, _T("msgctxt \""), dummy))
        {
            has_context = true;
            msgctxt = UnescapeCString(dummy.RemoveLast());
            while (!(line = ReadTextLine()).empty())
            {
                if (line[0u] == _T('\t'))
                    line.Remove(0, 1);
                if (line[0u] == _T('"') && line.Last() == _T('"'))
                {
                    msgctxt += UnescapeCString(line.Mid(1, line.Length() - 2));
                    PossibleWrappedLine();
                }
                else
                    break;
            }
        }

        // msgid:
        else if (ReadParam(line, _T("msgid \""), dummy))
        {
            mstr = UnescapeCString(dummy.RemoveLast());
            mlinenum = unsigned(m_textFile->GetCurrentLine() + 1);
            while (!(line = ReadTextLine()).empty())
            {
                if (line[0u] == _T('\t'))
                    line.Remove(0, 1);
                if (line[0u] == _T('"') && line.Last() == _T('"'))
                {
                    mstr += UnescapeCString(line.Mid(1, line.Length() - 2));
                    PossibleWrappedLine();
                }
                else
                    break;
            }
        }

        // msgid_plural:
        else if (ReadParam(line, _T("msgid_plural \""), dummy))
        {
            msgid_plural = UnescapeCString(dummy.RemoveLast());
            has_plural = true;
            mlinenum = unsigned(m_textFile->GetCurrentLine() + 1);
            while (!(line = ReadTextLine()).empty())
            {
                if (line[0u] == _T('\t'))
                    line.Remove(0, 1);
                if (line[0u] == _T('"') && line.Last() == _T('"'))
                {
                    msgid_plural += UnescapeCString(line.Mid(1, line.Length() - 2));
                    PossibleWrappedLine();
                }
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

            wxString str = UnescapeCString(dummy.RemoveLast());
            while (!(line = ReadTextLine()).empty())
            {
                if (line[0u] == _T('\t'))
                    line.Remove(0, 1);
                if (line[0u] == _T('"') && line.Last() == _T('"'))
                {
                    str += UnescapeCString(line.Mid(1, line.Length() - 2));
                    PossibleWrappedLine();
                }
                else
                    break;
            }
            mtranslations.Add(str);

            bool shouldIgnore = m_ignoreHeader && (mstr.empty() && !has_context);
            if ( shouldIgnore )
            {
                OnIgnoredEntry();
            }
            else
            {
                if (!mstr.empty() && m_ignoreTranslations)
                    mtranslations.clear();

                if (!OnEntry(mstr, wxEmptyString, false,
                             has_context, msgctxt,
                             mtranslations,
                             mflags, mrefs, mcomment, mextractedcomments, msgid_old,
                             mlinenum))
                {
                    return false;
                }
            }

            mcomment = mstr = msgid_plural = msgctxt = mflags = wxEmptyString;
            has_plural = has_context = false;
            mrefs.Clear();
            mextractedcomments.Clear();
            mtranslations.Clear();
            msgid_old.Clear();
        }

        // msgstr[i]:
        else if (ReadParam(line, "msgstr[", dummy))
        {
            if (!has_plural)
            {
                wxLogError(_("Broken catalog file: plural form msgstr used without msgid_plural"));
                return false;
            }

            wxString idx = dummy.BeforeFirst(_T(']'));
            wxString label = "msgstr[" + idx + "]";

            while (ReadParam(line, label + _T(" \""), dummy))
            {
                wxString str = UnescapeCString(dummy.RemoveLast());

                while (!(line=ReadTextLine()).empty())
                {
                    line.Trim(/*fromRight=*/false);
                    if (line[0u] == _T('"') && line.Last() == _T('"'))
                    {
                        str += UnescapeCString(line.Mid(1, line.Length() - 2));
                        PossibleWrappedLine();
                    }
                    else
                    {
                        if (ReadParam(line, "msgstr[", dummy))
                        {
                            idx = dummy.BeforeFirst(_T(']'));
                            label = "msgstr[" + idx + "]";
                        }
                        break;
                    }
                }
                mtranslations.Add(str);
            }

            if (!OnEntry(mstr, msgid_plural, true,
                         has_context, msgctxt,
                         mtranslations,
                         mflags, mrefs, mcomment, mextractedcomments, msgid_old,
                         mlinenum))
            {
                return false;
            }

            mcomment = mstr = msgid_plural = msgctxt = mflags = wxEmptyString;
            has_plural = has_context = false;
            mrefs.Clear();
            mextractedcomments.Clear();
            mtranslations.Clear();
            msgid_old.Clear();
        }

        // deleted lines:
        else if (ReadParam(line, "#~", dummy))
        {
            wxArrayString deletedLines;
            deletedLines.Add(line);
            mlinenum = unsigned(m_textFile->GetCurrentLine() + 1);
            while (!(line = ReadTextLine()).empty())
            {
                // if line does not start with "#~" anymore, stop reading
                if (!ReadParam(line, "#~", dummy))
                    break;
                // if the line starts with "#~ msgid", we skipped an empty line
                // and it's a new entry, so stop reading too (see bug #329)
                if (ReadParam(line, "#~ msgid", dummy))
                    break;

                deletedLines.Add(line);
            }
            if (!OnDeletedEntry(deletedLines,
                                mflags, mrefs, mcomment, mextractedcomments, mlinenum))
            {
                return false;
            }

            mcomment = mstr = msgid_plural = mflags = wxEmptyString;
            has_plural = false;
            mrefs.Clear();
            mextractedcomments.Clear();
            mtranslations.Clear();
            msgid_old.Clear();
        }

        // comment:
        else if (line[0u] == _T('#'))
        {
            bool readNewLine = false;

            while (!line.empty() &&
                    line[0u] == _T('#') &&
                   (line.Length() < 2 || (line[1u] != _T(',') && line[1u] != _T(':') && line[1u] != _T('.') && line[1u] != _T('~') )))
            {
                mcomment << line << _T('\n');
                readNewLine = true;
                line = ReadTextLine();
            }

            if (!readNewLine)
                line = ReadTextLine();
        }

        else
        {
            line = ReadTextLine();
        }
    }

    return true;
}


wxString CatalogParser::ReadTextLine()
{
    m_previousLineHardWrapped = m_lastLineHardWrapped;
    m_lastLineHardWrapped = false;

    wxString s;

    while (s.empty())
    {
        if (m_textFile->Eof())
            return wxEmptyString;

        // read next line and strip insignificant whitespace from it:
        auto ln = m_textFile->GetNextLine();

        // gettext tools don't include (extracted) comments in wrapping, so they can't
        // be reliably used to detect file's wrapping either; just skip them.
        if (!ln.StartsWith(wxS("#. ")) && !ln.StartsWith(wxS("# ")))
        {
            if (ln.EndsWith(wxS("\\n\"")))
            {
                // Similarly, lines ending with \n are always wrapped, so skip that too.
                m_lastLineHardWrapped = true;
            }
            else if (ln == "msgid \"\"" || ln == "msgstr \"\"")
            {
                // The header is always indented like this
                m_lastLineHardWrapped = true;
            }
            else
            {
                // Watch out for lines with too long words that couldn't be wrapped.
                // That "2" is to account for unwrappable comment lines: "#: somethinglong"
                // See https://github.com/vslavik/poedit/issues/135
                auto space = ln.find_last_of(' ');
                if (space != wxString::npos && space > 2)
                {
                    m_detectedLineWidth = std::max(m_detectedLineWidth, (int)ln.size());
                }
            }
        }

        s = ln.Strip(wxString::both);
    }

    return s;
}

int CatalogParser::GetWrappingWidth() const
{
    if (!m_detectedWrappedLines)
        return Catalog::NO_WRAPPING;

    return m_detectedLineWidth;
}



class CharsetInfoFinder : public CatalogParser
{
    public:
        CharsetInfoFinder(wxTextFile *f)
                : CatalogParser(f), m_charset("iso-8859-1") {}
        wxString GetCharset() const { return m_charset; }

    protected:
        wxString m_charset;

        virtual bool OnEntry(const wxString& msgid,
                             const wxString& /*msgid_plural*/,
                             bool /*has_plural*/,
                             bool has_context,
                             const wxString& /*context*/,
                             const wxArrayString& mtranslations,
                             const wxString& /*flags*/,
                             const wxArrayString& /*references*/,
                             const wxString& /*comment*/,
                             const wxArrayString& /*extractedComments*/,
                             const wxArrayString& /*msgid_old*/,
                             unsigned /*lineNumber*/)
        {
            if (msgid.empty() && !has_context)
            {
                // gettext header:
                Catalog::HeaderData hdr;
                hdr.FromString(mtranslations[0]);
                m_charset = hdr.Charset;
                if (m_charset == "CHARSET")
                    m_charset = "iso-8859-1";
                return false; // stop parsing
            }
            return true;
        }
};



class LoadParser : public CatalogParser
{
    public:
        LoadParser(Catalog& c, wxTextFile *f)
              : CatalogParser(f),
                FileIsValid(false),
                m_catalog(c), m_nextId(1), m_seenHeaderAlready(false) {}

        // true if the file is valid, i.e. has at least some data
        bool FileIsValid;

        Language GetMsgidLanguage()
        {
            auto utf8 = m_allMsgidText.utf8_str();
            return Language::TryDetectFromText(utf8.data(), utf8.length(), Language::English());
        }

    protected:
        Catalog& m_catalog;

        virtual bool OnEntry(const wxString& msgid,
                             const wxString& msgid_plural,
                             bool has_plural,
                             bool has_context,
                             const wxString& context,
                             const wxArrayString& mtranslations,
                             const wxString& flags,
                             const wxArrayString& references,
                             const wxString& comment,
                             const wxArrayString& extractedComments,
                             const wxArrayString& msgid_old,
                             unsigned lineNumber);

        virtual bool OnDeletedEntry(const wxArrayString& deletedLines,
                                    const wxString& flags,
                                    const wxArrayString& references,
                                    const wxString& comment,
                                    const wxArrayString& extractedComments,
                                    unsigned lineNumber);

        virtual void OnIgnoredEntry() { FileIsValid = true; }

    private:
        int m_nextId;
        bool m_seenHeaderAlready;

        // collected text of msgids, with newlines, for language detection
        wxString m_allMsgidText;
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
                         const wxArrayString& extractedComments,
                         const wxArrayString& msgid_old,
                         unsigned lineNumber)
{
    FileIsValid = true;

    static const wxString MSGCAT_CONFLICT_MARKER("#-#-#-#-#");

    if (msgid.empty() && !has_context)
    {
        if (!m_seenHeaderAlready)
        {
            // gettext header:
            m_catalog.m_header.FromString(mtranslations[0]);
            m_catalog.m_header.Comment = comment;
            m_seenHeaderAlready = true;
        }
        // else: ignore duplicate header in malformed files
    }
    else
    {
        CatalogItemPtr d = std::make_shared<CatalogItem>();
        d->SetId(m_nextId++);
        if (!flags.empty())
            d->SetFlags(flags);
        d->SetString(msgid);
        if (has_plural)
            d->SetPluralString(msgid_plural);
        if (has_context)
            d->SetContext(context);
        d->SetTranslations(mtranslations);
        d->SetComment(comment);
        d->SetLineNumber(lineNumber);
        for (size_t i = 0; i < references.GetCount(); i++)
            d->AddReference(references[i]);

        for (auto i: extractedComments)
        {
            // Sometimes, msgcat produces conflicts in extracted comments; see the gory details:
            // https://groups.google.com/d/topic/poedit/j41KuvXtVUU/discussion
            // As a workaround, just filter them out.
            // FIXME: Fix this properly... but not using msgcat in the first place
            if (i.StartsWith(MSGCAT_CONFLICT_MARKER) && i.EndsWith(MSGCAT_CONFLICT_MARKER))
                continue;
            d->AddExtractedComments(i);
        }
        d->SetOldMsgid(msgid_old);
        m_catalog.AddItem(d);

        // collect text for language detection:
        m_allMsgidText.append(msgid);
        m_allMsgidText.append('\n');
        if (!msgid_plural.empty())
        {
            m_allMsgidText.append(msgid_plural);
            m_allMsgidText.append('\n');
        }
    }
    return true;
}

bool LoadParser::OnDeletedEntry(const wxArrayString& deletedLines,
                                const wxString& flags,
                                const wxArrayString& /*references*/,
                                const wxString& comment,
                                const wxArrayString& extractedComments,
                                unsigned lineNumber)
{
    FileIsValid = true;

    CatalogDeletedData d;
    if (!flags.empty()) d.SetFlags(flags);
    d.SetDeletedLines(deletedLines);
    d.SetComment(comment);
    d.SetLineNumber(lineNumber);
    for (size_t i = 0; i < extractedComments.GetCount(); i++)
      d.AddExtractedComments(extractedComments[i]);
    m_catalog.AddDeletedItem(d);

    return true;
}



// ----------------------------------------------------------------------
// Catalog class
// ----------------------------------------------------------------------

Catalog::Catalog(Type type)
{
    m_sourceLanguage = Language::English();
    m_fileType = type;

    m_fileCRLF = wxTextFileType_None;
    m_fileWrappingWidth = DEFAULT_WRAPPING;

    m_isOk = true;
    m_header.BasePath = wxEmptyString;
    for(int i = BOOKMARK_0; i < BOOKMARK_LAST; i++)
    {
        m_header.Bookmarks[i] = -1;
    }
}

Catalog::Catalog(const wxString& po_file, int flags)
{
    m_sourceLanguage = Language::English();
    m_fileType = Type::PO;

    m_fileCRLF = wxTextFileType_None;
    m_fileWrappingWidth = DEFAULT_WRAPPING;

    m_isOk = Load(po_file, flags);
}

Catalog::~Catalog()
{
    Clear();
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
    dt.Team = wxEmptyString;
    dt.TeamEmail = wxEmptyString;
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
    if (dt.Team == "LANGUAGE")
        dt.Team.clear();
    if (dt.TeamEmail == "LL@li.org")
        dt.TeamEmail.clear();

    // translator should be pre-filled & not the default "FULL NAME <EMAIL@ADDRESS>"
    dt.DeleteHeader("Last-Translator");
    dt.Translator = wxConfig::Get()->Read("translator_name", wxEmptyString);
    dt.TranslatorEmail = wxConfig::Get()->Read("translator_email", wxEmptyString);

    dt.UpdateDict();
}


bool Catalog::Load(const wxString& po_file, int flags)
{
    wxTextFile f;

    Clear();
    m_isOk = false;
    m_fileName = po_file;
    m_header.BasePath = wxEmptyString;

    wxString ext;
    wxFileName::SplitPath(po_file, nullptr, nullptr, &ext);
    if (ext.CmpNoCase("pot") == 0)
        m_fileType = Type::POT;
    else
        m_fileType = Type::PO;

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

    LoadParser parser(*this, &f);
    parser.IgnoreHeader(flags & CreationFlag_IgnoreHeader);
    parser.IgnoreTranslations(flags & CreationFlag_IgnoreTranslations);
    if (!parser.Parse())
    {
        wxLogError(
            wxString::Format(
                _("Couldn't load file %s, it is probably corrupted."),
                po_file.c_str()));
        return false;
    }

    m_sourceLanguage = parser.GetMsgidLanguage();

    // now that the catalog is loaded, update its items with the bookmarks
    for (unsigned i = BOOKMARK_0; i < BOOKMARK_LAST; i++)
    {
        if (m_header.Bookmarks[i] != -1 &&
            m_header.Bookmarks[i] < (int)m_items.size())
        {
            m_items[m_header.Bookmarks[i]]->SetBookmark(
                    static_cast<Bookmark>(i));
        }
    }

    m_fileCRLF = GetFileCRLFFormat(f);
    m_fileWrappingWidth = parser.GetWrappingWidth();
    wxLogTrace("poedit", "detect line wrapping: %d", m_fileWrappingWidth);

    // If we didn't find any entries, the file must be invalid:
    if (!parser.FileIsValid)
        return false;

    m_isOk = true;

    f.Close();

    FixupCommonIssues();

    if ( flags & CreationFlag_IgnoreHeader )
        CreateNewHeader();

    return true;
}


void Catalog::FixupCommonIssues()
{
    if (m_header.Project == "PACKAGE VERSION")
        m_header.Project.clear();

    // All the following fixups are specific to POs and should *not* be done in POTs:
    if (m_fileType == Type::POT)
        return;

    if (!m_header.Lang.IsValid())
    {
        if (!m_fileName.empty())
        {
            m_header.Lang = Language::TryGuessFromFilename(m_fileName);
            wxLogTrace("poedit", "guessed language from filename '%s': %s", m_fileName, m_header.Lang.Code());
        }

        if (!m_header.Lang.IsValid())
        {
            // If all else fails, try to detect the language from content
            wxString allText;
            for (auto& i: items())
            {
                for (auto& s: i->GetTranslations())
                {
                    if (s.empty())
                        continue;
                    allText.append(s);
                    allText.append('\n');
                }
            }
            if (!allText.empty())
            {
                auto utf8 = allText.utf8_str();
                m_header.Lang = Language::TryDetectFromText(utf8.data(), utf8.length());
            }
        }
    }

    wxLogTrace("poedit", "catalog lang is '%s'", GetLanguage().Code());

    if (m_header.GetHeader("Language-Team") == "LANGUAGE <LL@li.org>")
    {
        m_header.DeleteHeader("Language-Team");
        m_header.Team.clear();
        m_header.TeamEmail.clear();
    }

    if (m_header.GetHeader("Last-Translator") == "FULL NAME <EMAIL@ADDRESS>")
    {
        m_header.DeleteHeader("Last-Translator");
        m_header.Translator.clear();
        m_header.TranslatorEmail.clear();
    }

    wxString pluralForms = m_header.GetHeader("Plural-Forms");

    if (pluralForms == "nplurals=INTEGER; plural=EXPRESSION;") // default invalid value
        pluralForms = "";

    if (!pluralForms.empty())
    {
        if (!pluralForms.EndsWith(";"))
        {
            pluralForms += ";";
            m_header.SetHeader("Plural-Forms", pluralForms);
        }
    }
    else
    {
        // Auto-fill default plural form if it is missing:
        if (m_header.Lang.IsValid() && HasPluralItems())
        {
            pluralForms = m_header.Lang.DefaultPluralFormsExpr();
            if (!pluralForms.empty())
                m_header.SetHeader("Plural-Forms", pluralForms);
        }
    }

    // TODO: mark catalog as modified if any changes were made
}

void Catalog::AddItem(const CatalogItemPtr& data)
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

CatalogItemPtr Catalog::FindItemByLine(int lineno)
{
    CatalogItemPtr last;

    for (auto& i: m_items)
    {
        if ( i->GetLineNumber() > lineno )
            return last;
        last = i;
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


// misc file-saving helpers
namespace
{

inline bool CanEncodeStringToCharset(const wxString& s, wxMBConv& conv)
{
    if (s.empty())
        return true;
    const wxCharBuffer converted(s.mb_str(conv));
    if ( converted.length() == 0 )
        return false;
    return true;
}

bool CanEncodeToCharset(const wxTextBuffer& f, const wxString& charset)
{
    if (charset.Lower() == "utf-8" || charset.Lower() == "utf8")
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


void SaveMultiLines(wxTextBuffer &f, const wxString& text)
{
    wxStringTokenizer tkn(text, _T('\n'));
    while (tkn.HasMoreTokens())
        f.AddLine(tkn.GetNextToken());
}

/** Adds \n characters as necessary for good-looking output
 */
wxString FormatStringForFile(const wxString& text)
{
    wxString s;
    s.reserve(text.length() + 16);

    wxStringTokenizer tkn(text, wxS('\n'), wxTOKEN_RET_EMPTY_ALL);
    while (tkn.HasMoreTokens())
    {
        if (!s.empty())
            s += wxS("\"\n\"");
        auto piece = tkn.GetNextToken();
        if (tkn.GetLastDelimiter())
            piece += tkn.GetLastDelimiter();
        s += EscapeCString(piece);
    }

    return s;
}

} // anonymous namespace


#ifdef __WXOSX__

@interface CompiledMOFilePresenter : NSObject<NSFilePresenter>
@property (atomic, copy) NSURL *presentedItemURL;
@property (atomic, copy) NSURL *primaryPresentedItemURL;
@end

@implementation CompiledMOFilePresenter
- (NSOperationQueue *)presentedItemOperationQueue {
     return [NSOperationQueue mainQueue];
}
@end

#endif // __WXOSX__


bool Catalog::Save(const wxString& po_file, bool save_mo,
                   int& validation_errors, CompilationStatus& mo_compilation_status)
{
    mo_compilation_status = CompilationStatus::NotDone;

    if ( wxFileExists(po_file) && !wxFile::Access(po_file, wxFile::write) )
    {
        wxLogError(_("File '%s' is read-only and cannot be saved.\nPlease save it under different name."),
                   po_file.c_str());
        return false;
    }

    TempOutputFileFor po_file_temp_obj(po_file);
    const wxString po_file_temp = po_file_temp_obj.FileName();

    wxTextFileType outputCrlf = GetDesiredCRLFFormat(m_fileCRLF);
    // Save into Unix line endings first and only if Windows is required,
    // reformat the file later. This is because msgcat cannot handle DOS
    // input particularly well.

    if ( !DoSaveOnly(po_file_temp, wxTextFileType_Unix) )
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
        int wrapping = DEFAULT_WRAPPING;
        if (wxConfig::Get()->ReadBool("keep_crlf", true))
            wrapping = m_fileWrappingWidth;

        wxString wrappingFlag;
        if (wrapping == DEFAULT_WRAPPING)
        {
            if (wxConfig::Get()->ReadBool("wrap_po_files", true))
            {
                wrapping = (int)wxConfig::Get()->ReadLong("wrap_po_files_width", 79);
            }
            else
            {
                wrapping = NO_WRAPPING;
            }
        }

        if (wrapping == NO_WRAPPING)
            wrappingFlag = " --no-wrap";
        else if (wrapping != DEFAULT_WRAPPING)
            wrappingFlag.Printf(" --width=%d", wrapping);

        TempOutputFileFor po_file_temp2_obj(po_file_temp);
        const wxString po_file_temp2 = po_file_temp2_obj.FileName();
        auto msgcatCmd = wxString::Format("msgcat --force-po%s -o %s %s",
                                          wrappingFlag,
                                          QuoteCmdlineArg(po_file_temp2),
                                          QuoteCmdlineArg(po_file_temp));
        wxLogTrace("poedit", "formatting file with %s", msgcatCmd);

        // Ignore msgcat errors output (but not exit code), because it
        //   a) complains about things DoValidate() already complained above
        //   b) issues warnings about source-extraction things (e.g. using non-ASCII
        //      msgids) that, while correct, are not something a *translator* can
        //      do anything about.
        wxLogNull null;
        msgcat_ok = ExecuteGettext(msgcatCmd) &&
                    wxFileExists(po_file_temp2);

        // msgcat always outputs Unix line endings, so we need to reformat the file
        if (msgcat_ok && outputCrlf == wxTextFileType_Dos)
        {
            wxTextFile finalFile(po_file_temp2);
            if (finalFile.Open())
                finalFile.Write(outputCrlf);
        }

        if (!TempOutputFileFor::ReplaceFile(po_file_temp2, po_file))
            msgcat_ok = false;
    }

    if ( msgcat_ok )
    {
        wxRemoveFile(po_file_temp);
    }
    else
    {
        if ( !po_file_temp_obj.Commit() )
        {
            wxLogError(_("Couldn't save file %s."), po_file.c_str());
        }
        else
        {
            // Only shows msgcat's failure warning if we don't also get
            // validation errors, because if we do, the cause is likely the
            // same.
            if ( !validation_errors )
            {
                wxLogWarning(_("There was a problem formatting the file nicely (but it was saved all right)."));
            }
        }
    }

    
    /* If the user wants it, compile .mo file right now: */

    if (m_fileType == Type::PO && save_mo && wxConfig::Get()->Read("compile_mo", (long)true))
    {
        const wxString mo_file = wxFileName::StripExtension(po_file) + ".mo";
        TempOutputFileFor mo_file_temp_obj(mo_file);
        const wxString mo_file_temp = mo_file_temp_obj.FileName();

        {
            // Ignore msgfmt errors output (but not exit code), because it
            // complains about things DoValidate() already complained above.
            wxLogNull null;

            if ( ExecuteGettext
                  (
                      wxString::Format("msgfmt -o %s %s",
                                       QuoteCmdlineArg(mo_file_temp),
                                       QuoteCmdlineArg(CliSafeFileName(po_file)))
                  ) )
            {
                mo_compilation_status = CompilationStatus::Success;
            }
            else
            {
                // Don't report errors, they were reported as part of validation
                // step above.  Notice that we run msgfmt *without* the -c flag
                // here to create the MO file in as many cases as possible, even if
                // it has some errors.
                //
                // Still, msgfmt has the ugly habit of sometimes returning non-zero
                // exit code, reporting "fatal errors" and *still* producing a usable
                // .mo file. If this happens, don't pretend the file wasn't created.
                if (wxFileName::FileExists(mo_file_temp))
                    mo_compilation_status = CompilationStatus::Success;
                else
                    mo_compilation_status = CompilationStatus::Error;
            }
        }

        // Move the MO from temporary location to the final one, if it was created
        if (mo_compilation_status == CompilationStatus::Success)
        {
#ifdef __WXOSX__
            NSURL *mofileUrl = [NSURL fileURLWithPath:str::to_NS(mo_file)];
            NSURL *mofiletempUrl = [NSURL fileURLWithPath:str::to_NS(mo_file_temp)];
            bool sandboxed = (getenv("APP_SANDBOX_CONTAINER_ID") != NULL);
            CompiledMOFilePresenter *presenter = nil;
            if (sandboxed)
            {
                presenter = [CompiledMOFilePresenter new];
                presenter.presentedItemURL = mofileUrl;
                presenter.primaryPresentedItemURL = [NSURL fileURLWithPath:str::to_NS(po_file)];
                [NSFileCoordinator addFilePresenter:presenter];
                [NSFileCoordinator filePresenters];
            }
            NSFileCoordinator *coo = [[NSFileCoordinator alloc] initWithFilePresenter:presenter];
            [coo coordinateWritingItemAtURL:mofileUrl options:NSFileCoordinatorWritingForReplacing error:nil byAccessor:^(NSURL *newURL) {
                NSURL *resultingUrl;
                BOOL ok = [[NSFileManager defaultManager] replaceItemAtURL:newURL
                                                             withItemAtURL:mofiletempUrl
                                                            backupItemName:nil
                                                                   options:0
                                                          resultingItemURL:&resultingUrl
                                                                     error:nil];
                if (!ok)
                {
                    wxLogError(_("Couldn't save file %s."), mo_file.c_str());
                    mo_compilation_status = CompilationStatus::Error;
                }
            }];
            if (sandboxed)
            {
                [NSFileCoordinator removeFilePresenter:presenter];
            }
#else // !__WXOSX__
            if ( !mo_file_temp_obj.Commit() )
            {
                wxLogError(_("Couldn't save file %s."), mo_file.c_str());
                mo_compilation_status = CompilationStatus::Error;
            }
#endif // __WXOSX__/!__WXOSX__
        }
    }

    m_fileName = po_file;

    return true;
}


std::string Catalog::SaveToBuffer()
{
    class StringSerializer : public wxMemoryText
    {
    public:
        bool OnWrite(wxTextFileType typeNew, const wxMBConv& conv) override
        {
            size_t cnt = GetLineCount();
            for (size_t n = 0; n < cnt; n++)
            {
                auto ln = GetLine(n) +
                          GetEOL(typeNew == wxTextFileType_None ? GetLineType(n) : typeNew);
                auto buf = ln.mb_str(conv);
                buffer.append(buf.data(), buf.length());
            }
            return true;
        }

        std::string buffer;
    };

    StringSerializer f;

    if (!DoSaveOnly(f, wxTextFileType_Unix))
        return std::string();
    return f.buffer;
}


bool Catalog::CompileToMO(const wxString& mo_file,
                          int& validation_errors,
                          CompilationStatus& mo_compilation_status)
{
    mo_compilation_status = CompilationStatus::NotDone;

    TempDirectory tmpdir;
    if ( !tmpdir.IsOk() )
        return false;
    wxString po_file_temp = tmpdir.CreateFileName("output.po");

    if ( !DoSaveOnly(po_file_temp, wxTextFileType_Unix) )
    {
        wxLogError(_("Couldn't save file %s."), po_file_temp.c_str());
        return false;
    }

    validation_errors = DoValidate(po_file_temp);

    TempOutputFileFor mo_file_temp_obj(mo_file);
    const wxString mo_file_temp = mo_file_temp_obj.FileName();

    {
        // Ignore msgfmt errors output (but not exit code), because it
        // complains about things DoValidate() already complained above.
        wxLogNull null;
        ExecuteGettext(wxString::Format("msgfmt -o %s %s",
                                        QuoteCmdlineArg(mo_file_temp),
                                        QuoteCmdlineArg(po_file_temp)));
    }

    // Don't check return code:
    // msgfmt has the ugly habit of sometimes returning non-zero
    // exit code, reporting "fatal errors" and *still* producing a usable
    // .mo file. If this happens, don't pretend the file wasn't created.
    if (!wxFileName::FileExists(mo_file_temp))
    {
        mo_compilation_status = CompilationStatus::Error;
        return false;
    }
    else
    {
        mo_compilation_status = CompilationStatus::Success;
    }

    if ( !mo_file_temp_obj.Commit() )
    {
        wxLogError(_("Couldn't save file %s."), mo_file.c_str());
        return false;
    }

    return true;
}




bool Catalog::DoSaveOnly(const wxString& po_file, wxTextFileType crlf)
{
    wxTextFile f;
    if (!f.Create(po_file))
        return false;

    return DoSaveOnly(f, crlf);
}

bool Catalog::DoSaveOnly(wxTextBuffer& f, wxTextFileType crlf)
{
    /* Save .po file: */
    if (!m_header.Charset || m_header.Charset == "CHARSET")
        m_header.Charset = "UTF-8";

    // Update information about last modification time. But if the header
    // was empty previously, the author apparently doesn't want this header
    // set, so don't mess with it. See https://sourceforge.net/tracker/?func=detail&atid=389156&aid=1900298&group_id=27043
    // for motivation:
    auto currentTime = GetCurrentTimeString();
    switch (m_fileType)
    {
        case Type::PO:
            if ( !m_header.RevisionDate.empty() )
                m_header.RevisionDate = currentTime;
            break;
        case Type::POT:
            if ( m_fileType == Type::POT && !m_header.CreationDate.empty() )
                m_header.CreationDate = currentTime;
            break;
    }

    SaveMultiLines(f, m_header.Comment);
    if (m_fileType == Type::POT)
        f.AddLine("#, fuzzy");
    f.AddLine(_T("msgid \"\""));
    f.AddLine(_T("msgstr \"\""));
    wxString pohdr = wxString(_T("\"")) + m_header.ToString(_T("\"\n\""));
    pohdr.RemoveLast();
    SaveMultiLines(f, pohdr);
    f.AddLine(wxEmptyString);

    auto pluralsCount = GetPluralFormsCount();

    for (auto& data: m_items)
    {
        data->SetLineNumber(int(f.GetLineCount()+1));
        SaveMultiLines(f, data->GetComment());
        for (unsigned i = 0; i < data->GetExtractedComments().GetCount(); i++)
        {
            if (data->GetExtractedComments()[i].empty())
              f.AddLine("#.");
            else
              f.AddLine("#. " + data->GetExtractedComments()[i]);
        }
        for (unsigned i = 0; i < data->GetRawReferences().GetCount(); i++)
            f.AddLine("#: " + data->GetRawReferences()[i]);
        wxString dummy = data->GetFlags();
        if (!dummy.empty())
            f.AddLine(dummy);
        for (unsigned i = 0; i < data->GetOldMsgid().GetCount(); i++)
            f.AddLine("#| " + data->GetOldMsgid()[i]);
        if ( data->HasContext() )
        {
            SaveMultiLines(f, _T("msgctxt \"") + FormatStringForFile(data->GetContext()) + _T("\""));
        }
        dummy = FormatStringForFile(data->GetString());
        SaveMultiLines(f, _T("msgid \"") + dummy + _T("\""));
        if (data->HasPlural())
        {
            dummy = FormatStringForFile(data->GetPluralString());
            SaveMultiLines(f, _T("msgid_plural \"") + dummy + _T("\""));

            for (unsigned i = 0; i < pluralsCount; i++)
            {
                dummy = FormatStringForFile(data->GetTranslation(i));
                wxString hdr = wxString::Format(_T("msgstr[%u] \""), i);
                SaveMultiLines(f, hdr + dummy + _T("\""));
            }
        }
        else
        {
            dummy = FormatStringForFile(data->GetTranslation());
            SaveMultiLines(f, _T("msgstr \"") + dummy + _T("\""));
        }
        f.AddLine(wxEmptyString);
    }

    // Write back deleted items in the file so that they're not lost
    for (unsigned itemIdx = 0; itemIdx < m_deletedItems.size(); itemIdx++)
    {
        if ( itemIdx != 0 )
            f.AddLine(wxEmptyString);

        CatalogDeletedData& deletedItem = m_deletedItems[itemIdx];
        deletedItem.SetLineNumber(int(f.GetLineCount()+1));
        SaveMultiLines(f, deletedItem.GetComment());
        for (unsigned i = 0; i < deletedItem.GetExtractedComments().GetCount(); i++)
            f.AddLine("#. " + deletedItem.GetExtractedComments()[i]);
        for (unsigned i = 0; i < deletedItem.GetRawReferences().GetCount(); i++)
            f.AddLine("#: " + deletedItem.GetRawReferences()[i]);
        wxString dummy = deletedItem.GetFlags();
        if (!dummy.empty())
            f.AddLine(dummy);

        for (size_t j = 0; j < deletedItem.GetDeletedLines().GetCount(); j++)
            f.AddLine(deletedItem.GetDeletedLines()[j]);
    }

    if (!CanEncodeToCharset(f, m_header.Charset))
    {
#if wxUSE_GUI
        wxString msg;
        msg.Printf(_("The catalog couldn't be saved in '%s' charset as specified in catalog settings.\n\nIt was saved in UTF-8 instead and the setting was modified accordingly."),
                   m_header.Charset.c_str());
        wxMessageBox(msg, _("Error saving catalog"),
                     wxOK | wxICON_EXCLAMATION);
#endif
        m_header.Charset = "UTF-8";

        // Re-do the save again because we modified a header:
        f.Clear();
        return DoSaveOnly(f, crlf);
    }

    // Otherwise everything can be safely saved:
    return f.Write(crlf, wxCSConv(m_header.Charset));
}


bool Catalog::HasDuplicateItems() const
{
    typedef std::pair<wxString, wxString> MsgId;
    std::set<MsgId> ids;
    for (auto& item: m_items)
    {
        if (!ids.emplace(std::make_pair(item->GetContext(), item->GetString())).second)
            return true;
    }
    return false;
}

bool Catalog::FixDuplicateItems()
{
    auto oldname = m_fileName;

    TempDirectory tmpdir;
    if ( !tmpdir.IsOk() )
        return false;

    wxString po_file_temp = tmpdir.CreateFileName("catalog.po");
    wxString po_file_fixed = tmpdir.CreateFileName("fixed.po");

    if ( !DoSaveOnly(po_file_temp, wxTextFileType_Unix) )
    {
        wxLogError(_("Couldn't save file %s."), po_file_temp.c_str());
        return false;
    }

    ExecuteGettext(wxString::Format("msguniq -o %s %s",
                                    QuoteCmdlineArg(po_file_fixed),
                                    QuoteCmdlineArg(po_file_temp)));

    if (!wxFileName::FileExists(po_file_fixed))
        return false;

    bool ok = Load(po_file_fixed);
    m_fileName = oldname;
    return ok;
}


namespace
{

wxString MaskForType(const char *extensions, const wxString& description, bool showExt = true)
{
    (void)showExt;
#ifdef __WXMSW__
    if (showExt)
        return wxString::Format("%s (%s)|%s", description, extensions, extensions);
    else
#endif
        return wxString::Format("%s|%s", description, extensions);
}

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


int Catalog::Validate()
{
    TempDirectory tmpdir;
    if ( !tmpdir.IsOk() )
        return 0;

    wxString tmp_po = tmpdir.CreateFileName("validated.po");
    if ( !DoSaveOnly(tmp_po, wxTextFileType_Unix) )
        return 0;

    return DoValidate(tmp_po);
}

int Catalog::DoValidate(const wxString& po_file)
{
    GettextErrors err;
    ExecuteGettextAndParseOutput
    (
        wxString::Format("msgfmt -o /dev/null -c %s", QuoteCmdlineArg(CliSafeFileName(po_file))),
        err
    );

    for (auto& i: m_items)
    {
        i->SetValidity(CatalogItem::Val_Valid);
    }

    for ( GettextErrors::const_iterator i = err.begin(); i != err.end(); ++i )
    {
        if ( i->line != -1 )
        {
            auto item = FindItemByLine(i->line);
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

    return (int)err.size();
}

void Catalog::SetFileName(const wxString& fn)
{
    wxFileName f(fn);
    f.Normalize();
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
    root.Normalize();

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

#if wxUSE_GUI // TODO: better separation into another file
bool Catalog::Update(ProgressInfo *progress, bool summary, UpdateResultReason& reason)
{
    reason = UpdateResultReason::Unspecified;

    if (!m_isOk)
        return false;

    wxString cwd = wxGetCwd();
    auto path = GetSourcesBasePath();
    if (!path.empty())
    {
        if (!wxFileName::DirExists(path))
        {
            reason = UpdateResultReason::NoSourcesFound;
            return false;
        }

        wxSetWorkingDirectory(path);
    }

    SourceDigger dig(progress);

    auto newcat = dig.Dig(m_header.SearchPaths,
                          m_header.SearchPathsExcluded,
                          m_header.Keywords,
                          m_header.SourceCodeCharset,
                          reason);

    if (progress->Cancelled())
        reason = UpdateResultReason::CancelledByUser;

    if (newcat)
    {
        bool succ = false;
        if ( progress )
            progress->UpdateMessage(_("Merging differences..."));

        bool cancelledByUser = false;
        if (!summary || ShowMergeSummary(newcat, &cancelledByUser))
        {
            switch (m_fileType)
            {
                case Type::PO:
                    succ = Merge(newcat);
                    break;
                case Type::POT:
                    m_items = newcat->m_items;
                    succ = true;
                    break;
            }
        }
        if (!succ)
        {
            newcat.reset();
        }
        if (cancelledByUser)
            reason = UpdateResultReason::CancelledByUser;
    }

    wxSetWorkingDirectory(cwd);

    return newcat != nullptr;
}
#endif

bool Catalog::UpdateFromPOT(const wxString& pot_file,
                            bool summary,
                            UpdateResultReason& reason,
                            bool replace_header)
{
    reason = UpdateResultReason::Unspecified;
    if (!m_isOk) return false;

    CatalogPtr newcat = std::make_shared<Catalog>(pot_file, CreationFlag_IgnoreTranslations);

    if (!newcat->IsOk())
    {
        wxLogError(_("'%s' is not a valid POT file."), pot_file.c_str());
        return false;
    }

    bool cancelledByUser = false;
    if (!summary || ShowMergeSummary(newcat, &cancelledByUser))
    {
        if ( !Merge(newcat) )
            return false;
        if ( replace_header )
            CreateNewHeader(newcat->Header());
        return true;
    }
    else
    {
        if (cancelledByUser)
            reason = UpdateResultReason::CancelledByUser;
        return false;
    }
}


bool Catalog::Merge(const CatalogPtr& refcat)
{
    wxString oldname = m_fileName;

    TempDirectory tmpdir;
    if ( !tmpdir.IsOk() )
        return false;

    wxString tmp1 = tmpdir.CreateFileName("ref.pot");
    wxString tmp2 = tmpdir.CreateFileName("input.po");
    wxString tmp3 = tmpdir.CreateFileName("output.po");

    refcat->DoSaveOnly(tmp1, wxTextFileType_Unix);
    DoSaveOnly(tmp2, wxTextFileType_Unix);

    wxString flags("-q --force-po");
    if (wxConfig::Get()->ReadBool("use_tm_when_updating", false) == false)
    {
        flags += " --no-fuzzy-matching";
    }

    bool succ = ExecuteGettext
                (
                    wxString::Format
                    (
                        "msgmerge %s -o %s %s %s",
                        flags,
                        QuoteCmdlineArg(tmp3),
                        QuoteCmdlineArg(tmp2),
                        QuoteCmdlineArg(tmp1)
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


static inline wxString ItemMergeSummary(const CatalogItemPtr& item)
{
    wxString s = item->GetString();
    if ( item->HasPlural() )
        s += "|" + item->GetPluralString();
    if ( item->HasContext() )
        s += wxString::Format(" [%s]", item->GetContext());

    return s;
}

void Catalog::GetMergeSummary(const CatalogPtr& refcat,
                              wxArrayString& snew, wxArrayString& sobsolete)
{
    wxASSERT( snew.empty() );
    wxASSERT( sobsolete.empty() );

    std::set<wxString> strsThis, strsRef;

    for (auto& i: m_items)
        strsThis.insert(ItemMergeSummary(i));
    for (auto& i: refcat->m_items)
        strsRef.insert(ItemMergeSummary(i));

    for (auto& i: strsThis)
    {
        if (strsRef.find(i) == strsRef.end())
            sobsolete.Add(i);
    }

    for (auto& i: strsRef)
    {
        if (strsThis.find(i) == strsThis.end())
            snew.Add(i);
    }
}

bool Catalog::ShowMergeSummary(const CatalogPtr& refcat, bool *cancelledByUser)
{
#if wxUSE_GUI
    if (cancelledByUser)
        *cancelledByUser = false;
    if (wxConfig::Get()->ReadBool("show_summary", false))
    {
        wxArrayString snew, sobsolete;
        GetMergeSummary(refcat, snew, sobsolete);
        MergeSummaryDialog sdlg;
        sdlg.TransferTo(snew, sobsolete);
        bool ok = (sdlg.ShowModal() == wxID_OK);
        if (cancelledByUser)
            *cancelledByUser = !ok;
        return ok;
    }
    else
        return true;
#else
    (void)refcat;
    (void)cancelledByUser;
    return true;
#endif
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
    m_header.SetHeaderNotEmpty("Plural-Forms", lang.DefaultPluralFormsExpr());
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
        if (i->GetValidity() == CatalogItem::Val_Invalid)
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
    m_isFuzzy = false;
    m_moreFlags.Empty();

    if (flags.empty()) return;
    wxStringTokenizer tkn(flags.Mid(1), " ,", wxTOKEN_STRTOK);
    wxString s;
    while (tkn.HasMoreTokens())
    {
        s = tkn.GetNextToken();
        if (s == "fuzzy") m_isFuzzy = true;
        else m_moreFlags << ", " << s;
    }
}


wxString CatalogItem::GetFlags() const
{
    wxString f;
    if (m_isFuzzy) f << ", fuzzy";
    f << m_moreFlags;
    if (!f.empty())
        return "#" + f;
    else
        return wxEmptyString;
}

void CatalogItem::SetFuzzy(bool fuzzy)
{
    if (!fuzzy && m_isFuzzy)
        m_oldMsgid.clear();
    m_isFuzzy = fuzzy;
}

bool CatalogItem::IsInFormat(const wxString& format)
{
    wxString lookingFor;
    lookingFor.Printf("%s-format", format.c_str());
    wxStringTokenizer tkn(m_moreFlags, " ,", wxTOKEN_STRTOK);
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

void CatalogItem::SetTranslationFromSource()
{
    m_validity = Val_Unknown;
    m_isFuzzy = false;
    m_isAutomatic = false;
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
}

void CatalogItem::ClearTranslation()
{
    m_isFuzzy = false;
    m_isAutomatic = false;
    m_isTranslated = false;
    for (auto& t: m_translations)
    {
        if (!t.empty())
            m_isModified = true;
        t.clear();
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
            size_t idx = 0;
            while (idx < line.length() && line[idx] != _T(':')) { idx++; }
            while (idx < line.length() && !wxIsspace(line[idx])) { idx++; }

            refs.push_back(line.Left(idx));
            line = line.Mid(idx).Strip(wxString::both);
        }
    }

    return refs;
}
