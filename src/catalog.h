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

#ifndef _CATALOG_H_
#define _CATALOG_H_

#include <wx/encconv.h>
#include <wx/regex.h>
#include <wx/arrstr.h>
#include <wx/textfile.h>

#include <vector>
#include <map>

class ProgressInfo;

/// The possible bookmarks for a given item
typedef enum
{
    NO_BOOKMARK = -1,
    BOOKMARK_0,
    BOOKMARK_1,
    BOOKMARK_2,
    BOOKMARK_3,
    BOOKMARK_4,
    BOOKMARK_5,
    BOOKMARK_6,
    BOOKMARK_7,
    BOOKMARK_8,
    BOOKMARK_9,
    BOOKMARK_LAST
} Bookmark;

/** This class holds information about one particular string.
    This includes source string and its occurrences in source code
    (so-called references), translation and translation's status
    (fuzzy, non translated, translated) and optional comment.

    This class is mostly internal, used by Catalog to store data.
 */
class CatalogItem
{
    public:
        /// Ctor. Initializes the object with source string and translation.
        CatalogItem(const wxString& str = wxEmptyString,
                    const wxString& str_plural = wxEmptyString)
                : m_string(str),
                  m_plural(str_plural),
                  m_hasPlural(false),
                  m_hasContext(false),
                  m_references(),
                  m_isFuzzy(false),
                  m_isTranslated(false),
                  m_isModified(false),
                  m_isAutomatic(false),
                  m_validity(Val_Unknown),
                  m_lineNum(0),
                  m_bookmark(NO_BOOKMARK) {}

        CatalogItem(const CatalogItem& dt)
                : m_string(dt.m_string),
                  m_plural(dt.m_plural),
                  m_hasPlural(dt.m_hasPlural),
                  m_hasContext(dt.m_hasContext),
                  m_context(dt.m_context),
                  m_translations(dt.m_translations),
                  m_references(dt.m_references),
                  m_autocomments(dt.m_autocomments),
                  m_oldMsgid(dt.m_oldMsgid),
                  m_isFuzzy(dt.m_isFuzzy),
                  m_isTranslated(dt.m_isTranslated),
                  m_isModified(dt.m_isModified),
                  m_isAutomatic(dt.m_isAutomatic),
                  m_hasBadTokens(dt.m_hasBadTokens),
                  m_moreFlags(dt.m_moreFlags),
                  m_comment(dt.m_comment),
                  m_validity(dt.m_validity),
                  m_lineNum(dt.m_lineNum),
                  m_errorString(dt.m_errorString),
                  m_bookmark(dt.m_bookmark) {}

        /// Returns the source string.
        const wxString& GetString() const { return m_string; }

        /// Does this entry have a msgid_plural?
        bool HasPlural() const { return m_hasPlural; }

        /// Returns the plural string.
        const wxString& GetPluralString() const { return m_plural; }

        /// Does this entry have a msgctxt?
        bool HasContext() const { return m_hasContext; }

        /// Returns context string (can only be called if HasContext() returns
        /// true and empty string is accepted value).
        const wxString& GetContext() const { return m_context; }

        /// How many translations (plural forms) do we have?
        unsigned GetNumberOfTranslations() const
            { return m_translations.size(); }

        /// Returns number of plural forms in this translation; note that this
        /// may be less than what the header says, because some may be
        /// still untranslated
        unsigned GetPluralFormsCount() const;

        /// Returns the nth-translation.
        wxString GetTranslation(unsigned n = 0) const;

        /// Returns all translations.
        const wxArrayString& GetTranslations() const
            { return m_translations; }

        /// Returns references (#:) lines for the entry
        const wxArrayString& GetRawReferences() const { return m_references; }

        /// Returns array of all occurrences of this string in source code,
        /// parsed into individual references
        wxArrayString GetReferences() const;

        /// Returns comment added by the translator to this entry
        const wxString& GetComment() const { return m_comment; }

        /// Returns array of all auto comments.
        const wxArrayString& GetAutoComments() const { return m_autocomments; }

        /// Convenience function: does this entry has a comment?
        bool HasComment() const { return !m_comment.empty(); }

