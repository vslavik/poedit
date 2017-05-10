/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 1999-2017 Vaclav Slavik
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

#include <unicode/unistr.h>
#include "str_helpers.h"

#include <wx/config.h>
#include <wx/log.h>

/*static*/ SortOrder SortOrder::Default()
{
    SortOrder order;

    wxString by = wxConfig::Get()->Read("/sort_by", "file-order");
    long ctxt = wxConfig::Get()->Read("/sort_group_by_context", 0L);
    long untrans = wxConfig::Get()->Read("/sort_untrans_first", 0L);
    long errors = wxConfig::Get()->Read("/sort_errors_first", 1L);

    if ( by == "source" )
        order.by = By_Source;
    else if ( by == "translation" )
        order.by = By_Translation;
    else
        order.by = By_FileOrder;

    order.groupByContext = (ctxt != 0);
    order.untransFirst = (untrans != 0);
    order.errorsFirst = (errors != 0);

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
    wxConfig::Get()->Write("/sort_group_by_context", groupByContext);
    wxConfig::Get()->Write("/sort_untrans_first", untransFirst);
    wxConfig::Get()->Write("/sort_errors_first", errorsFirst);
}


CatalogItemsComparator::CatalogItemsComparator(const Catalog& catalog, const SortOrder& order)
    : m_catalog(catalog), m_order(order)
{
    UErrorCode err = U_ZERO_ERROR;
    switch (m_order.by)
    {
        case SortOrder::By_Source:
            m_collator.reset(icu::Collator::createInstance(catalog.GetSourceLanguage().ToIcu(), err));
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

    if ( m_order.errorsFirst )
    {
        if ( a.HasIssue() && !b.HasIssue() )
            return true;
        else if ( !a.HasIssue() && b.HasIssue() )
            return false;

        if ( a.HasError() && !b.HasError() )
            return true;
        else if ( !a.HasError() && b.HasError() )
            return false;
    }

    if ( m_order.untransFirst )
    {
        if ( !a.IsTranslated() && b.IsTranslated() )
            return true;
        else if ( a.IsTranslated() && !b.IsTranslated() )
            return false;

        if ( a.IsFuzzy() && !b.IsFuzzy() )
            return true;
        else if ( !a.IsFuzzy() && b.IsFuzzy() )
            return false;
    }

    if ( m_order.groupByContext )
    {
        if ( a.HasContext() && !b.HasContext() )
            return true;
        else if ( !a.HasContext() && b.HasContext() )
            return false;
        else if ( a.HasContext() && b.HasContext() )
        {
            int r = CompareStrings(a.GetContext(), b.GetContext());
            if ( r != 0 )
                return r < 0;
        }
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

    if (m_collator)
    {
        UErrorCode err = U_ZERO_ERROR;
#if wxUSE_UNICODE_UTF8
        return m_collator->compareUTF8(a.wx_str(), b.wx_str(), err);
#elif SIZEOF_WCHAR_T == 2
        return m_collator->compare(a.wx_str(), a.length(), b.wx_str(), b.length(), err);
#else
        return m_collator->compare(str::to_icu(a), str::to_icu(b), err);
#endif
    }
    else
    {
        return a.CmpNoCase(b);
    }
}
