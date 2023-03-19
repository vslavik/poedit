/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 2023 Vaclav Slavik
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

#ifndef Poedit_catalog_json_h
#define Poedit_catalog_json_h

#include "catalog.h"
#include "errors.h"

#include "json.h"

#include <vector>


class JSONFileException : public Exception
{
public:
    JSONFileException(const wxString& what) : Exception(what) {}
};


class JSONCatalogItem : public CatalogItem
{
public:
    typedef ordered_json json_t;

    JSONCatalogItem(int id, json_t& node) : m_node(node)
    {
        m_id = id;
        m_isFuzzy = false; // not supported
    }

    JSONCatalogItem(const CatalogItem&) = delete;

    wxArrayString GetReferences() const override { return wxArrayString(); }

protected:
    json_t& m_node;
};


class JSONCatalog : public Catalog
{
public:
    typedef ordered_json json_t;

    ~JSONCatalog() {}

    bool HasCapability(Cap cap) const override;

    static bool CanLoadFile(const wxString& extension);
    wxString GetPreferredExtension() const override { return "json"; }

    static std::shared_ptr<JSONCatalog> Open(const wxString& filename);

    bool Save(const wxString& filename, bool save_mo,
              ValidationResults& validation_results,
              CompilationStatus& mo_compilation_status) override;

    std::string SaveToBuffer() override;

    ValidationResults Validate(bool wasJustLoaded) override;

    // FIXME: derive from filename too
    Language GetLanguage() const override { return m_language; }
    void SetLanguage(Language lang) override { m_language = lang; }

    // FIXME: PO specific
    bool HasDeletedItems() const override { return false;}
    void RemoveDeletedItems() override {}

protected:
    JSONCatalog(json_t&& doc) : Catalog(Type::JSON), m_doc(std::move(doc)) {}

    virtual void Parse() = 0;

private:
    static std::shared_ptr<JSONCatalog> CreateForJSON(json_t&& doc);

protected:
    json_t m_doc;
    Language m_language;

    struct FormattingRules
    {
        int indent;
        char indent_char;
        bool dos_line_endings;
    };
    FormattingRules m_formatting;
};


#endif // Poedit_catalog_json_h
