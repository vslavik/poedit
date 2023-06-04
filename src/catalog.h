/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 1999-2023 Vaclav Slavik
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

#ifndef Poedit_catalog_h
#define Poedit_catalog_h

#include "language.h"

#include <wx/encconv.h>
#include <wx/arrstr.h>
#include <wx/textfile.h>

#include <initializer_list>
#include <iostream>
#include <map>
#include <memory>
#include <vector>

class CloudSyncDestination;
struct SourceCodeSpec;

class Catalog;
class CatalogItem;
typedef std::shared_ptr<CatalogItem> CatalogItemPtr;
typedef std::shared_ptr<Catalog> CatalogPtr;


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


/**
    Optional data attached to CatalogItem.

    Used primarily in connection wiht UsesSymbolicIDsForSource() to provide
    read source strings.
 */
struct SideloadedItemData
{
    /// Source string replacement
    wxString source_string, source_plural_string;

    /// Extracted comments / description (used by 
    wxArrayString extracted_comments;
};

/**
    Optional data attached to Catalog.

    Used primarily in connection wiht UsesSymbolicIDsForSource() to provide
    read source strings.
 */
struct SideloadedCatalogData
{
    CatalogPtr reference_file;
    Language source_language;
};


/** This class holds information about one particular string.
    This includes source string and its occurrences in source code
    (so-called references), translation and translation's status
    (fuzzy, non translated, translated) and optional comment.

    This class is mostly internal, used by Catalog to store data.
 */
class CatalogItem
{
    protected:
        /// Ctor. Initializes the object with source string and translation.
        CatalogItem()
                : m_id(0),
                  m_hasPlural(false),
                  m_hasContext(false),
                  m_isFuzzy(false),
                  m_isTranslated(false),
                  m_isModified(false),
                  m_isPreTranslated(false),
                  m_lineNum(0),
                  m_bookmark(NO_BOOKMARK) {}

        CatalogItem(const CatalogItem&) = delete;

        virtual ~CatalogItem() {}

    public:
        // -------------------------------------------------------------------
        // Read-only access to values in the item:
        // -------------------------------------------------------------------

        /// Gets numeric, 1-based ID
        int GetId() const { return m_id; }

        // Implementation for use in derived classes:
        virtual wxString GetRawSymbolicId() const { return wxString(); }
        const wxString& GetRawString() const { return m_string; }
        const wxString& GetRawPluralString() const { return m_plural; }

        /// Get item's symbolic ID if used by the file
        wxString GetSymbolicId() const { return m_sideloaded ? GetRawString() : GetRawSymbolicId(); }
        bool HasSymbolicId() const { return !GetSymbolicId().empty(); }

        /// Returns the source string.
        const wxString& GetString() const { return m_sideloaded ? m_sideloaded->source_string : GetRawString(); }

        /// Returns the plural string.
        const wxString& GetPluralString() const { return m_sideloaded ? m_sideloaded->source_plural_string : GetRawPluralString(); }

        /// Does this entry have a msgid_plural?
        bool HasPlural() const { return m_hasPlural; }

        /// Does this entry have a msgctxt?
        bool HasContext() const { return m_hasContext; }

        /// Returns context string (can only be called if HasContext() returns
        /// true and empty string is accepted value).
        const wxString& GetContext() const { return m_context; }

        /// How many translations (plural forms) do we have?
        unsigned GetNumberOfTranslations() const
            { return (unsigned)m_translations.size(); }

        /// Returns number of plural forms in this translation; note that this
        /// may be less than what the header says, because some may be
        /// still untranslated
        unsigned GetPluralFormsCount() const;

        /// Returns the nth-translation.
        wxString GetTranslation(unsigned n = 0) const;

        /// Returns all translations.
        const wxArrayString& GetTranslations() const { return m_translations; }

        /// Returns array of all occurrences of this string in source code,
        /// parsed into individual references
        virtual wxArrayString GetReferences() const = 0;

        /// Returns comment added by the translator to this entry
        const wxString& GetComment() const { return m_comment; }

        /// Returns array of all auto comments.
        const wxArrayString& GetExtractedComments() const { return m_sideloaded ? m_sideloaded->extracted_comments : m_extractedComments; }

        /// Convenience function: does this entry has a comment?
        bool HasComment() const { return !m_comment.empty(); }

        /// Convenience function: does this entry has auto comments?
        bool HasExtractedComments() const { return !GetExtractedComments().empty(); }

