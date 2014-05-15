/*
 *  This file is part of Poedit (http://poedit.net)
 *
 *  Copyright (C) 1999-2014 Vaclav Slavik
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

#include "icuhelpers.h"

#include <wx/config.h>
#include <wx/log.h>

/*static*/ SortOrder SortOrder::Default()
{
    SortOrder order;

    wxString by = wxConfig::Get()->Read("/sort_by", "file-order");
    long untrans = wxConfig::Get()->Read("/sort_untrans_first", 1L);

    if ( by == "source" )
        order.by = By_Source;
    else if ( by == "translation" )
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
            bystr = "file-order";
            break;
        case By_Source:
            bystr = "source";
            break;
        case By_Translation:
            bystr = "translation";
            break;
    }

    wxConfig::Get()->Write("/sort_by", bystr);
    wxConfig::Get()->Write("/sort_untrans_first", untransFirst);
}


CatalogItemsComparator::CatalogItemsComparator(const Catalog& catalog, const SortOrder& order)
    : m_catalog(catalog), m_order(order)
{
    UErrorCode err = U_ZERO_ERROR;
    switch (m_order.by)
    {
        case SortOrder::By_Source:
            // TODO: allow non-English source languages too
            m_collator.reset(icu::Collator::createInstance(icu::Locale::getEnglish(), err));
            break;

        case SortOrder::By_Translation:
            m_collator.reset(icu::Collator::createInstance(catalog.GetLanguage().ToIcu(), err));
            break;

        case SortOrder::By_FileOrder:
            break;
    }

    if (!U_SUCCESS(err) || err == U_USING_FALLBACK_WARNING)
    {
        wxLogTrace("poedit", "warning: not using collation for %s (%s)",
                   catalog.GetLanguage().Code(), u_errorName(err));
    }

    if (m_collator)
    {
        // Case-insensitive comparison:
        m_collator->setStrength(icu::Collator::SECONDARY);
    }
}


bool CatalogItemsComparator::operator()(int i, int j) const
{
    const CatalogItem& a = Item(i);
    const CatalogItem& b = Item(j);

    if ( m_order.untransFirst )
    {
        if ( a.GetValidity() == CatalogItem::Val_Invalid && b.GetValidity() != CatalogItem::Val_Invalid )
            return true;
        if ( a.GetValidity() != CatalogItem::Val_Invalid && b.GetValidity() == CatalogItem::Val_Invalid )
            return false;

        if ( !a.IsTranslated() && b.IsTranslated() )
            return true;
        if ( a.IsTranslated() && !b.IsTranslated() )
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


int CatalogItemsComparator::CompareStrings(wxString a, wxString b) const
{
    a.Replace("&", "");
    a.Replace("_", "");

    b.Replace("&", "");
    b.Replace("_", "");

    UErrorCode err = U_ZERO_ERROR;
#if wxUSE_UNICODE_UTF8
    return m_collator->compareUTF8(a.wx_str(), b.wx_str(), err);
#elif SIZEOF_WCHAR_T == 2
    return m_collator->compare(a.wx_str(), a.length(), b.wx_str(), b.length(), err);
#else
    return m_collator->compare(ToIcuStr(a), ToIcuStr(b), err);
#endif
}
