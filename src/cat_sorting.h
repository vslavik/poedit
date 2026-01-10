/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 2010-2026 Vaclav Slavik
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

#ifndef _CAT_SORTING_H_
#define _CAT_SORTING_H_

#include "catalog.h"
#include "unicode_helpers.h"

#include <memory>

/// Sort order information
struct SortOrder
{
    enum ByWhat
    {
        By_FileOrder,
        By_Source,
        By_Translation
    };

    SortOrder() : by(By_FileOrder), groupByContext(false), untransFirst(false), errorsFirst(true) {}

    /// Loads default sort order from config settings
    static SortOrder Default();

    /// Saves this sort order into config
    void Save();

    /// What are we sorting by
    ByWhat by;

    /// Group items by context?
    bool groupByContext;

    /// Do untranslated entries go first?
    bool untransFirst;

    /// Do entries with errors go first?
    bool errorsFirst;
};


/**
    Comparator for sorting catalog items by different criteria.
 */
class CatalogItemsComparator
{
public:
    /**
        Initializes comparator instance for given catalog.
     */
    CatalogItemsComparator(const Catalog& catalog, const SortOrder& order);

    CatalogItemsComparator(const CatalogItemsComparator&) = delete;
    CatalogItemsComparator& operator=(const CatalogItemsComparator&) = delete;

    bool operator()(int i, int j) const;

protected:
    const CatalogItem& Item(int i) const { return *m_catalog[i]; }

    // Pre-process given string and return it in a form efficient for comparing
    // with ICU collator. This does two things:
    //  1. Converts to UTF-16 (matters on non-Windows platforms where wchar_t is UTF-32)
    //  2. Perform substitution of accelerator characters in the string
    static str::UCharBuffer ConvertToSortKey(const wxString& a)
    {
        if (a.find_first_of(L"&_") == wxString::npos)
        {
            return str::to_icu(a);
        }
        else
        {
            wxString a_(a);
            a_.Replace("&", "");
            a_.Replace("_", "");

            auto buf = str::to_icu(a_);
            // on Windows to_icu() returns shallow view of the string, we need to make
            // a deep copy because a_ is a local temporary variable:
            buf.ensure_owned();
            return buf;
        }
    }

    static str::UCharBuffer ConvertToSortKey(const wxString&& a)
	{
		return std::move(ConvertToSortKey(a).ensure_owned());
	}

private:
    const Catalog& m_catalog;
    SortOrder m_order;
    std::unique_ptr<unicode::Collator> m_collator;
    std::vector<str::UCharBuffer> m_sortKeys;
};


#endif // _CAT_SORTING_H_
