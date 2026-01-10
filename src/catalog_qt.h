/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 2025-2026 Vaclav Slavik
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


#ifndef Poedit_catalog_qt_h
#define Poedit_catalog_qt_h

#include "catalog.h"
#include "errors.h"

#include "pugixml.h"

#include <mutex>
#include <vector>


class QtLinguistCatalog;


class QtLinguistException : public Exception
{
public:
    QtLinguistException(const wxString& what) : Exception(what) {}
};

class QtLinguistReadException : public QtLinguistException
{
public:
    QtLinguistReadException(const wxString& what);
};


class QtLinguistCatalogItem : public CatalogItem
{
public:
    QtLinguistCatalogItem(QtLinguistCatalog& owner, int id, pugi::xml_node node);
    QtLinguistCatalogItem(const CatalogItem&) = delete;

    wxString GetRawSymbolicId() const override { return m_symbolicId; }
    wxArrayString GetReferences() const override;

protected:
    void UpdateInternalRepresentation() override;

    struct document_lock : public std::lock_guard<std::mutex>
    {
        document_lock(QtLinguistCatalogItem *parent);
    };

protected:
    QtLinguistCatalog& m_owner;
    pugi::xml_node m_node;
    wxString m_symbolicId;
};


class QtLinguistCatalog : public Catalog
{
public:
    ~QtLinguistCatalog() {}

    bool HasCapability(Cap cap) const override;

    static bool CanLoadFile(const wxString& extension);
    wxString GetPreferredExtension() const override { return "ts"; }

    static std::shared_ptr<QtLinguistCatalog> Open(const wxString& filename);

    bool Save(const wxString& filename, bool save_mo,
              ValidationResults& validation_results,
              CompilationStatus& mo_compilation_status) override;

    std::string SaveToBuffer() override;

    Language GetLanguage() const override { return m_language; }
    void SetLanguage(Language lang) override;

    PluralFormsExpr GetPluralForms() const override;

    bool HasDeletedItems() const override { return m_hasDeletedItems; }
    void RemoveDeletedItems() override;

    pugi::xml_node GetXMLRoot() const { return m_doc.child("TS"); }

protected:
    QtLinguistCatalog(pugi::xml_document&& doc) : Catalog(Type::QT_LINGUIST), m_doc(std::move(doc))
    {
        m_sourceIsSymbolicID = false;
    }

    void Parse(pugi::xml_node root);
    void ParseSubtree(int& id, pugi::xml_node root, const wxString& context);

protected:
    std::mutex m_documentMutex;
    pugi::xml_document m_doc;

    Language m_language;
    bool m_hasDeletedItems = false;

    friend class QtLinguistCatalogItem;
};

#endif // Poedit_catalog_qt_h