        /// Gets gettext flags. \see SetFlags
        wxString GetFlags() const;

        /// Returns format flag ("php" for "php-format" etc.) if there's any,
        // empty string otherwise
        wxString GetFormatFlag() const;

        /// Like GetFormatFlags(), but only for internal uses (e.g. fileformat-specific highlighting)
        virtual std::string GetInternalFormatFlag() const { return std::string(); }

        /// Gets value of fuzzy flag.
        bool IsFuzzy() const { return m_isFuzzy; }
        /// Gets value of translated flag.
        bool IsTranslated() const { return m_isTranslated; }
        /// Gets value of modified flag.
        bool IsModified() const { return m_isModified; }
        /// Gets value of pre-translated translation flag.
        bool IsPreTranslated() const { return m_isPreTranslated; }
        /// Get line number of this entry.
        int GetLineNumber() const { return m_lineNum; }

        const wxArrayString& GetOldMsgidRaw() const { return m_oldMsgid; }
        wxString GetOldMsgid() const;
        bool HasOldMsgid() const { return !m_oldMsgid.empty(); }

        /// Returns the bookmark for the item
        Bookmark GetBookmark() const {return m_bookmark;}
        /// Returns true if the item has a bookmark
        bool HasBookmark() const {return (GetBookmark() != NO_BOOKMARK);}


        // -------------------------------------------------------------------
        // Setters for user-editable values:
        // -------------------------------------------------------------------

        /** Sets the translation. Changes "translated" status to true
            if \a t is not empty.
         */
        void SetTranslation(const wxString& t, unsigned index = 0);

        /// Sets all translations.
        void SetTranslations(const wxArrayString& t);

        /// Set translations to equal source text.
        void SetTranslationFromSource();

        // Clears all translation content from the entry
        void ClearTranslation();

        /// Sets fuzzy flag.
        void SetFuzzy(bool fuzzy);
        /// Sets translated flag.
        void SetTranslated(bool t) { m_isTranslated = t; }
        /// Sets modified flag.
        void SetModified(bool modified) { m_isModified = modified; }
        /// Sets pre-translated translation flag.
        void SetPreTranslated(bool pre) { m_isPreTranslated = pre; }
        /// Sets the bookmark
        void SetBookmark(Bookmark bookmark) {m_bookmark = bookmark;}

        /// Sets the comment.
        void SetComment(const wxString& c) { m_comment = c; }


        // -------------------------------------------------------------------
        // Auxiliary data attached to the item, such as QA issues. Setting those
        // is done in base class only, because they don't affect the saved output.
        // -------------------------------------------------------------------

        struct Issue
        {
            enum Severity
            {
                Warning,
                Error
            };

            Severity severity;
            wxString message;

            Issue(Severity s, const wxString& m) : severity(s), message(m) {}
        };

        bool HasIssue() const { return m_issue != nullptr; }
        bool HasError() const { return m_issue && m_issue->severity == Issue::Error; }
        const std::shared_ptr<Issue>& GetIssue() const { return m_issue; }

        void ClearIssue() { m_issue.reset(); }
        void SetIssue(std::shared_ptr<Issue> issue) { m_issue = issue; }
        void SetIssue(const Issue& issue) { m_issue = std::make_shared<Issue>(issue); }
        void SetIssue(Issue::Severity severity, const wxString& message) { m_issue = std::make_shared<Issue>(severity, message); }

        void AttachSideloadedData(const std::shared_ptr<SideloadedItemData>& d) { m_sideloaded = d; }
        void ClearSideloadedData() { m_sideloaded.reset(); }

    protected:
        // API for subclasses:
        virtual void UpdateInternalRepresentation() = 0;

    protected:
        // -------------------------------------------------------------------
        // Private data setters only for internal use:
        // -------------------------------------------------------------------

        void SetId(int id) { m_id = id; }

        void SetString(const wxString& s)
        {
            m_string = s;
            ClearIssue();
        }

        void SetPluralString(const wxString& p)
        {
            m_plural = p;
            m_hasPlural = true;
        }

        void SetContext(const wxString& context)
        {
            m_hasContext = true;
            m_context = context;
        }

        void SetLineNumber(int line) { m_lineNum = line; }

        void AddExtractedComments(const wxString& com)
        {
            m_extractedComments.Add(com);
        }

        void SetOldMsgid(const wxArrayString& data) { m_oldMsgid = data; }

