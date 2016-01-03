/*
 *  This file is part of Poedit (http://poedit.net)
 *
 *  Copyright (C) 1999-2015 Vaclav Slavik
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

#ifndef Poedit_extractor_h
#define Poedit_extractor_h

#include <wx/wx.h>
#include <wx/string.h>

#include <vector>

class WXDLLIMPEXP_FWD_BASE wxConfigBase;


/** This class holds information about an external extractor. It does
    \b not do any extraction. The only functionality it provides is
    the metadata to invoke extractors.
 */
class Extractor
{
    public:
        Extractor() : Enabled(true) {}

        /// User-oriented name of the extractor (e.g. "C/C++").
        wxString Name;

        /// Whether the extractor is currently enabled.
        bool Enabled;

        /** Semicolon-separated list of wildcards. The extractor is capable
            of parsing files matching these wildcards. Example: "*.cpp;*.h"
         */
        wxString Extensions;
        /** Command used to execute the extractor. %o expands to output file,
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

        /** Expansion string for single charset setting. %c expands to
            charset name. %C in command is replaced with this. */
        wxString CharsetItem;

        /// Returns array of files from 'files' that this extractor understands.
        wxArrayString SelectParsable(const wxArrayString& files);
      
        /** Returns command line used to launch the extractor with specified
            input. This expands all veriables in Command property of the
            parser and returns string that be directly passed to wxExecute.
            \param files    list of files to parse
            \param keywords list of recognized keywords
            \param output   name of temporary output file
            \param charset  source code charset (may be empty)
         */
        wxString GetCommand(const wxArrayString& files, 
                            const wxArrayString& keywords,
                            const wxString& output,
                            const wxString& charset);
};

/** Database of all available extractors. This class is regular pseudo-template
    dynamic wxArray with additional methods for storing its content to
    wxConfig object and retrieving it.
 */
class ExtractorsDB
{
public:
    /// Reads DB from registry/dotfile.
    void Read(wxConfigBase *cfg);
    
    /// Write DB to registry/dotfile.
    void Write(wxConfigBase *cfg);

    /// Returns index of extractor with given name or -1 if it can't be found:
    int FindExtractor(const wxString& name);

    std::vector<Extractor> Data;
};

#endif // Poedit_extractor_h
