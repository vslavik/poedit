/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 2000-2025 Vaclav Slavik
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

#ifndef Poedit_cat_operations_h
#define Poedit_cat_operations_h

#include "catalog.h"
#include "gexecute.h"


/// Summary data about the result of merging two catalogs.
struct MergeStats
{
    // Added and removed strings (as their summary representation, not the full items)
    std::vector<wxString> added;
    std::vector<wxString> removed;

    int changes_count() const { return int(added.size() + removed.size()); }

    // Any errors/warnings that occurred during the merge
    ParsedGettextErrors errors;
};


/// Resulting data from a merge operation.
struct MergeResult
{
    CatalogPtr updated_catalog;
    ParsedGettextErrors errors;

    explicit operator bool() const { return updated_catalog != nullptr; }
};


/**
    Calculates difference between a catalog and a reference ("upstream") one
    w.r.t. merging operation, i.e. different in source strings.
 */
extern void ComputeMergeStats(MergeStats& r, CatalogPtr catalog, CatalogPtr reference);


/**
    Merges catalog with a reference catalog, updating catalog with new strings
    present in @a reference and removing strings that are no longer present there.

    @note The returned updated_catalog may be the same as @a catalog, but it may also be
          a new object, possibly also @a reference. Don't make assumptions about it and
          always treat it as an entirely new object.

    @warning The @a reference object cannot be used after being passed to this function!
 */
extern MergeResult MergeCatalogWithReference(CatalogPtr catalog, CatalogPtr reference);

#endif // Poedit_cat_operations_h
