
/*

    poedit, a wxWindows i18n catalogs editor

    ---------------
      catalog.h
    
      Translations catalog
    
      (c) Vaclav Slavik, 1999-2004

*/

#ifndef _CATALOG_H_
#define _CATALOG_H_

#include <wx/hash.h>
#include <wx/dynarray.h>
#include <wx/encconv.h>
#include <wx/regex.h>
#include <vector>

class WXDLLEXPORT wxTextFile;

class CatalogData;
WX_DECLARE_OBJARRAY(CatalogData, CatalogDataArray);
    
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
            
            wxString Language, Country, Project, CreationDate, 
                     RevisionDate, Translator, TranslatorEmail,
                     Team, TeamEmail, Charset, SourceCodeCharset;

            wxArrayString SearchPaths, Keywords;
            wxString BasePath;

            wxString Comment;

        protected:
            Entries m_entries;

            const Entry *Find(const wxString& key) const;
        };

        /// Default ctor. Creates empty catalog, you have to call Load.
        Catalog();
        /// Ctor that loads the catalog from \a po_file with Load.
        Catalog(const wxString& po_file);
        ~Catalog();

        /** Creates new, empty header. Sets Charset to something meaningful
            ("UTF-8", currently).
         */
        void CreateNewHeader();

        /// Clears the catalog, removes all entries from it.
        void Clear();

        /** Loads catalog from .po file.
            If file named po_file ".poedit" (e.g. "cs.po.poedit") exists,
            this function loads additional information from it. .po.poedit
            file contains parts of catalog header data that are not part 
            of standard .po format, namely SearchPaths, Keywords, BasePath
            and Language.
         */
        bool Load(const wxString& po_file);

        /** Saves catalog to file. Creates both .po (text) and .mo (binary) 
            version of the catalog (unless the latter was disabled in 
            preferences). Calls external xmsgfmt program to generate the .mo
            file. 

            Note that \a po_file refers to .po file, .mo file will have same 
            name & location as .po file except for different extension.

            If the header contains data not covered by the PO format, 
            additional poEdit-specific file with .po.poedit extension is 
            created.
         */
        bool Save(const wxString& po_file, bool save_mo = true);

        /// Exports the catalog to HTML format
        bool ExportToHTML(const wxString& filename);

        /** Updates the catalog from sources.
            \see SourceDigger, Parser, UpdateFromPOT.
         */
        bool Update();

        /** Updates the catalog from POT file.
            \see Update
         */
        bool UpdateFromPOT(const wxString& pot_file);

        /** Adds translation into the catalog. 
            \return true on success or false if such key does
                     not exist in the catalog
         */
        bool Translate(const wxString& key, const wxString& translation);

        /** Returns pointer to catalog item with key \a key or NULL if 
            such key is not available.
         */
        CatalogData* FindItem(const wxString& key) const;

        /// Returns the number of strings/translations in the catalog.
        size_t GetCount() const { return m_count; }

        /** Returns number of all, fuzzy, badtokens and untranslated items.
            Any argument may be NULL if the caller is not interested in
            given statistic value.
         */
        void GetStatistics(int *all, int *fuzzy, int *badtokens, int *untranslated);

        /// Gets n-th item in the catalog (read-write access).
        CatalogData& operator[](unsigned n) { return m_dataArray[n]; }

        /// Gets n-th item in the catalog (read-only access).
        const CatalogData& operator[](unsigned n) const { return m_dataArray[n]; }

        /// Gets catalog header (read-write access).
        HeaderData& Header() { return m_header; }

        /// Returns plural forms count: taken from Plural-Forms header if
        /// present, 0 otherwise
        unsigned GetPluralFormsCount() const;

        /** Returns status of catalog object: true if ok, false if damaged
            (i.e. constructor or Load failed).
         */
        bool IsOk() const { return m_isOk; }

        /// Appends content of \cat to this catalog.
        void Append(Catalog& cat);

        /** Returns xx_YY ISO code of catalog's language if either the poEdit
            extensions headers are present or if filename is known and is in
            the xx[_YY] form, otherwise returns empty string. */
        wxString GetLocaleCode() const;

        /// Adds entry to the catalog (the catalog will take ownership of
        /// the object).
        void AddItem(CatalogData *data);

    protected:
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
        wxHashTable *m_data;
        CatalogDataArray m_dataArray;
        unsigned m_count; // no. of items 
        bool m_isOk;
        wxString m_fileName;
        HeaderData m_header;

        friend class LoadParser;
};


/// Internal class - used for parsing of po files.
class CatalogParser
{
    public:
        CatalogParser(wxTextFile *f, wxMBConv *conv)
            : m_textFile(f), m_conv(conv) {}
        virtual ~CatalogParser() {}

