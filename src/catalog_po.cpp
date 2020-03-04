/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 1999-2020 Vaclav Slavik
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

#include "catalog_po.h"

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

#ifdef __WXOSX__
#import <Foundation/Foundation.h>
#endif

// TODO: split into different file
#if wxUSE_GUI
    #include <wx/msgdlg.h>
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

    output = input.Mid(in_pos);
    output.Trim(true); // trailing whitespace
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
        wxLogError(wxPLURAL(L"%i line of file “%s” was not loaded correctly.",
                            L"%i lines of file “%s” were not loaded correctly.",
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
                _(L"Line %d of file “%s” is corrupted (not valid %s data)."),
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

} // anonymous namespace


// ----------------------------------------------------------------------
// Parsers
// ----------------------------------------------------------------------

bool POCatalogParser::Parse()
{
    static const wxString prefix_flags(wxS("#, "));
    static const wxString prefix_autocomments(wxS("#. "));
    static const wxString prefix_autocomments2(wxS("#.")); // account for empty auto comments
    static const wxString prefix_references(wxS("#: "));
    static const wxString prefix_prev_msgid(wxS("#| "));
    static const wxString prefix_msgctxt(wxS("msgctxt \""));
    static const wxString prefix_msgid(wxS("msgid \""));
    static const wxString prefix_msgid_plural(wxS("msgid_plural \""));
    static const wxString prefix_msgstr(wxS("msgstr \""));
    static const wxString prefix_msgstr_plural(wxS("msgstr["));
    static const wxString prefix_deleted(wxS("#~"));
    static const wxString prefix_deleted_msgid(wxS("#~ msgid"));

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
        while (line.length() == 2 && *line.begin() == '#' && (line[1] == ',' || line[1] == ':' || line[1] == '|'))
            line = ReadTextLine();

        // flags:
        // Can't we have more than one flag, now only the last is kept ...
        if (ReadParam(line, prefix_flags, dummy))
        {
            static wxString prefix_flags_partial(wxS(", "));
            mflags = prefix_flags_partial + dummy;
            line = ReadTextLine();
        }

        // auto comments:
        if (ReadParam(line, prefix_autocomments, dummy) || ReadParam(line, prefix_autocomments2, dummy))
        {
            mextractedcomments.Add(dummy);
            line = ReadTextLine();
        }

        // references:
        else if (ReadParam(line, prefix_references, dummy))
        {
            // Just store the references unmodified, we don't modify this
            // data anywhere.
            mrefs.push_back(dummy);
            line = ReadTextLine();
        }

        // previous msgid value:
        else if (ReadParam(line, prefix_prev_msgid, dummy))
        {
            msgid_old.Add(dummy);
            line = ReadTextLine();
        }

        // msgctxt:
        else if (ReadParam(line, prefix_msgctxt, dummy))
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
        else if (ReadParam(line, prefix_msgid, dummy))
        {
            mstr = UnescapeCString(dummy.RemoveLast());
            mlinenum = unsigned(m_textFile->GetCurrentLine() + 1);
            while (!(line = ReadTextLine()).empty())
            {
                if (line[0u] == wxS('\t'))
                    line.Remove(0, 1);
                if (line[0u] == wxS('"') && line.Last() == wxS('"'))
                {
                    mstr += UnescapeCString(line.Mid(1, line.Length() - 2));
                    PossibleWrappedLine();
                }
                else
                    break;
            }
        }

        // msgid_plural:
        else if (ReadParam(line, prefix_msgid_plural, dummy))
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
        else if (ReadParam(line, prefix_msgstr, dummy))
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
        else if (ReadParam(line, prefix_msgstr_plural, dummy))
        {
            if (!has_plural)
            {
                wxLogError(_("Broken catalog file: plural form msgstr used without msgid_plural"));
                return false;
            }

            wxString idx = dummy.BeforeFirst(wxS(']'));
            wxString label_prefix = prefix_msgstr_plural + idx + wxS("] \"");

            while (ReadParam(line, label_prefix, dummy))
            {
                wxString str = UnescapeCString(dummy.RemoveLast());

                while (!(line=ReadTextLine()).empty())
                {
                    line.Trim(/*fromRight=*/false);
                    if (line[0u] == wxS('"') && line.Last() == wxS('"'))
                    {
                        str += UnescapeCString(line.Mid(1, line.Length() - 2));
                        PossibleWrappedLine();
                    }
                    else
                    {
                        if (ReadParam(line, prefix_msgstr_plural, dummy))
                        {
                            idx = dummy.BeforeFirst(wxS(']'));
                            label_prefix = prefix_msgstr_plural + idx + wxS("] \"");
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
        else if (ReadParam(line, prefix_deleted, dummy))
        {
            wxArrayString deletedLines;
            deletedLines.Add(line);
            mlinenum = unsigned(m_textFile->GetCurrentLine() + 1);
            while (!(line = ReadTextLine()).empty())
            {
                // if line does not start with "#~" anymore, stop reading
                if (!ReadParam(line, prefix_deleted, dummy))
                    break;
                // if the line starts with "#~ msgid", we skipped an empty line
                // and it's a new entry, so stop reading too (see bug #329)
                if (ReadParam(line, prefix_deleted_msgid, dummy))
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
        else if (line[0u] == wxS('#'))
        {
            bool readNewLine = false;

            while (!line.empty() &&
                    line[0u] == wxS('#') &&
                   (line.Length() < 2 || (line[1u] != wxS(',') && line[1u] != wxS(':') && line[1u] != wxS('.') && line[1u] != wxS('~') )))
            {
                mcomment << line << wxS('\n');
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


wxString POCatalogParser::ReadTextLine()
{
    m_previousLineHardWrapped = m_lastLineHardWrapped;
    m_lastLineHardWrapped = false;

    static const wxString msgid_alone(wxS("msgid \"\""));
    static const wxString msgstr_alone(wxS("msgstr \"\""));

    for (;;)
    {
        if (m_textFile->Eof())
            return wxString();

        // read next line and strip insignificant whitespace from it:
        const auto& ln = m_textFile->GetNextLine();
        if (ln.empty())
            continue;

        // gettext tools don't include (extracted) comments in wrapping, so they can't
        // be reliably used to detect file's wrapping either; just skip them.
        if (!ln.StartsWith(wxS("#. ")) && !ln.StartsWith(wxS("# ")))
        {
            if (ln.EndsWith(wxS("\\n\"")))
            {
                // Similarly, lines ending with \n are always wrapped, so skip that too.
                m_lastLineHardWrapped = true;
            }
            else if (ln == msgid_alone || ln == msgstr_alone)
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

        if (wxIsspace(ln[0]) || wxIsspace(ln.Last()))
        {
            auto s = ln.Strip(wxString::both);
            if (!s.empty())
                return s;
        }
        else
        {
            return ln;
        }
    }

    return wxString();
}

int POCatalogParser::GetWrappingWidth() const
{
    if (!m_detectedWrappedLines)
        return POCatalog::NO_WRAPPING;

    return m_detectedLineWidth;
}



class POCharsetInfoFinder : public POCatalogParser
{
    public:
        POCharsetInfoFinder(wxTextFile *f)
                : POCatalogParser(f), m_charset("UTF-8") {}
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
                    m_charset = "ISO-8859-1";
                return false; // stop parsing
            }
            return true;
        }
};



class POLoadParser : public POCatalogParser
{
    public:
        POLoadParser(POCatalog& c, wxTextFile *f)
              : POCatalogParser(f),
                FileIsValid(false),
                m_catalog(c), m_nextId(1), m_seenHeaderAlready(false), m_collectMsgidText(true) {}

        // true if the file is valid, i.e. has at least some data
        bool FileIsValid;

        Language GetMsgidLanguage()
        {
            auto lang = GetSpecifiedMsgidLanguage();
            if (lang.IsValid())
                return lang;

            auto utf8 = m_allMsgidText.utf8_str();
            lang = Language::TryDetectFromText(utf8.data(), utf8.length());
            if (!lang.IsValid())
                lang = Language::English();  // gettext historically assumes English
            return lang;
        }

    protected:
        Language GetSpecifiedMsgidLanguage()
        {
            auto x_srclang = m_catalog.Header().GetHeader("X-Source-Language");
            if (x_srclang.empty())
                x_srclang = m_catalog.m_header.GetHeader("X-Loco-Source-Locale");
            if (!x_srclang.empty())
            {
                auto parsed = Language::TryParse(str::to_utf8(x_srclang));
                if (parsed.IsValid())
                    return parsed;
            }
            return Language();
        }

        POCatalog& m_catalog;

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
        bool m_collectMsgidText;
        wxString m_allMsgidText;
};


bool POLoadParser::OnEntry(const wxString& msgid,
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
            m_collectMsgidText = !GetSpecifiedMsgidLanguage().IsValid();
            m_seenHeaderAlready = true;
        }
        // else: ignore duplicate header in malformed files
    }
    else
    {
        auto d = std::make_shared<POCatalogItem>();
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
        d->SetRawReferences(references);

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
        if (m_collectMsgidText)
        {
            m_allMsgidText.append(msgid);
            m_allMsgidText.append('\n');
            if (!msgid_plural.empty())
            {
                m_allMsgidText.append(msgid_plural);
                m_allMsgidText.append('\n');
            }
        }
    }
    return true;
}

bool POLoadParser::OnDeletedEntry(const wxArrayString& deletedLines,
                                const wxString& flags,
                                const wxArrayString& /*references*/,
                                const wxString& comment,
                                const wxArrayString& extractedComments,
                                unsigned lineNumber)
{
    FileIsValid = true;

    POCatalogDeletedData d;
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
// POCatalogItem class
// ----------------------------------------------------------------------

wxArrayString POCatalogItem::GetReferences() const
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


// ----------------------------------------------------------------------
// POCatalog class
// ----------------------------------------------------------------------

POCatalog::POCatalog(Type type) : Catalog(type)
{
    m_fileCRLF = wxTextFileType_None;
    m_fileWrappingWidth = DEFAULT_WRAPPING;
}

POCatalog::POCatalog(const wxString& po_file, int flags) : Catalog(Type::PO)
{
    m_fileCRLF = wxTextFileType_None;
    m_fileWrappingWidth = DEFAULT_WRAPPING;

    m_isOk = Load(po_file, flags);
}


bool POCatalog::HasCapability(Catalog::Cap cap) const
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


bool POCatalog::CanLoadFile(const wxString& extension)
{
    return extension == "po" || extension == "pot";
}


wxString POCatalog::GetPreferredExtension() const
{
    switch (m_fileType)
    {
        case Type::PO:
            return "po";
        case Type::POT:
            return "pot";

        case Type::XLIFF:
            wxFAIL_MSG("not possible here"); 
    }

    return "";
}


static inline wxString GetCurrentTimeString()
{
    return wxDateTime::Now().Format("%Y-%m-%d %H:%M%z");
}


bool POCatalog::Load(const wxString& po_file, int flags)
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
        POCharsetInfoFinder charsetFinder(&f);
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

    POLoadParser parser(*this, &f);
    parser.IgnoreHeader(flags & CreationFlag_IgnoreHeader);
    parser.IgnoreTranslations(flags & CreationFlag_IgnoreTranslations);
    if (!parser.Parse())
    {
        wxLogError(
            wxString::Format(
                _(L"Couldn’t load file %s, it is probably corrupted."),
                po_file.c_str()));
        return false;
    }

    m_sourceLanguage = parser.GetMsgidLanguage();

    // now that the catalog is loaded, update its items with the bookmarks
    for (unsigned i = BOOKMARK_0; i < BOOKMARK_LAST; i++)
    {
        if (m_header.Bookmarks[i] == -1)
            continue;

        if (m_header.Bookmarks[i] < (int)m_items.size())
        {
            m_items[m_header.Bookmarks[i]]->SetBookmark(
                    static_cast<Bookmark>(i));
        }
        else // invalid bookmark
        {
            m_header.Bookmarks[i] = -1;
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


void POCatalog::FixupCommonIssues()
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
        m_header.LanguageTeam.clear();
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
            pluralForms = m_header.Lang.DefaultPluralFormsExpr().str();
            if (!pluralForms.empty())
                m_header.SetHeader("Plural-Forms", pluralForms);
        }
    }

    // TODO: mark catalog as modified if any changes were made
}


void POCatalog::Clear()
{
    // Catalog base class fields:
    m_items.clear();
    m_isOk = true;
    for (int i = BOOKMARK_0; i < BOOKMARK_LAST; i++)
        m_header.Bookmarks[i] = -1;

    // PO-specific fields:
    m_deletedItems.clear();
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

template<typename Func>
inline void SplitIntoLines(const wxString& text, Func&& f)
{
    if (text.empty())
        return;

    wxString::const_iterator last = text.begin();
    for (wxString::const_iterator i = text.begin(); i != text.end(); ++i)
    {
        if (*i == '\n')
        {
            f(wxString(last, i), false);
            last = i + 1;
        }
    }

    if (last != text.end())
        f(wxString(last, text.end()), true);
}

void SaveMultiLines(wxTextBuffer &f, const wxString& text)
{
    SplitIntoLines(text, [&f](wxString&& s, bool)
    {
        f.AddLine(s);
    });
}

/** Adds \n characters as necessary for good-looking output
 */
wxString FormatStringForFile(const wxString& text)
{
    wxString s;
    s.reserve(text.length() + 16);

    static wxString quoted_newline(wxS("\"\n\""));

    SplitIntoLines(text, [&s](wxString&& piece, bool last)
    {
        if (!s.empty())
            s += quoted_newline;
        if (!last)
            piece += '\n';
        EscapeCStringInplace(piece);
        s += piece;
    });

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


bool POCatalog::Save(const wxString& po_file, bool save_mo,
                     ValidationResults& validation_results, CompilationStatus& mo_compilation_status)
{
    mo_compilation_status = CompilationStatus::NotDone;

    if ( wxFileExists(po_file) && !wxFile::Access(po_file, wxFile::write) )
    {
        wxLogError(_(L"File “%s” is read-only and cannot be saved.\nPlease save it under different name."),
                   po_file.c_str());
        return false;
    }

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

        case Type::XLIFF:
            wxFAIL_MSG("not possible here");
            break;
    }

    TempOutputFileFor po_file_temp_obj(po_file);
    const wxString po_file_temp = po_file_temp_obj.FileName();

    wxTextFileType outputCrlf = GetDesiredCRLFFormat(m_fileCRLF);
    // Save into Unix line endings first and only if Windows is required,
    // reformat the file later. This is because msgcat cannot handle DOS
    // input particularly well.

    if ( !DoSaveOnly(po_file_temp, wxTextFileType_Unix) )
    {
        wxLogError(_(L"Couldn’t save file %s."), po_file.c_str());
        return false;
    }

    try
    {
        validation_results = DoValidate(po_file_temp);
    }
    catch (...)
    {
        // DoValidate may fail catastrophically if app bundle is damaged, but
        // that shouldn't prevent Poedit from trying to save user's file.
        wxLogError("%s", DescribeCurrentException());
    }

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
            wxCSConv conv(m_header.Charset);
            wxTextFile finalFile(po_file_temp2);
            if (finalFile.Open(conv))
                finalFile.Write(outputCrlf, conv);
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
            wxLogError(_(L"Couldn’t save file %s."), po_file.c_str());
        }
        else
        {
            // Only shows msgcat's failure warning if we don't also get
            // validation errors, because if we do, the cause is likely the
            // same.
            if ( !validation_results.errors )
            {
                wxLogWarning(_("There was a problem formatting the file nicely (but it was saved all right)."));
            }
        }
    }

    
    /* If the user wants it, compile .mo file right now: */

    bool compileMO = save_mo;
    if (!wxConfig::Get()->Read("compile_mo", (long)true))
        compileMO = false;

    if (m_fileType == Type::PO && compileMO)
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
                    wxLogError(_(L"Couldn’t save file %s."), mo_file.c_str());
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
                wxLogError(_(L"Couldn’t save file %s."), mo_file.c_str());
                mo_compilation_status = CompilationStatus::Error;
            }
#endif // __WXOSX__/!__WXOSX__
        }
    }

    m_fileName = po_file;

    return true;
}


std::string POCatalog::SaveToBuffer()
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


bool POCatalog::CompileToMO(const wxString& mo_file,
                            ValidationResults& validation_results,
                            CompilationStatus& mo_compilation_status)
{
    mo_compilation_status = CompilationStatus::NotDone;

    TempDirectory tmpdir;
    if ( !tmpdir.IsOk() )
        return false;
    wxString po_file_temp = tmpdir.CreateFileName("output.po");

    if ( !DoSaveOnly(po_file_temp, wxTextFileType_Unix) )
    {
        wxLogError(_(L"Couldn’t save file %s."), po_file_temp.c_str());
        return false;
    }

    validation_results = DoValidate(po_file_temp);

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
        wxLogError(_(L"Couldn’t save file %s."), mo_file.c_str());
        return false;
    }

    return true;
}




bool POCatalog::DoSaveOnly(const wxString& po_file, wxTextFileType crlf)
{
    wxTextFile f;
    if (!f.Create(po_file))
        return false;

    return DoSaveOnly(f, crlf);
}

bool POCatalog::DoSaveOnly(wxTextBuffer& f, wxTextFileType crlf)
{
    /* Save .po file: */
    if (!m_header.Charset || m_header.Charset == "CHARSET")
        m_header.Charset = "UTF-8";

    SaveMultiLines(f, m_header.Comment);
    if (m_fileType == Type::POT)
        f.AddLine(wxS("#, fuzzy"));
    f.AddLine(wxS("msgid \"\""));
    f.AddLine(wxS("msgstr \"\""));
    wxString pohdr = wxString(wxS("\"")) + m_header.ToString(wxS("\"\n\""));
    pohdr.RemoveLast();
    SaveMultiLines(f, pohdr);
    f.AddLine(wxEmptyString);

    auto pluralsCount = GetPluralFormsCount();

    for (auto& data_: m_items)
    {
        auto data = std::static_pointer_cast<POCatalogItem>(data_);

        data->SetLineNumber(int(f.GetLineCount()+1));
        SaveMultiLines(f, data->GetComment());
        for (unsigned i = 0; i < data->GetExtractedComments().GetCount(); i++)
        {
            if (data->GetExtractedComments()[i].empty())
              f.AddLine(wxS("#."));
            else
              f.AddLine(wxS("#. ") + data->GetExtractedComments()[i]);
        }
        for (unsigned i = 0; i < data->GetRawReferences().GetCount(); i++)
            f.AddLine(wxS("#: ") + data->GetRawReferences()[i]);
        wxString dummy = data->GetFlags();
        if (!dummy.empty())
            f.AddLine(wxS("#") + dummy);
        for (unsigned i = 0; i < data->GetOldMsgidRaw().GetCount(); i++)
            f.AddLine(wxS("#| ") + data->GetOldMsgidRaw()[i]);
        if ( data->HasContext() )
        {
            SaveMultiLines(f, wxS("msgctxt \"") + FormatStringForFile(data->GetContext()) + wxS("\""));
        }
        dummy = FormatStringForFile(data->GetString());
        SaveMultiLines(f, wxS("msgid \"") + dummy + wxS("\""));
        if (data->HasPlural())
        {
            dummy = FormatStringForFile(data->GetPluralString());
            SaveMultiLines(f, wxS("msgid_plural \"") + dummy + wxS("\""));

            for (unsigned i = 0; i < pluralsCount; i++)
            {
                dummy = FormatStringForFile(data->GetTranslation(i));
                wxString hdr = wxString::Format(wxS("msgstr[%u] \""), i);
                SaveMultiLines(f, hdr + dummy + wxS("\""));
            }
        }
        else
        {
            dummy = FormatStringForFile(data->GetTranslation());
            SaveMultiLines(f, wxS("msgstr \"") + dummy + wxS("\""));
        }
        f.AddLine(wxEmptyString);
    }

    // Write back deleted items in the file so that they're not lost
    for (unsigned itemIdx = 0; itemIdx < m_deletedItems.size(); itemIdx++)
    {
        if ( itemIdx != 0 )
            f.AddLine(wxEmptyString);

        POCatalogDeletedData& deletedItem = m_deletedItems[itemIdx];
        deletedItem.SetLineNumber(int(f.GetLineCount()+1));
        SaveMultiLines(f, deletedItem.GetComment());
        for (unsigned i = 0; i < deletedItem.GetExtractedComments().GetCount(); i++)
            f.AddLine(wxS("#. ") + deletedItem.GetExtractedComments()[i]);
        for (unsigned i = 0; i < deletedItem.GetRawReferences().GetCount(); i++)
            f.AddLine(wxS("#: ") + deletedItem.GetRawReferences()[i]);
        wxString dummy = deletedItem.GetFlags();
        if (!dummy.empty())
            f.AddLine(wxS("#") + dummy);

        for (size_t j = 0; j < deletedItem.GetDeletedLines().GetCount(); j++)
            f.AddLine(deletedItem.GetDeletedLines()[j]);
    }

    if (!CanEncodeToCharset(f, m_header.Charset))
    {
#if wxUSE_GUI
        wxString msg;
        msg.Printf(_(L"The catalog couldn’t be saved in “%s” charset as specified in catalog settings.\n\nIt was saved in UTF-8 instead and the setting was modified accordingly."),
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


bool POCatalog::HasDuplicateItems() const
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

bool POCatalog::FixDuplicateItems()
{
    auto oldname = m_fileName;

    TempDirectory tmpdir;
    if ( !tmpdir.IsOk() )
        return false;

    wxString ext;
    wxFileName::SplitPath(m_fileName, nullptr, nullptr, &ext);
    wxString po_file_temp = tmpdir.CreateFileName("catalog." + ext);
    wxString po_file_fixed = tmpdir.CreateFileName("fixed." + ext);

    if ( !DoSaveOnly(po_file_temp, wxTextFileType_Unix) )
    {
        wxLogError(_(L"Couldn’t save file %s."), po_file_temp.c_str());
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


Catalog::ValidationResults POCatalog::Validate(bool wasJustLoaded)
{
    if (!HasCapability(Catalog::Cap::Translations))
        return ValidationResults();  // no errors in POT files

    if (wasJustLoaded)
    {
        return DoValidate(GetFileName());
    }
    else
    {
        TempDirectory tmpdir;
        if ( !tmpdir.IsOk() )
            return ValidationResults();

        wxString tmp_po = tmpdir.CreateFileName("validated.po");
        if ( !DoSaveOnly(tmp_po, wxTextFileType_Unix) )
            return ValidationResults();

        return DoValidate(tmp_po);
    }
}

Catalog::ValidationResults POCatalog::DoValidate(const wxString& po_file)
{
    ValidationResults res;

    GettextErrors err;
    ExecuteGettextAndParseOutput
    (
        wxString::Format("msgfmt -o /dev/null -c %s", QuoteCmdlineArg(CliSafeFileName(po_file))),
        err
    );

    for (auto& i: m_items)
        i->ClearIssue();

    res.errors = (int)err.size();

    if (Config::ShowWarnings())
        res.warnings = QAChecker::GetFor(*this)->Check(*this);

    for ( GettextErrors::const_iterator i = err.begin(); i != err.end(); ++i )
    {
        if ( i->line != -1 )
        {
            auto item = FindItemByLine(i->line);
            if ( item )
            {
                item->SetIssue(CatalogItem::Issue::Error, i->text);
                continue;
            }
        }
        // if not matched to an item:
        wxLogError(i->text);
    }

    return res;
}


bool POCatalog::UpdateFromPOT(const wxString& pot_file, bool replace_header)
{
    POCatalogPtr pot = std::make_shared<POCatalog>(pot_file, CreationFlag_IgnoreTranslations);
    if (!pot->IsOk())
    {
        wxLogError(_(L"“%s” is not a valid POT file."), pot_file.c_str());
        return false;
    }

    return UpdateFromPOT(pot, replace_header);
}

bool POCatalog::UpdateFromPOT(POCatalogPtr pot, bool replace_header)
{
    switch (m_fileType)
    {
        case Type::PO:
        {
            if (!Merge(pot))
                return false;
            break;
        }
        case Type::POT:
        {
            m_items = pot->m_items;
            break;
        }

        case Type::XLIFF:
            wxFAIL_MSG("not possible here");
            break;
    }

    if (replace_header)
        CreateNewHeader(pot->Header());

    return true;
}

POCatalogPtr POCatalog::CreateFromPOT(POCatalogPtr pot)
{
    POCatalogPtr c = std::make_shared<POCatalog>();
    if (c->UpdateFromPOT(pot, /*replace_header=*/true))
        return c;
    else
        return nullptr;
}

bool POCatalog::Merge(const POCatalogPtr& refcat)
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

    wxString flags("-q --force-po --previous");
    if (Config::MergeBehavior() == Merge_None)
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
