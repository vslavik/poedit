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

#include "catalog_json.h"

#include "qa_checks.h"
#include "configuration.h"
#include "str_helpers.h"
#include "utility.h"

#include <wx/intl.h>
#include <wx/log.h>

#include <boost/algorithm/string.hpp>

#include <fstream>
#include <memory>
#include <set>
#include <sstream>


namespace
{


} // anonymous namespace


class JSONUnrecognizedFileException : public JSONFileException
{
public:
    JSONUnrecognizedFileException()
        : JSONFileException(_(L"This JSON file isn’t a translations file and cannot be edited in Poedit."))
    {}
};


bool JSONCatalog::HasCapability(Catalog::Cap cap) const
{
    switch (cap)
    {
        case Cap::Translations:
            return true;
        case Cap::LanguageSetting:
            return false; // FIXME: for now
        case Cap::UserComments:
            return false;
        // FIXME: Need to show lack of support for fuzzy formats
    }
    return false; // silence VC++ warning
}


bool JSONCatalog::CanLoadFile(const wxString& extension)
{
    return extension == "json";
}


std::shared_ptr<JSONCatalog> JSONCatalog::Open(const wxString& filename)
{
    // FIXME: handle exceptions

    std::ifstream f(filename.fn_str());
    auto data = json_t::parse(f);

    auto cat = CreateForJSON(std::move(data));
    if (!cat)
        throw JSONUnrecognizedFileException();

    // TODO: detect line endings and indentation - see to the beginning of `f`, read 100 or so bytes, look for content after 1st {
    cat->m_fileName = filename;
    cat->Parse();

    return cat;
}


bool JSONCatalog::Save(const wxString& filename, bool /*save_mo*/,
                        ValidationResults& /*validation_results*/,
                        CompilationStatus& /*mo_compilation_status*/)
{
    if ( wxFileExists(filename) && !wxFile::Access(filename, wxFile::write) )
    {
        wxLogError(_(L"File “%s” is read-only and cannot be saved.\nPlease save it under different name."),
                   filename.c_str());
        return false;
    }

    TempOutputFileFor tempfile(filename);

    {
        std::ofstream f(tempfile.FileName().fn_str());
        f << SaveToBuffer();
    }

    if ( !tempfile.Commit() )
    {
        wxLogError(_(L"Couldn’t save file %s."), filename.c_str());
        return false;
    }

    m_fileName = filename;
    return true;
}


std::string JSONCatalog::SaveToBuffer()
{
    // FIXME: Preserve detected formatting (line endings, indent characters as much as possible)
    return m_doc.dump();
}


Catalog::ValidationResults JSONCatalog::Validate(bool)
{
    // FIXME: move this elsewhere, remove #include "qa_checks.h", configuration.h

    ValidationResults res;

    for (auto& i: m_items)
        i->ClearIssue();

    res.errors = 0;

#if wxUSE_GUI
    if (Config::ShowWarnings())
        res.warnings = QAChecker::GetFor(*this)->Check(*this);
#endif

    return res;
}


class GenericJSONItem : public JSONCatalogItem
{
public:
    GenericJSONItem(int id, const std::string& key, json_t& node) : JSONCatalogItem(id, node)
    {
        m_string = str::to_wx(key);
        auto trans = str::to_wx(node.get<std::string>());
        m_translations.push_back(trans);
        m_isTranslated = !trans.empty();
    }

    void UpdateInternalRepresentation() override
    {
        m_node = str::to_utf8(GetTranslation());
    }
};


class GenericJSONCatalog : public JSONCatalog
{
public:
    using JSONCatalog::JSONCatalog;

    static bool SupportsFile(const json_t& doc)
    {
        // note that parsing may still fail, this is just pre-flight
        return doc.is_object();
    }

    void Parse() override
    {
        int id = 0;
        ParseSubtree(id, m_doc, "");

        if (m_items.empty())
            throw JSONUnrecognizedFileException();
    }

private:
    void ParseSubtree(int& id, json_t& node, const std::string& prefix)
    {
        for (auto& el : node.items())
        {
            auto& val = el.value();
            if (val.is_string())
            {
                m_items.push_back(std::make_shared<GenericJSONItem>(++id, prefix + el.key(), el.value()));
            }
            else if (val.is_object())
            {
                ParseSubtree(id, val, prefix + el.key() + ".");
            }
            else
            {
                throw JSONUnrecognizedFileException();
            }
        }
    }
};


std::shared_ptr<JSONCatalog> JSONCatalog::CreateForJSON(json_t&& doc)
{
    if (GenericJSONCatalog::SupportsFile(doc))
        return std::shared_ptr<JSONCatalog>(new GenericJSONCatalog(std::move(doc)));

    return nullptr;
}
