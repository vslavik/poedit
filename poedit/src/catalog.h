
/*

    poedit, a wxWindows i18n catalogs editor

    ---------------
      catalog.h
    
      Translations catalog
    
      (c) Vaclav Slavik, 1999

*/


#ifdef __GNUG__
#pragma interface
#endif

#ifndef _CATALOG_H_
#define _CATALOG_H_

#include <wx/hash.h>
#include <wx/dynarray.h>
#include <wx/encconv.h>



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
        struct HeaderData
        {
            wxString Language, Project, CreationDate, 
                     RevisionDate, Translator, TranslatorEmail,
                     Team, TeamEmail, Charset;

            wxArrayString SearchPaths, Keywords;
            wxString BasePath;

            wxString Comment;
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

        /** Updates the catalog from sources.
            \see SourceDigger, Parser.
         */
        bool Update();

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

        /** Returns number of all, fuzzy and untranslated items.
            Any argument may be NULL if the caller is not interested in
            given statistic value.
         */
        void GetStatistics(int *all, int *fuzzy, int *untranslated);

        /// Gets n-th item in the catalog (read-write access).
        CatalogData& operator[](unsigned n) { return m_dataArray[n]; }

        /// Gets n-th item in the catalog (read-only access).
        const CatalogData& operator[](unsigned n) const { return m_dataArray[n]; }

        /// Gets catalog header (read-write access).
        HeaderData& Header() { return m_header; }

        /** Returns status of catalog object: true if ok, false if damaged
            (i.e. constructor or Load failed).
         */
        bool IsOk() const { return m_isOk; }

        /// Appends content of \cat to this catalog.
        void Append(Catalog& cat);


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
         */
	    void GetMergeSummary(Catalog *refcat, 
	                         wxArrayString& snew, wxArrayString& sobsolete);
            
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
        CatalogParser(wxTextFile *f) : m_textFile(f) {}
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
                             const wxString& msgstr,
                             const wxString& flags,
                             const wxArrayString& references,
                             const wxString& comment,
                             unsigned lineNumber) = 0;

        /// Textfile being parsed.
        wxTextFile *m_textFile;
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
        CatalogData(const wxString& str, const wxString& translation) 
                : wxObject(), 
                  m_string(str), 
                  m_translation(translation), 
                  m_references(),
                  m_isFuzzy(false),
                  m_isTranslated(!translation.IsEmpty()),
                  m_isModified(false),
                  m_isAutomatic(false),
                  m_lineNum(0) {}

        /// Returns the original string.
        const wxString& GetString() const { return m_string; }

        /// Returns the translation.
        const wxString& GetTranslation() const { return m_translation; }

        /// Returns array of all occurences of this string in source code.
        const wxArrayString& GetReferences() const { return m_references; }

        /// Returns comment added by the translator to this entry
        const wxString& GetComment() const { return m_comment; }

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
        void SetString(const wxString& s) { m_string = s; }

        /** Sets the translation. Changes "translated" status to true
            if \a t is not empty.
         */
        void SetTranslation(const wxString& t) 
        { 
            m_translation = t; 
            m_isTranslated = !t.IsEmpty();
        }

        /// Sets the comment.
        void SetComment(const wxString& c) { m_comment = c; }

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
            
    private:
        wxString m_string, m_translation;
        wxArrayString m_references;
        bool m_isFuzzy, m_isTranslated, m_isModified, m_isAutomatic;
        wxString m_moreFlags;
        wxString m_comment;
        unsigned m_lineNum;
};

#endif // _CATALOG_H_