        /** Sets gettext flags directly in string format. It may be
            either empty string or ", fuzzy", ", c-format",
            ", fuzzy, c-format" or others (not understood by Poedit),
            i.e. the leading # is not included, but ", " is.
         */
        void SetFlags(const wxString& flags);

    protected:
        int m_id;

        wxString m_string, m_plural;
        bool m_hasPlural;

        bool m_hasContext;
        wxString m_context;

        wxArrayString m_translations;

        wxArrayString m_extractedComments;
        wxArrayString m_oldMsgid;
        bool m_isFuzzy, m_isTranslated, m_isModified, m_isPreTranslated;
        wxString m_moreFlags;
        wxString m_comment;
        int m_lineNum;
        Bookmark m_bookmark;

        std::shared_ptr<Issue> m_issue;
        std::shared_ptr<SideloadedItemData> m_sideloaded;
};


typedef std::vector<CatalogItemPtr> CatalogItemArray;


/** This class stores all translations, together with filelists, references
    and other additional information. It can read .po files and save both
    .mo and .po files. Furthermore, it provides facilities for updating the
    catalog from source files.
 */
class Catalog
{
    public:
        /// Type of the file loaded
        enum class Type
        {
            PO,
            POT,
            XLIFF,
            JSON,
            JSON_FLUTTER
        };

        /// Capabilities of the file type
        enum class Cap
        {
            Translations,       // Can translations be added (e.g. POTs can't)?
            LanguageSetting,    // Is language code saved in the file?
            UserComments,       // Can users add comments?
            FuzzyTranslations   // Can translations be marked as needing work?
        };

        /// Is this file capable of doing these things
        virtual bool HasCapability(Cap cap) const = 0;

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

            wxString Project, CreationDate,
                     RevisionDate, Translator, TranslatorEmail,
                     LanguageTeam, Charset, SourceCodeCharset;
            Language Lang;

            wxArrayString SearchPaths, SearchPathsExcluded, Keywords;
            int Bookmarks[BOOKMARK_LAST];
            wxString BasePath;

            wxString Comment;

        protected:
            Entries m_entries;

            const Entry *Find(const wxString& key) const;

