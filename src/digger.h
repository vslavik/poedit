/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 2000-2017 Vaclav Slavik
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

#ifndef _DIGGER_H_
#define _DIGGER_H_

#include <wx/hash.h>
#include <wx/dynarray.h>

#include "catalog.h"

class wxArrayString;
class ExtractorsDB;
class Extractor;
class ProgressInfo;
class TempDirectory;

/** This class extracts translatable strings from sources.
    It uses ExtractorsDB to get information about external programs to
    call in order to dig information from single file.
 */
class SourceDigger
{
    public:
        /// Ctor. \a pi is used to display the progress of parsing.
        SourceDigger(ProgressInfo *progress) : m_progressInfo(progress) {}

        /** Scans files for translatable strings and returns Catalog
            instance containing them. All files in input \a paths that 
            match file extensions in a definition of extractor in ExtractorsDB
            instance passed to the ctor are proceed by external extractor
            program (typically, gettext) according to extractor definition.

            \param paths    list of directories to look in
            \param excludePaths directories to skip
            \param keywords list of keywords that are recognized as 
                            prefixes for translatable strings in sources
            \param charset  source code charset (may be empty)
         */
        CatalogPtr Dig(const wxArrayString& paths,
                       const wxArrayString& excludePaths,
                       const wxArrayString& keywords,
                       const wxString& charset,
                       UpdateResultReason& reason);

    private:
        // Path matching with support for wildcards
        struct PathToMatch
        {
            wxString path;
            bool isWildcard;

            bool MatchesFile(const wxString& fn) const
            {
                if (isWildcard)
                    return wxMatchWild(path, fn);
                else
                    return fn == path || fn.StartsWith(path + "/");
            }
        };

        class PathsToMatch
        {
        public:
            PathsToMatch() {}
            explicit PathsToMatch(const wxArrayString& a)
            {
                paths.reserve(a.size());
                for (auto& p: a)
                    paths.push_back({p, wxIsWild(p)});
            }

            bool MatchesFile(const wxString& fn) const
            {
                for (auto& p: paths)
                {
                    if (p.MatchesFile(fn))
                        return true;
                }
                return false;
            }

        private:
            std::vector<PathToMatch> paths;
        };

        /** Finds all parsable files. Returned value is a new[]-allocated
            array of wxArrayString objects. n-th string array in returned
            array holds list of files that can be parsed by n-th extractor
            in \a pdb database.
         */
        wxArrayString *FindFiles(const wxArrayString& paths,
                                 const PathsToMatch& excludePaths,
                                 ExtractorsDB& pdb);

        /** Finds all files in given directory.
            \return false if an error occurred.
         */
        int FindInDir(const wxString& dirname,
                      const PathsToMatch& excludePaths,
                      wxArrayString& files);

        /** Digs translatable strings from given files.
            \param outfiles list to which (temporary) file name of extracted
                            catalog will be appended
            \param files    list of files to parse
            \param extractor   extractor definition
            \param keywords list of keywords that mark translatable strings
            \param charset  source code charset (may be empty)
         */
        bool DigFiles(TempDirectory& tmpdir,
                      wxArrayString& outFiles,
                      const wxArrayString& files,
                      Extractor &extract, const wxArrayString& keywords,
                      const wxString& charset);

        ProgressInfo *m_progressInfo;
};



#endif // _DIGGER_H_