        /// Adds new reference to the entry (used by SourceDigger).
        void AddReference(const wxString& ref)
        {
            if (m_references.Index(ref) == wxNOT_FOUND)
                m_references.Add(ref);
        }

        /// Sets the string.
        void SetString(const wxString& s)
        {
            m_string = s;
            m_validity = Val_Unknown;
        }

        /// Sets the plural form (if applicable).
        void SetPluralString(const wxString& p)
        {
            m_plural = p;
            m_hasPlural = true;
        }

        /// Sets the context (msgctxt0
        void SetContext(const wxString& context)
        {
            m_hasContext = true;
            m_context = context;
        }

        /** Sets the translation. Changes "translated" status to true
            if \a t is not empty.
         */
        void SetTranslation(const wxString& t, unsigned index = 0);

        /// Sets all translations.
        void SetTranslations(const wxArrayString& t);

        /// Sets the comment.
        void SetComment(const wxString& c)
        {
            m_comment = c;
        }

        /** Sets gettext flags directly in string format. It may be
            either empty string or "#, fuzzy", "#, c-format",
            "#, fuzzy, c-format" or others (not understood by Poedit).
         */
        void SetFlags(const wxString& flags);

        /// Gets gettext flags. \see SetFlags
        wxString GetFlags() const;

        /// Sets fuzzy flag.
        void SetFuzzy(bool fuzzy) { m_isFuzzy = fuzzy; }
        /// Gets value of fuzzy flag.
        bool IsFuzzy() const { return m_isFuzzy; }
        /// Sets translated flag.
        void SetTranslated(bool t) { m_isTranslated = t; }
        /// Gets value of translated flag.
        bool IsTranslated() const { return m_isTranslated; }
        /// Sets modified flag.
        void SetModified(bool modified) { m_isModified = modified; }
        /// Gets value of modified flag.
        bool IsModified() const { return m_isModified; }
        /// Sets automatic translation flag.
        void SetAutomatic(bool automatic) { m_isAutomatic = automatic; }
        /// Gets value of automatic translation flag.
        bool IsAutomatic() const { return m_isAutomatic; }
        /// Sets the number of the line this entry occurs on.
        void SetLineNumber(int line) { m_lineNum = line; }
        /// Get line number of this entry.
        int GetLineNumber() const { return m_lineNum; }

        /** Returns true if the gettext flags line contains "foo-format"
            flag when called with "foo" as argument. */
        bool IsInFormat(const wxString& format);

        /// Adds new autocomments (#. )
        void AddAutoComments(const wxString& com)
        {
            m_autocomments.Add(com);
        }

        /// Clears autocomments.
        void ClearAutoComments()
        {
            m_autocomments.Clear();
        }

        void SetOldMsgid(const wxArrayString& data) { m_oldMsgid = data; }
        const wxArrayString& GetOldMsgid() const { return m_oldMsgid; }

        // Validity (syntax-checking) status of the entry:
        enum Validity
        {
            Val_Unknown = -1,
            Val_Invalid = 0,
            Val_Valid = 1
        };

        /** Checks if %i etc. are correct in the translation (true if yes).
            Strings that are not c-format are always correct. */
        Validity GetValidity() const { return m_validity; }
        void SetValidity(bool val)
            { m_validity = val ? Val_Valid : Val_Invalid; }

        void SetErrorString(const wxString& str) { m_errorString = str; }
        wxString GetErrorString() const { return m_errorString; }

        /// Returns the bookmark for the item
        Bookmark GetBookmark() const {return m_bookmark;}
        /// Returns true if the item has a bookmark
        bool HasBookmark() const {return (GetBookmark() != NO_BOOKMARK);}
        /// Sets the bookmark
        void SetBookmark(Bookmark bookmark) {m_bookmark = bookmark;}

    private:
        wxString m_string, m_plural;
        bool m_hasPlural;

        bool m_hasContext;
        wxString m_context;

        wxArrayString m_translations;

