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

#ifndef Poedit_qa_checks_h
#define Poedit_qa_checks_h

#include "catalog.h"

#include <memory>
#include <vector>


/// Interface for implementing quality checks
class QACheck
{
public:
    virtual ~QACheck() {}

    // Implementation has to implement one of the CheckXXX() methods,
    // it doesn't have to implement all of them.

    /**
        Checks given item for issues, possibly calling CatalogItem::SetIssue()
        to flag it as broken. Returns true if an issue was found, false otherwise.
     */
    virtual bool CheckItem(CatalogItemPtr item);

    /// A more convenient API, checking only strings
    virtual bool CheckString(CatalogItemPtr item, const wxString& source, const wxString& translation);
};


/// This class performs actual checking
class QAChecker
{
public:
    /// Returns checker suitable for given file
    static std::shared_ptr<QAChecker> GetFor(Catalog& catalog);

    /// Checks all items. Returns # of issues found.
    int Check(Catalog& catalog);

    /// Check a single item. Returns # of issues found.
    int Check(CatalogItemPtr item);

    // Low-level creation and setup:

    QAChecker() {}

    template<typename TCheck>
    void AddCheck() { AddCheck(std::make_shared<TCheck>()); }

    template<typename TCheck>
    void AddCheck(const Language& lang) { AddCheck(std::make_shared<TCheck>(lang)); }

    void AddCheck(std::shared_ptr<QACheck> c) { m_checks.push_back(c); }

protected:
    std::vector<std::shared_ptr<QACheck>> m_checks;
};

#endif // Poedit_qa_checks_h
