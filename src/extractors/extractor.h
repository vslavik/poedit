/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 2017 Vaclav Slavik
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

#include <memory>
#include <set>
#include <vector>

#include <wx/string.h>

#include "utility.h"

/**
    Base class for extractors -- implementations of extracting translations
    from source code.
 */
class Extractor
{
public:
    /// Specification of the source code to search.
    struct SourceCodeSpec
    {
        wxString BasePath;
        wxArrayString SearchPaths;
        wxArrayString ExcludedPaths;

        wxArrayString Keywords;
        wxString Charset;
    };

    typedef std::vector<std::shared_ptr<Extractor>> ExtractorsList;
    typedef std::vector<wxString> FilesList;

    // Static helper methods:

    /**
        Returns all available extractor implementations.

        Extractors are listed in their priority and should be used in this
        order, i.e. subsequent extractors should only be used to process files
        not yet handled by previous extractors.
     */
    static ExtractorsList CreateAllExtractors();

    /**
        Collects all files from source code, possibly including files that
        don't contain translations.

        The returned list is guaranteed to be sorted by operator<
     */
    static FilesList CollectAllFiles(const SourceCodeSpec& sources);


    /**
        Extracts translations from given source files using all
        available extractors.

        Returns filename of the created POT file, which is stored in @a tmpdir
        or empty string on failure.
     */
    static wxString ExtractWithAll(TempDirectory& tmpdir,
                                   const SourceCodeSpec& sourceSpec,
                                   const std::vector<wxString>& files);

    // Extractor helpers:

    /// Returns only those files from @a files that are supported by this extractor.
    FilesList FilterFiles(const FilesList& files) const;

    // Extractor API for derived classes:

    /// Returns extractor's symbolic name
    virtual wxString GetId() const = 0;

    /// Returns whether the file is recognized
    virtual bool IsFileSupported(const wxString& file) const = 0;

    /**
        Extracts translations from given source files using all
        available extractors.

        Returns filename of the created POT file, which is stored in @a tmpdir
        or empty string on failure.
     */
    virtual wxString Extract(TempDirectory& tmpdir,
                             const SourceCodeSpec& sourceSpec,
                             const std::vector<wxString>& files) const = 0;

protected:
    Extractor() {}
    virtual ~Extractor() {}

    /// Concatenates catalogs using msgcat
    static wxString ConcatCatalogs(TempDirectory& tmpdir, const std::vector<wxString>& files);

protected:
    // private factories:
    static ExtractorsList CreateAllLegacyExtractors();
};

#endif // Poedit_extractor_h
