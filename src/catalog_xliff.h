/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 2018 Vaclav Slavik
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

#ifndef Poedit_catalog_xliff_h
#define Poedit_catalog_xliff_h

#include "catalog.h"
#include "errors.h"

#include "pugixml.h"


class XLIFFException : public Exception
{
public:
    XLIFFException(const wxString& what) : Exception(what) {}
};

class XLIFFReadException : public XLIFFException
{
public:
    XLIFFReadException(const wxString& filename, const wxString& what);
};


class XLIFFCatalogItem : public CatalogItem
{
public:
    XLIFFCatalogItem(int id, pugi::xml_node node) : m_node(node), m_isPlainText(true)
        { m_id = id; }
    XLIFFCatalogItem(const CatalogItem&) = delete;

protected:
    pugi::xml_node m_node;
    bool m_isPlainText;
};



class XLIFFCatalog : public Catalog
{
public:
    ~XLIFFCatalog() {}

    bool HasCapability(Cap cap) const override;

    static bool CanLoadFile(const wxString& extension);
    wxString GetPreferredExtension() const override { return "xlf"; }

    static std::shared_ptr<XLIFFCatalog> Open(const wxString& filename);

    bool Save(const wxString& filename, bool save_mo,
              ValidationResults& validation_results,
              CompilationStatus& mo_compilation_status) override;

    std::string SaveToBuffer() override;

    ValidationResults Validate() override;

    Language GetLanguage() const override { return m_language; }
    void SetLanguage(Language lang) override { m_language = lang; }

    // FIXME: PO specific
    bool HasDeletedItems() const override { return false;}
    void RemoveDeletedItems() override {}

    pugi::xml_node GetXMLRoot() const { return m_doc.child("xliff"); }

protected:
    XLIFFCatalog(const wxString& filename, pugi::xml_document&& doc)
        : Catalog(Type::XLIFF), m_doc(std::move(doc)) { m_fileName = filename; }

    virtual void Parse(pugi::xml_node root) = 0;

protected:
    pugi::xml_document m_doc;
    Language m_language;
};


class XLIFF12Catalog : public XLIFFCatalog
{
public:
    XLIFF12Catalog(const wxString& filename, pugi::xml_document&& doc)
        : XLIFFCatalog(filename, std::move(doc)) {}

    void SetLanguage(Language lang) override;

protected:
    void Parse(pugi::xml_node root) override;
};


class XLIFF2Catalog : public XLIFFCatalog
{
public:
    XLIFF2Catalog(const wxString& filename, pugi::xml_document&& doc)
        : XLIFFCatalog(filename, std::move(doc)) {}

    void SetLanguage(Language lang) override;

protected:
    void Parse(pugi::xml_node root) override;
};

#endif // Poedit_catalog_xliff_h
