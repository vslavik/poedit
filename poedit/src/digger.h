
/*

    poedit, a wxWindows i18n catalogs editor

    ---------------
      digger.h
    
      Sources digging class (xgettext)
    
      (c) Vaclav Slavik, 2000

*/


#ifdef __GNUG__
#pragma interface
#endif

#ifndef _DIGGER_H_
#define _DIGGER_H_

#include <wx/hash.h>
#include <wx/dynarray.h>


class Catalog;
class wxArrayString;
class ParsersDB;
class Parser;
class ProgressInfo;
    
// This class extracts translatable strings from sources

class SourceDigger
{
    public:
            SourceDigger(ProgressInfo *pi) : m_ProgressInfo(pi) {}
                        
            // Save catalog into file. Creates both .po (text) and .mo (binary) version of file.
            // The argument refers to .po file! .mo file will have same name & location as .po
            // file except for different extension.
            Catalog *Dig(const wxArrayString& paths, 
                         const wxArrayString& keywords);

    private:
            wxArrayString *FindFiles(const wxArrayString& paths, ParsersDB& pdb);
            bool FindInDir(const wxString& dirname, wxArrayString& files);
            bool DigFiles(Catalog *cat, const wxArrayString& files, 
                          Parser &parser, const wxArrayString& keywords);
                          
            ProgressInfo *m_ProgressInfo;
};



#endif // _DIGGER_H_