        /** Parses the entire file, calls OnEntry each time
            new msgid/msgstr pair is found.
         */
        void Parse();
            
    protected:
        /** Called when new entry was parsed. Parsing continues
            if returned value is true and is cancelled if it 
            is false.
         */
        virtual bool OnEntry(const wxString& msgid,
                             const wxString& msgid_plural,
                             bool has_plural,
                             const wxArrayString& mtranslations,
                             const wxString& flags,
                             const wxArrayString& references,
                             const wxString& comment,
                             const wxArrayString& autocomments,
                             unsigned lineNumber) = 0;

        /// Textfile being parsed.
        wxTextFile *m_textFile;
        wxMBConv   *m_conv;
};


/** This class holds information about one particular string.
    This includes original string and its occurences in source code
    (so-called references), translation and translation's status
    (fuzzy, non translated, translated and optional comment.
    
    This class is mostly internal, used by Catalog to store data.
 */
class CatalogData : public wxObject
{
    public:
        /// Ctor. Initializes the object with original string and translation.
        CatalogData(const wxString& str, const wxString& str_plural)
                : wxObject(), 
                  m_string(str),
                  m_plural(str_plural),
                  m_hasPlural(false),
                  m_references(),
                  m_isFuzzy(false),
                  m_isTranslated(false),
                  m_isModified(false),
                  m_isAutomatic(false),
                  m_validity(Val_Unknown),
                  m_lineNum(0) {}

        CatalogData(const CatalogData& dt)
                : wxObject(),
                  m_string(dt.m_string),
                  m_plural(dt.m_plural),
                  m_hasPlural(dt.m_hasPlural),
                  m_translations(dt.m_translations),
                  m_references(dt.m_references),
                  m_autocomments(dt.m_autocomments),
                  m_isFuzzy(dt.m_isFuzzy),
                  m_isTranslated(dt.m_isTranslated),
                  m_isModified(dt.m_isModified),
                  m_isAutomatic(dt.m_isAutomatic),
                  m_hasBadTokens(dt.m_hasBadTokens),
                  m_moreFlags(dt.m_moreFlags),
                  m_comment(dt.m_comment),
                  m_validity(dt.m_validity),
                  m_lineNum(dt.m_lineNum),
                  m_errorString(dt.m_errorString) {}

        /// Returns the original string.
        const wxString& GetString() const { return m_string; }

        /// Does this entry have a msgid_plural?
        const bool HasPlural() const { return m_hasPlural; }

        /// Returns the plural string.
        const wxString& GetPluralString() const { return m_plural; }

        /// How many translations (plural forms) do we have?
        size_t GetNumberOfTranslations() const
            { return m_translations.GetCount(); }
 
        /// Returns the nth-translation.
        wxString GetTranslation(unsigned n = 0) const;

        /// Returns array of all occurences of this string in source code.
        const wxArrayString& GetReferences() const { return m_references; }

        /// Returns comment added by the translator to this entry
        const wxString& GetComment() const { return m_comment; }

        /// Returns array of all auto comments.
        const wxArrayString& GetAutoComments() const { return m_autocomments; }

        /// Convenience function: does this entry has a comment?
        const bool HasComment() const { return !m_comment.IsEmpty(); }

        /// Adds new reference to the entry (used by SourceDigger).
        void AddReference(const wxString& ref)
        {
            if (m_references.Index(ref) == wxNOT_FOUND) 
                m_references.Add(ref);
        }

        /// Clears references (used by SourceDigger).
        void ClearReferences()
        {
            m_references.Clear();
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
            "#, fuzzy, c-format" or others (not understood by poEdit).
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
        void SetLineNumber(unsigned line) { m_lineNum = line; }
        /// Get line number of this entry.
        unsigned GetLineNumber() const { return m_lineNum; }

        /** Returns true if the gettext flags line contains "foo-format"
            flag when called with "foo" as argument. */
        bool IsInFormat(const wxString& format);
        
        /// Adds new autocomments (#. )
        void AddAutoComments(const wxString& com)
        {
            if (m_autocomments.Index(com) == wxNOT_FOUND) 
                m_autocomments.Add(com);
        }

        /// Clears autocomments.
        void ClearAutoComments()
        {
            m_autocomments.Clear();
        }
        
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

    private:
        wxString m_string, m_plural;
        bool m_hasPlural;
        wxArrayString m_translations;
        
        wxArrayString m_references, m_autocomments;
        bool m_isFuzzy, m_isTranslated, m_isModified, m_isAutomatic;
        bool m_hasBadTokens;
        wxString m_moreFlags;
        wxString m_comment;
        Validity m_validity;
        unsigned m_lineNum;
        wxString m_errorString;
};

#endif // _CATALOG_H_
