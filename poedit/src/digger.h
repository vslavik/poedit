
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
    
/** This class extracts translatable strings from sources.
    It uses ParsersDB to get information about external programs to
    call in order to dig information from single file.
 */
class SourceDigger
{
    public:
        /// Ctor. \a pi is used to display the progress of parsing.
        SourceDigger(ProgressInfo *pi) : m_progressInfo(pi) {}

        /** Scans files for translatable strings and returns Catalog
            instance containing them. All files in input \a paths that 
            match file extensions in a definition of parser in ParsersDB
            instance passed to the ctor are proceed by external parser 
            program (typically, gettext) according to parser definition.

            \param paths    list of directories to look in
            \param keywords list of keywords that are recognized as 
                            prefixes for translatable strings in sources
         */
        Catalog *Dig(const wxArrayString& paths, 
                     const wxArrayString& keywords);

    private:
        /** Finds all parsable files. Returned value is a new[]-allocated
            array of wxArrayString objects. n-th string array in returned
            array holds list of files that can be parsed by n-th parser
            in \a pdb database.
         */
        wxArrayString *FindFiles(const wxArrayString& paths, ParsersDB& pdb);

        /** Finds all files in given directory.
            \return false if an error occured.
         */
        bool FindInDir(const wxString& dirname, wxArrayString& files);

        /** Digs translatable strings from given files.
            \param cat      the catalog to store found strings to
            \param files    list of files to parse
            \param parser   parser definition
            \param keywords list of keywords that mark translatable strings
         */
        bool DigFiles(Catalog *cat, const wxArrayString& files, 
                      Parser &parser, const wxArrayString& keywords);

        ProgressInfo *m_progressInfo;
};



#endif // _DIGGER_H_
