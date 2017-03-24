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

#include "qa_checks.h"

#include <unicode/uchar.h>

#include <wx/translation.h>


// -------------------------------------------------------------
// QACheck implementations
// -------------------------------------------------------------

namespace QA
{

} // namespace QA


// -------------------------------------------------------------
// QACheck support code
// -------------------------------------------------------------

bool QACheck::CheckItem(CatalogItemPtr item)
{
    if (!item->GetTranslation().empty() && CheckString(item, item->GetString(), item->GetTranslation()))
        return true;

    if (item->HasPlural())
    {
        unsigned count = item->GetNumberOfTranslations();
        for (unsigned i = 1; i < count; i++)
        {
            auto t = item->GetTranslation(i);
            if (!t.empty() && CheckString(item, item->GetPluralString(), t))
                return true;
        }
    }

    return false;
}


bool QACheck::CheckString(CatalogItemPtr /*item*/, const wxString& /*source*/, const wxString& /*translation*/)
{
    wxFAIL_MSG("not implemented - must override CheckString OR CheckItem");
    return false;
}


// -------------------------------------------------------------
// QAChecker
// -------------------------------------------------------------

int QAChecker::Check(Catalog& catalog)
{
    // TODO: parallelize this (make async tasks for chunks of the catalog)
    //       doing it per-checker is MT-unsafe with API that calls SetIssue()!

    int issues = 0;

    for (auto& i: catalog.items())
        issues += Check(i);

    return issues;
}


int QAChecker::Check(CatalogItemPtr item)
{
    int issues = 0;

    for (auto& c: m_checks)
    {
        if (item->GetString().empty() || (item->HasPlural() && item->GetPluralString().empty()))
            continue;

        if (c->CheckItem(item))
            issues++;
    }

    return issues;
}


std::shared_ptr<QAChecker> QAChecker::GetFor(Catalog& /*catalog*/)
{
    auto c = std::make_shared<QAChecker>();
    return c;
}