            void NormalizeHeaderOrder();
        };

        enum CreationFlags
        {
            CreationFlag_IgnoreHeader       = 1,
            CreationFlag_IgnoreTranslations = 2
        };

        enum class CompilationStatus
        {
            NotDone,
            Success,
            Error
        };

        struct ValidationResults
        {
            ValidationResults() : errors(0), warnings(0) {}
            int errors;
            int warnings;
        };

        /// Default ctor. Creates empty catalog, you have to call Load.
        static CatalogPtr Create(Type type);

        /**
            Ctor that loads the catalog from \a po_file with Load.
            \a flags is CreationFlags combination.

            This function never returns nullptr. In throws on failure.
         */
        static CatalogPtr Create(const wxString& filename, int flags = 0);

        static bool CanLoadFile(const wxString& extension);

        virtual wxString GetPreferredExtension() const = 0;

        virtual ~Catalog() {}

        /** Creates new, empty header. Sets Charset to something meaningful
            ("UTF-8", currently).
         */
        void CreateNewHeader();

        /** Creates new header initialized based on a POT file's header.
         */
        void CreateNewHeader(const HeaderData& pot_header);

        /** Saves catalog to file. Creates both .po (text) and .mo (binary)
            version of the catalog (unless the latter was disabled in
            preferences). Calls external xmsgfmt program to generate the .mo
            file.

            Note that \a po_file refers to .po file, .mo file will have same
            name & location as .po file except for different extension.
         */
        virtual bool Save(const wxString& po_file, bool save_mo,
                          ValidationResults& validation_results,
                          CompilationStatus& mo_compilation_status) = 0;

        /**
            "Saves" the PO file into a memory buffer with content identical
            to what Save() would save into a file.
            
            Returns empty string in case of failure.
         */
        virtual std::string SaveToBuffer() = 0;

        /// File mask for opening/saving this catalog's file type
        wxString GetFileMask() const { return GetTypesFileMask({m_fileType}); }
        /// File mask for opening/saving any supported file type
        static wxString GetTypesFileMask(std::initializer_list<Type> types);
        static wxString GetAllTypesFileMask();

        /// Exports the catalog to HTML format
        void ExportToHTML(std::ostream& output);

        Type GetFileType() const { return m_fileType; }

        wxString GetFileName() const { return m_fileName; }
        void SetFileName(const wxString& fn);

        /**
            Return base path to source code for extraction, or empty string if not configured.
            
            This is the path that file references are relative to. It should be,
            but may not be, the root of the source tree.
         */
        wxString GetSourcesBasePath() const;

        /**
            Returns top-most directory of the configured source tree.
            
            Returns empty string if not configured.
         */
        wxString GetSourcesRootPath() const;

        /**
            Returns true if the source code to update the PO from is available.
         */
        bool HasSourcesConfigured() const;

        /**
            Returns true if the source code to update the PO from is available.
         */
        bool HasSourcesAvailable() const;

        std::shared_ptr<SourceCodeSpec> GetSourceCodeSpec() const;

        /// Returns the number of strings/translations in the catalog.
        unsigned GetCount() const { return (unsigned)m_items.size(); }

        const CatalogItemArray& items() const { return m_items; }

        /// Is the catalog empty?
        bool empty() const { return m_items.empty(); }

        /** Returns number of all, fuzzy, badtokens and untranslated items.
            Any argument may be NULL if the caller is not interested in
            given statistic value.

            @note "untranslated" are entries without translation; "unfinished"
                  are entries with any problems
         */
        void GetStatistics(int *all, int *fuzzy, int *badtokens,
                           int *untranslated, int *unfinished);

        /// Gets n-th item in the catalog (read-write access).
        CatalogItemPtr operator[](unsigned n) { return m_items[n]; }

        /// Gets n-th item in the catalog (read-only access).
        const CatalogItemPtr operator[](unsigned n) const { return m_items[n]; }

        /// Gets catalog header (read-write access).
        HeaderData& Header() { return m_header; }

        /// Returns plural forms count: taken from Plural-Forms header if
        /// present, 0 otherwise (unless there are existing plural forms
        /// translations in the file)
        virtual unsigned GetPluralFormsCount() const;

        /// Returns catalog's source language (may be invalid, but usually English).
        Language GetSourceLanguage() const { return m_sideloaded ? m_sideloaded->source_language : m_sourceLanguage; }

        /// Returns catalog's language (may be invalid).
        virtual Language GetLanguage() const { return m_header.Lang; }

        /// Change the catalog's language and update headers accordingly
        virtual void SetLanguage(Language lang);

        /// Whether source text is just symbolic identifier and not actual text
        bool UsesSymbolicIDsForSource() const { return m_sourceIsSymbolicID && !m_sideloaded; }
            
        /// Returns true if the catalog contains obsolete entries (~.*)
        virtual bool HasDeletedItems() const = 0;

        /// Removes all obsolete translations from the catalog
        virtual void RemoveDeletedItems() = 0;

        /// Finds item by line number
        CatalogItemPtr FindItemByLine(int lineno);

        /// Finds catalog index by line number
        int FindItemIndexByLine(int lineno);

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
        virtual ValidationResults Validate(const wxString& fileWithSameContent = wxString());

        void AttachCloudSync(std::shared_ptr<CloudSyncDestination> c) { m_cloudSync = c; }
        std::shared_ptr<CloudSyncDestination> GetCloudSync() const { return m_cloudSync; }

        /**
            Attach source text data from another file to this one.

            This is used to implement showing of actual source text when the in-file source is
            just symbolic IDs. After calling this function, Get(Plural)String() will no longer
            return the ID, but will instead return as source text the translation from @a ref
            (typically you'll want that file to be for English).

            Attaches data from the @a ref file to this one,
         */
        void SideloadSourceDataFromReferenceFile(CatalogPtr ref);

        /// Undo the effect of SideloadSourceDataFromReferenceFile()
        void ClearSideloadedSourceData();

        /// Whether the source text was replaced from another file
        bool HasSideloadedReferenceFile() const { return m_sideloaded != nullptr; }
        std::shared_ptr<SideloadedCatalogData> GetSideloadedSourceData() const { return m_sideloaded; }

    protected:
        Catalog(Type type);

        /// Perform post-creation processing to e.g. fixup issues, detect missing language etc.
        virtual void PostCreation();

    protected:
        CatalogItemArray m_items;

        Type m_fileType;
        wxString m_fileName;
        HeaderData m_header;
        Language m_sourceLanguage;
        bool m_sourceIsSymbolicID = false;

        std::shared_ptr<CloudSyncDestination> m_cloudSync;
        std::shared_ptr<SideloadedCatalogData> m_sideloaded;
};

#endif // Poedit_catalog_h