        wxArrayString m_references, m_autocomments;
        wxArrayString m_oldMsgid;
        bool m_isFuzzy, m_isTranslated, m_isModified, m_isAutomatic;
        bool m_hasBadTokens;
        wxString m_moreFlags;
        wxString m_comment;
        Validity m_validity;
        int m_lineNum;
        wxString m_errorString;
        Bookmark m_bookmark;
};

/** This class holds information about one particular deleted item.
    This includes deleted lines, references, translation's status
    (fuzzy, non translated, translated) and optional comment(s).

    This class is mostly internal, used by Catalog to store data.
 */
// FIXME: derive this from CatalogItem (or CatalogItemBase)
class CatalogDeletedData
{
    public:
        /// Ctor.
        CatalogDeletedData()
                : m_lineNum(0) {}
        CatalogDeletedData(const wxArrayString& deletedLines)
                : m_deletedLines(deletedLines),
                  m_lineNum(0) {}

        CatalogDeletedData(const CatalogDeletedData& dt)
                : m_deletedLines(dt.m_deletedLines),
                  m_references(dt.m_references),
                  m_autocomments(dt.m_autocomments),
                  m_flags(dt.m_flags),
                  m_comment(dt.m_comment),
                  m_lineNum(dt.m_lineNum) {}

        /// Returns the deleted lines.
        const wxArrayString& GetDeletedLines() const { return m_deletedLines; }

        /// Returns references (#:) lines for the entry
        const wxArrayString& GetRawReferences() const { return m_references; }

        /// Returns comment added by the translator to this entry
        const wxString& GetComment() const { return m_comment; }

        /// Returns array of all auto comments.
        const wxArrayString& GetAutoComments() const { return m_autocomments; }

        /// Convenience function: does this entry has a comment?
        bool HasComment() const { return !m_comment.empty(); }

        /// Adds new reference to the entry (used by SourceDigger).
        void AddReference(const wxString& ref)
        {
            if (m_references.Index(ref) == wxNOT_FOUND)
                m_references.Add(ref);
        }

        /// Sets the string.
        void SetDeletedLines(const wxArrayString& a)
        {
            m_deletedLines = a;
        }

        /// Sets the comment.
        void SetComment(const wxString& c)
        {
            m_comment = c;
        }

        /** Sets gettext flags directly in string format. It may be
            either empty string or "#, fuzzy", "#, c-format",
            "#, fuzzy, c-format" or others (not understood by Poedit).
         */
        void SetFlags(const wxString& flags) {m_flags = flags;};

        /// Gets gettext flags. \see SetFlags
        wxString GetFlags() const {return m_flags;};

        /// Sets the number of the line this entry occurs on.
        void SetLineNumber(int line) { m_lineNum = line; }
        /// Get line number of this entry.
        int GetLineNumber() const { return m_lineNum; }

        /// Adds new autocomments (#. )
        void AddAutoComments(const wxString& com)
        {
            m_autocomments.Add(com);
        }

        /// Clears autocomments.
        void ClearAutoComments()
        {
            m_autocomments.Clear();
        }

    private:
        wxArrayString m_deletedLines;

        wxArrayString m_references, m_autocomments;
        wxString m_flags;
        wxString m_comment;
        int m_lineNum;
};


typedef std::vector<CatalogItem> CatalogItemArray;
typedef std::vector<CatalogDeletedData> CatalogDeletedDataArray;
typedef std::map<wxString, unsigned> CatalogItemIndex;

/** This class stores all translations, together with filelists, references
    and other additional information. It can read .po files and save both
    .mo and .po files. Furthermore, it provides facilities for updating the
    catalog from source files.
 */
class Catalog
{
    public:
        /// PO file header information.
        class HeaderData
        {
        public:
            HeaderData() {}

            /** Initializes the headers from string that is in msgid "" format
                (i.e. list of key:value\n entries). */
            void FromString(const wxString& str);

            /** Converts the header into string representation that can be
                directly written to .po file as msgid "". */
            wxString ToString(const wxString& line_delim = wxEmptyString);

            /// Updates headers list from parsed values entries below
            void UpdateDict();
            /// Reverse operation to UpdateDict
            void ParseDict();

            /// Returns value of header or empty string if missing.
            wxString GetHeader(const wxString& key) const;

