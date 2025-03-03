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

#include "cat_operations.h"

#include "catalog_po.h"
#include "concurrency.h"
#include "progress.h"

#include <set>


namespace
{

inline wxString ItemMergeSummary(const CatalogItemPtr& item)
{
    wxString s = item->GetRawString();
    if ( item->HasPlural() )
        s += " | " + item->GetRawPluralString();
    if ( item->HasContext() )
        s += wxString::Format(" [%s]", item->GetContext());

    return s;
}

} // anonymous namespace


void ComputeMergeStats(MergeStats& r, CatalogPtr po, CatalogPtr refcat)
{
    Progress progress(2);

    r.added.clear();
    r.removed.clear();

    // First collect all strings from both sides, then diff the sets.
    // Run the two sides in parallel for speed up on large files.

    std::set<wxString> strsThis, strsRef;

    auto collect1 = dispatch::async([&]
    {
        for (auto& i: po->items())
            strsThis.insert(ItemMergeSummary(i));
    });

    auto collect2 = dispatch::async([&]
    {
        for (auto& i: refcat->items())
            strsRef.insert(ItemMergeSummary(i));
    });

    collect1.get();
    collect2.get();
    progress.increment();

    auto add1 = dispatch::async([&]
    {
        for (auto& i: strsThis)
        {
            if (strsRef.find(i) == strsRef.end())
                r.removed.push_back(i);
        }
    });

    auto add2 = dispatch::async([&]
    {
        for (auto& i: strsRef)
        {
            if (strsThis.find(i) == strsThis.end())
                r.added.push_back(i);
        }
    });

    add1.get();
    add2.get();
    progress.increment();
}


MergeResult MergeCatalogWithReference(CatalogPtr catalog, CatalogPtr reference)
{
    auto po_catalog = std::dynamic_pointer_cast<POCatalog>(catalog);
    auto po_ref = std::dynamic_pointer_cast<POCatalog>(reference);
    if (!po_catalog || !po_ref)
        return {};

    if (!po_catalog->UpdateFromPOT(po_ref))
        return {};

    return {po_catalog};
}
