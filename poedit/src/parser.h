
/*

    poedit, a wxWindows i18n catalogs editor

    ---------------
      parser.h
    
      Database of available parsers
    
      (c) Vaclav Slavik, 1999

*/


#ifdef __GNUG__
#pragma interface
#endif

#ifndef _PARSER_H_
#define _PARSER_H_

#include <wx/wx.h>
#include <wx/dynarray.h>
#include <wx/string.h>


/** This class holds information about an external parser. It does
    \b not do any parsing. The only functionality it provides
 */
class Parser
{
    public:
        /// User-oriented name of the parser (e.g. "C/C++").
        wxString Name;
        /** Semicolon-separated list of wildcards. The parser is capable
            of parsing files matching these wildcards. Example: "*.cpp;*.h"
         */
        wxString Extensions;
        /** Command used to execute the parser. %o expands to output file,
            %K to list of keywords and %F to list of files.
         */
        wxString Command;
        /** Expansion string for single keyword. %k expands to keyword. 
            %K in Command is replaced by n expansions of KeywordItem where
            n is the number of keywords.
         */
        wxString KeywordItem;
        /** Expansion string for single filename. %f expands to filename. 
            %F in Command is replaced by n expansions of FileItem where
            n is the number of filenames.
         */
        wxString FileItem;

        /// Returns array of files from 'files' that this parser understands.
        wxArrayString SelectParsable(const wxArrayString& files);
      
        /** Returns command line used to launch the parser with specified
            input. This expands all veriables in Command property of the
            parser and returns string that be directly passed to wxExecute.
            \param files    list of files to parse
            \param keywords list of recognized keywords
            \param output   name of temporary output file
         */
        wxString GetCommand(const wxArrayString& files, 
                            const wxArrayString& keywords, 
                            const wxString& output);
};

WX_DECLARE_OBJARRAY(Parser, ParserArray);

class WXDLLEXPORT wxConfigBase;

/** Database of all available parsers. This class is regular pseudo-template
    dynamic wxArray with additional methods for storing its content to
    wxConfig object and retrieving it.
 */
class ParsersDB : public ParserArray
{
    public:
        /// Reads DB from registry/dotfile.
        void Read(wxConfigBase *cfg);
        
        /// Write DB to registry/dotfile.
        void Write(wxConfigBase *cfg);
};

#endif // _PARSER_H_