            /// Returns true if given key is present in the header.
            bool HasHeader(const wxString& key) const;

            /** Sets header to given value. Overwrites old value if present,
                appends to the end of header values otherwise. */
            void SetHeader(const wxString& key, const wxString& value);

            /// Like SetHeader, but deletes the header if value is empty
            void SetHeaderNotEmpty(const wxString& key, const wxString& value);

            /// Removes given header entry
            void DeleteHeader(const wxString& key);

            struct Entry
            {
                wxString Key, Value;
            };
            typedef std::vector<Entry> Entries;

            const Entries& GetAllHeaders() const { return m_entries; }

            // Parsed values:

            wxString LanguageCode, Project, CreationDate,
                     RevisionDate, Translator, TranslatorEmail,
                     Team, TeamEmail, Charset, SourceCodeCharset;

            wxArrayString SearchPaths, Keywords;
            int Bookmarks[BOOKMARK_LAST];
            wxString BasePath;

            wxString Comment;

        protected:
            Entries m_entries;

            const Entry *Find(const wxString& key) const;
        };

        enum CreationFlags
        {
            CreationFlag_IgnoreHeader = 1
        };

        /// Default ctor. Creates empty catalog, you have to call Load.
        Catalog();

        /// Ctor that loads the catalog from \a po_file with Load.
        /// \a flags is CreationFlags combination.
        Catalog(const wxString& po_file, int flags = 0);

        ~Catalog();

        /** Creates new, empty header. Sets Charset to something meaningful
            ("UTF-8", currently).
         */
        void CreateNewHeader();

        /** Creates new header initialized based on a POT file's header.
         */
        void CreateNewHeader(const HeaderData& pot_header);

        /// Clears the catalog, removes all entries from it.
        void Clear();

        /** Loads catalog from .po file.
            If file named po_file ".poedit" (e.g. "cs.po.poedit") exists,
            this function loads additional information from it. .po.poedit
            file contains parts of catalog header data that are not part
            of standard .po format, namely SearchPaths, Keywords, BasePath
            and Language.

            @param flags  CreationFlags combination.
         */
        bool Load(const wxString& po_file, int flags = 0);

        /** Saves catalog to file. Creates both .po (text) and .mo (binary)
            version of the catalog (unless the latter was disabled in
            preferences). Calls external xmsgfmt program to generate the .mo
            file.

            Note that \a po_file refers to .po file, .mo file will have same
            name & location as .po file except for different extension.
         */
        bool Save(const wxString& po_file, bool save_mo, int& validation_errors);

        /// Exports the catalog to HTML format
        bool ExportToHTML(const wxString& filename);

        /** Updates the catalog from sources.
            \see SourceDigger, Parser, UpdateFromPOT.
         */
        bool Update(ProgressInfo *progress, bool summary = true);

        /** Updates the catalog from POT file.
            \see Update
         */
        bool UpdateFromPOT(const wxString& pot_file,
                           bool summary = true,
                           bool replace_header = false);

        /// Returns the number of strings/translations in the catalog.
        size_t GetCount() const { return m_items.size(); }

        /** Returns number of all, fuzzy, badtokens and untranslated items.
            Any argument may be NULL if the caller is not interested in
            given statistic value.

            @note "untranslated" are entries without translation; "unfinished"
                  are entries with any problems
         */
        void GetStatistics(int *all, int *fuzzy, int *badtokens,
                           int *untranslated, int *unfinished);

        /// Gets n-th item in the catalog (read-write access).
        CatalogItem& operator[](unsigned n) { return m_items[n]; }

        /// Gets n-th item in the catalog (read-only access).
        const CatalogItem& operator[](unsigned n) const { return m_items[n]; }

        /// Gets catalog header (read-write access).
        HeaderData& Header() { return m_header; }

        /// Returns plural forms count: taken from Plural-Forms header if
        /// present, 0 otherwise (unless there are existing plural forms
        /// translations in the file)
        unsigned GetPluralFormsCount() const;

        /// Returns true if Plural-Forms header doesn't match plural forms
        /// usage in catalog items
        bool HasWrongPluralFormsCount() const;

