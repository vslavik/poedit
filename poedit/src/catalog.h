
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



class wxTextFile;

class CatalogData;
WX_DECLARE_OBJARRAY(CatalogData, CatalogDataArray);
    
// This class stores all translations, together with filelists, references
// and other additional information. It can read .po files and same both
// .mo and .po files

class Catalog
{
    public:
            struct HeaderData
            {
                wxString Language, Project, CreationDate, 
                         RevisionDate, Translator, TranslatorEmail,
                         Team, TeamEmail, Charset;
                         
                wxArrayString SearchPaths, Keywords;
                wxString BasePath;

                wxString Comment;
            };
            

            Catalog();
            Catalog(const wxString& po_file);
            ~Catalog();
            
            // Creates new, empty header
            void CreateNewHeader();

            // Clear the catalog, remove all entries
            void Clear();

            // Loads catalog from .po file
            // (used by ctor)
            bool Load(const wxString& po_file);

            // Save catalog into file. Creates both .po (text) and .mo (binary) version of file.
            // The argument refers to .po file! .mo file will have same name & location as .po
            // file except for different extension.
            bool Save(const wxString& po_file, bool save_mo = true);

            // Updates the catalog from sources:
            bool Update();
            
            // Add translation into catalog. Returns true on success or false if such key does
            // not exist in catalog
            bool Translate(const wxString& key, const wxString& translation);
            
            // Returns pointer to item with key 'key' or NULL if not available
            CatalogData* FindItem(const wxString& key);
            
            // Return number of strings/translations in the catalog:
            unsigned GetCount() const { return m_Count; }
            
            // Return number of all, fuzzy and untranslated items
            // (any argument may be NULL)
            void GetStatistics(int *all, int *fuzzy, int *untranslated);
            
            // Get n-th item in catalog:
            CatalogData& operator[](unsigned n) { return m_DataArray[n]; }
            
            // Get catalog header (read-write access)
            HeaderData& Header() { return m_Header; }
            
            // Return status of catalog object: true if ok, false if damaged
            // (e.g. constructor failed)
            bool IsOk() const { return m_IsOk; }
            
            // Append content of the other catalog into this one
            void Append(Catalog& cat);


    protected:

            // Merges the catalog with reference one
            // (in the sense of msgmerge -- this catalog is old one with
            // translations, refcat is reference catalog created by Update()
            void Merge(Catalog *refcat);

            // Returns list of strings that are new in reference catalog
	        // (compared to this one) and that are not present in refcat
	        // (i.e. are obsoleted)
	        void GetMergeSummary(Catalog *refcat, 
	                             wxArrayString& snew, wxArrayString& sobsolete);
            
    private:
            
            wxHashTable *m_Data;
            CatalogDataArray m_DataArray;
            unsigned m_Count; // no. of items 
            bool m_IsOk;
            wxString m_FileName;
            wxFontEncoding m_FileEncoding, m_MemEncoding;
            
            HeaderData m_Header;
            
            friend class LoadParser;
};



// Internal class - used for parsing of po files

class CatalogParser
{
    public:
            CatalogParser(wxTextFile *f) : m_TextFile(f) {}
            virtual ~CatalogParser() {}

            // Parses the entire file, calls OnEntry each time
            // new msgid/msgstr pair is found
            void Parse();
            
    protected:
            // Called when new entry was parsed. Parsing continues
            // if returned value is true and is cancelled if it 
            // is false
            virtual bool OnEntry(const wxString& msgid,
                                 const wxString& msgstr,
                                 const wxString& flags,
                                 const wxArrayString& references,
                                 const wxString& comment) = 0;
                                 
            wxTextFile *m_TextFile;
};





// Internal class - used for storing data in hashtable:
    
class CatalogData : public wxObject
{
    public:
            CatalogData(const wxString& str, const wxString& translation) 
                    : wxObject(), m_String(str), 
                      m_Translation(translation), 
                      m_References(),
                      m_IsFuzzy(false),
                      m_IsTranslated(!translation.IsEmpty()) {}
    
            const wxString& GetString() const { return m_String; }
            const wxString& GetTranslation() const { return m_Translation; }
            const wxArrayString& GetReferences() const { return m_References; }
            const wxString& GetComment() const { return m_Comment; }
            
            
            void AddReference(const wxString& ref)
            {
                if (m_References.Index(ref) == wxNOT_FOUND) 
                    m_References.Add(ref);
            }
            
            void ClearReferences()
            {
                m_References.Clear();
            }
            
            void SetString(const wxString& s) { m_String = s; }
            void SetTranslation(const wxString& t) 
            { 
                m_Translation = t; 
                m_IsTranslated = !t.IsEmpty();
            }
            void SetComment(const wxString& c) { m_Comment = c; }
            
            // set/get gettext flags, i.e. empty string or e.g.
            // "#, fuzzy"
            // "#, fuzzy, c-format
            void SetFlags(const wxString& flags);
            wxString GetFlags();
            
            // sets one flag:
            void SetFuzzy(bool fuzzy) { m_IsFuzzy = fuzzy; }
            bool IsFuzzy() { return m_IsFuzzy; }
            void SetTranslated(bool t) { m_IsTranslated = t; }
            bool IsTranslated() { return m_IsTranslated; }
            
    private:
            wxString m_String, m_Translation;
            wxArrayString m_References;
            bool m_IsFuzzy, m_IsTranslated;
            wxString m_MoreFlags;
            wxString m_Comment;
};






#endif // _CATALOG_H_
