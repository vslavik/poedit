/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 2018-2026 Vaclav Slavik
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

#include <mutex>
#include <vector>


class XLIFFCatalog;


class XLIFFException : public Exception
{
public:
    XLIFFException(const wxString& what) : Exception(what) {}
};

class XLIFFReadException : public XLIFFException
{
public:
    XLIFFReadException(const wxString& what);
};


// Metadata concerning XLIFF representation in Poedit, e.g. for placeholders
struct XLIFFStringMetadata
{
    XLIFFStringMetadata() : isPlainText(true) {}
    XLIFFStringMetadata(XLIFFStringMetadata&& other) = default;
    XLIFFStringMetadata& operator=(XLIFFStringMetadata&&) = default;
    XLIFFStringMetadata(const XLIFFStringMetadata&) = delete;
    void operator=(const XLIFFStringMetadata&) = delete;

    bool isPlainText;

    struct Subst
    {
        std::string placeholder;
        std::string markup;
    };
    std::vector<Subst> substitutions;
};


class XLIFFCatalogItem : public CatalogItem
{
public:
    XLIFFCatalogItem(XLIFFCatalog& owner, int id, pugi::xml_node node) : m_owner(owner), m_node(node)
        { m_id = id; }
    XLIFFCatalogItem(const CatalogItem&) = delete;

    wxString GetRawSymbolicId() const override { return m_symbolicId; }

protected:
    struct document_lock : public std::lock_guard<std::mutex>
    {
        document_lock(XLIFFCatalogItem *parent);
    };

protected:
    XLIFFCatalog& m_owner;
    pugi::xml_node m_node;
    XLIFFStringMetadata m_metadata;
    wxString m_symbolicId;
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

    Language GetLanguage() const override { return m_language; }
    void SetLanguage(Language lang) override { m_language = lang; }

    pugi::xml_node GetXMLRoot() const { return m_doc.child("xliff"); }
    std::string GetXPathValue(const char* xpath) const;

protected:
    XLIFFCatalog(pugi::xml_document&& doc)
        : Catalog(Type::XLIFF), m_doc(std::move(doc)) {}

    struct InstanceCreatorImpl
    {
        virtual ~InstanceCreatorImpl() {}
        virtual std::shared_ptr<XLIFFCatalog> CreateFromDoc(pugi::xml_document&& doc, const std::string& xliff_version) = 0;
    };

    static std::shared_ptr<XLIFFCatalog> OpenImpl(const wxString& filename, InstanceCreatorImpl& creator);

    virtual void Parse(pugi::xml_node root) = 0;

protected:
    std::mutex m_documentMutex;
    pugi::xml_document m_doc;
    Language m_language;

    friend class XLIFFCatalogItem;
};


class XLIFF1Catalog : public XLIFFCatalog
{
public:
    XLIFF1Catalog(pugi::xml_document&& doc, int subversion)
        : XLIFFCatalog(std::move(doc)),
          m_subversion(subversion)
    {}

    void SetLanguage(Language lang) override;

protected:
    void Parse(pugi::xml_node root) override;

    int m_subversion;
};


class XLIFF2Catalog : public XLIFFCatalog
{
public:
    XLIFF2Catalog(pugi::xml_document&& doc) : XLIFFCatalog(std::move(doc)) {}

    void SetLanguage(Language lang) override;

protected:
    void Parse(pugi::xml_node root) override;
};

#endif // Poedit_catalog_xliff_h
