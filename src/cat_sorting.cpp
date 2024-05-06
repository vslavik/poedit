/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 1999-2023 Vaclav Slavik
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
    if (m_order.by == SortOrder::By_Translation && !m_catalog.HasCapability(Catalog::Cap::Translations))
        m_order.by = SortOrder::By_FileOrder;

    switch (m_order.by)
    {
        case SortOrder::By_Translation:
            m_collator.reset(new unicode::Collator(catalog.GetLanguage(), unicode::Collator::case_insensitive));
            break;

        case SortOrder::By_FileOrder:
            // we still need collator for e.g. comparing contexts, use source language for that
        case SortOrder::By_Source:
            m_collator.reset(new unicode::Collator(catalog.GetSourceLanguage(), unicode::Collator::case_insensitive));
            break;
    }

    // Prepare cache for faster comparison. ICU uses UTF-16 internally, we can significantly
    // speed up comparisons by doing string conversion in advance, in O(n) time and space, if
    // the platform's native representation is UTF-32 or -8 (which it is on non-Windows).
    // Moreover, the additional processing of removing accelerators is also done only once
    // on all platforms, resulting in massive speedups on Windows too.
    switch (m_order.by)
    {
        case SortOrder::By_Source:
            m_sortKeys.reserve(m_catalog.GetCount());
            for (auto& i: m_catalog.items())
                m_sortKeys.push_back(ConvertToSortKey(i->GetString()));
            break;

        case SortOrder::By_Translation:
            m_sortKeys.reserve(m_catalog.GetCount());
            for (auto& i: m_catalog.items())
                m_sortKeys.push_back(ConvertToSortKey(i->GetTranslation()));
            break;

        case SortOrder::By_FileOrder:
            break;
    }
}


bool CatalogItemsComparator::operator()(int i, int j) const
{
    const CatalogItem& a = Item(i);
    const CatalogItem& b = Item(j);

    if ( m_order.errorsFirst )
    {
        // hard errors always go first:
        if ( a.HasError() && !b.HasError() )
            return true;
        else if ( !a.HasError() && b.HasError() )
            return false;

        // warnings are more nuanced and should only be considered on non-fuzzy
        // entries (see https://github.com/vslavik/poedit/issues/611 for discussion):
        auto const a_shouldWarn = a.HasIssue() && !a.IsFuzzy();
        auto const b_shouldWarn = b.HasIssue() && !b.IsFuzzy();
        if ( a_shouldWarn && !b_shouldWarn )
            return true;
        else if ( !a_shouldWarn && b_shouldWarn )
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
            // we don't want to apply translation string pre-processing to contexts, use collator directly
            auto r = m_collator->compare(a.GetContext(), b.GetContext());
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
        case SortOrder::By_Translation:
        {
            auto r = m_collator->compare(m_sortKeys[i], m_sortKeys[j]);
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
