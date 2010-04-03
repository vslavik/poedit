/*
 *  This file is part of Poedit (http://www.poedit.net)
 *
 *  Copyright (C) 1999-2010 Vaclav Slavik
 *  Copyright (C) 2005 Olivier Sannier
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

#include "cat_sorting.h"

CatalogItemsComparator::CatalogItemsComparator(const Catalog& catalog)
    : m_catalog(catalog)
{
}

bool CatalogItemsComparator::operator()(int i, int j) const
{
    const CatalogItem& a = Item(i);
    const CatalogItem& b = Item(j);

    if ( !a.IsTranslated() && b.IsTranslated() )
        return true;
    if ( a.IsTranslated() && !b.IsTranslated() )
        return false;

    if ( a.GetValidity() == CatalogItem::Val_Invalid && b.GetValidity() != CatalogItem::Val_Invalid )
        return true;
    if ( a.GetValidity() != CatalogItem::Val_Invalid && b.GetValidity() == CatalogItem::Val_Invalid )
        return false;

    if ( a.IsFuzzy() && !b.IsFuzzy() )
        return true;
    if ( !a.IsFuzzy() && b.IsFuzzy() )
        return false;

    // As last resort, sort by position in file. Note that this means that
    // no two items are considered equal w.r.t. sort order; this ensures stable
    // ordering.
    return i < j;
}