        /// Does this catalog have any items with plural forms?
        bool HasPluralItems() const;

        /** Returns status of catalog object: true if ok, false if damaged
            (i.e. constructor or Load failed).
         */
        bool IsOk() const { return m_isOk; }

        /** Returns xx_YY ISO code of catalog's language if either the Poedit
            extensions headers are present or if filename is known and is in
            the xx[_YY] form, otherwise returns empty string. */
        wxString GetLocaleCode() const;

        /// Adds entry to the catalog (the catalog will take ownership of
        /// the object).
        void AddItem(const CatalogItem& data);

        /// Adds entry to the catalog (the catalog will take ownership of
        /// the object).
        void AddDeletedItem(const CatalogDeletedData& data);

        /// Returns true if the catalog contains obsolete entries (~.*)
        bool HasDeletedItems();

        /// Removes all obsolete translations from the catalog
        void RemoveDeletedItems();

        /// Finds item by line number
        CatalogItem *FindItemByLine(int lineno);

        /// Sets the given item to have the given bookmark and returns the index
        /// of the item that previously had this bookmark (or -1)
        int SetBookmark(int id, Bookmark bookmark);

        /// Returns the index of the item that has the given bookmark or -1
        int GetBookmarkIndex(Bookmark bookmark) const
        {
            return m_header.Bookmarks[bookmark];
        }


        /// Validates correctness of the translation by running msgfmt
        /// Returns number of errors (i.e. 0 if no errors).
        int Validate();

    protected:
        int DoValidate(const wxString& po_file);
        bool DoSaveOnly(const wxString& po_file);
        bool DoSaveOnly(wxTextFile& f, wxTextFileType crlf);

        /** Merges the catalog with reference catalog
            (in the sense of msgmerge -- this catalog is old one with
            translations, \a refcat is reference catalog created by Update().)

            \return true if the merge was successfull, false otherwise.
                    Note that if it returns false, the catalog was
                    \em not modified!
         */
        bool Merge(Catalog *refcat);

        /** Returns list of strings that are new in reference catalog
            (compared to this one) and that are not present in \a refcat
            (i.e. are obsoleted).

            \see ShowMergeSummary
         */
        void GetMergeSummary(Catalog *refcat,
                             wxArrayString& snew, wxArrayString& sobsolete);

        /** Shows a dialog with merge summary.
            \see GetMergeSummary, Merge

            \return true if the merge was OK'ed by the user, false otherwise
         */
        bool ShowMergeSummary(Catalog *refcat);

    private:
        CatalogItemArray m_items;
        CatalogDeletedDataArray m_deletedItems;

        bool m_isOk;
        wxString m_fileName;
        HeaderData m_header;

        friend class LoadParser;
};


/// Internal class - used for parsing of po files.
class CatalogParser
{
    public:
        CatalogParser(wxTextFile *f)
            : m_textFile(f),
              m_ignoreHeader(false)
        {}

        virtual ~CatalogParser() {}

        /// Tell the parser to ignore header entries when processing
        void IgnoreHeader(bool ignore) { m_ignoreHeader = ignore; }

        /** Parses the entire file, calls OnEntry each time
            new msgid/msgstr pair is found.

            @return false if parsing failed, true otherwise
         */
        bool Parse();

    protected:
        /** Called when new entry was parsed. Parsing continues
            if returned value is true and is cancelled if it
            is false.
         */
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
                             unsigned lineNumber) = 0;

        /** Called when new deleted entry was parsed. Parsing continues
            if returned value is true and is cancelled if it
            is false. Defaults to an empty implementation.
         */
        virtual bool OnDeletedEntry(const wxArrayString& /*deletedLines*/,
                                    const wxString& /*flags*/,
                                    const wxArrayString& /*references*/,
                                    const wxString& /*comment*/,
                                    const wxArrayString& /*autocomments*/,
                                    unsigned /*lineNumber*/)
        {
            return true;
        }

        /// Textfile being parsed.
        wxTextFile *m_textFile;

        /// Whether the header should be parsed or not
        bool m_ignoreHeader;
};

#endif // _CATALOG_H_
