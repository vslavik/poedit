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

#include <wx/config.h>

/*static*/ SortOrder SortOrder::Default()
{
    SortOrder order;

    wxString by = wxConfig::Get()->Read(_T("/sort_by"), _T("file-order"));
    long untrans = wxConfig::Get()->Read(_T("/sort_untrans_first"), 1L);

    if ( by == _T("source") )
        order.by = By_Source;
    else if ( by == _T("translation") )
        order.by = By_Translation;
    else
        order.by = By_FileOrder;

    order.untransFirst = (untrans != 0);

    return order;
}


void SortOrder::Save()
{
    wxString bystr;
    switch ( by )
    {
        case By_FileOrder:
            bystr = _T("file-order");
            break;
        case By_Source:
            bystr = _T("source");
            break;
        case By_Translation:
            bystr = _T("translation");
            break;
    }

    wxConfig::Get()->Write(_T("/sort_by"), bystr);
    wxConfig::Get()->Write(_T("/sort_untrans_first"), untransFirst);
}


bool CatalogItemsComparator::operator()(int i, int j) const
{
    const CatalogItem& a = Item(i);
    const CatalogItem& b = Item(j);

    if ( m_order.untransFirst )
    {
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
    }

    switch ( m_order.by )
    {
        case SortOrder::By_FileOrder:
        {
            return i < j;
        }

        case SortOrder::By_Source:
        {
            int r = CompareStrings(a.GetString(), b.GetString());
            if ( r != 0 )
                return r < 0;
            break;
        }

        case SortOrder::By_Translation:
        {
            int r = CompareStrings(a.GetTranslation(), b.GetTranslation());
            if ( r != 0 )
                return r < 0;
            break;
        }
    }

    // As last resort, sort by position in file. Note that this means that
    // no two items are considered equal w.r.t. sort order; this ensures stable
    // ordering.
    return i < j;
}


int CatalogItemsComparator::CompareStrings(const wxString& a, const wxString& b) const
{
    // TODO: * use ICU for correct ordering
    //       * use natural sort (for numbers)
    //       * use ICU for correct case insensitivity
    //       * ignore accelerators, especially at the beginning
    return a.CmpNoCase(b);
}
