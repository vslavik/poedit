
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


// Information about available parsers:

class Parser
{
    public:
        wxString Name, 
                 Extensions,
                 Command, 
                 KeywordItem, 
                 FileItem;

        // returns array of files from 'files' that this
        // parser understands
        wxArrayString SelectParsable(const wxArrayString& files);
      
        // returns command line used to launch the parser
        wxString GetCommand(const wxArrayString& files, 
                            const wxArrayString& keywords, 
                            const wxString& output);
};

WX_DECLARE_OBJARRAY(Parser, ParserArray);

class wxConfigBase;

// Database of all available parsers.

class ParsersDB : public ParserArray
{
    public:
        // Read DB from registry
        void Read(wxConfigBase *cfg);
        
        // Write DB to registry
        void Write(wxConfigBase *cfg);
};

#endif // _PARSER_H_
