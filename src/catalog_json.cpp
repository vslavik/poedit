/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 2023-2025 Vaclav Slavik
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

#include "configuration.h"
#include "str_helpers.h"
#include "utility.h"

#include <wx/intl.h>
#include <wx/log.h>

#include <boost/algorithm/string.hpp>

#include <fstream>
#include <map>
#include <memory>
#include <sstream>


namespace
{

// Try to determine JSON file's formatting, i.e. line endings and identation, by inspecting the
// beginning of the file.
void DetectFileFormatting(std::ifstream& f, int& indent, char& indent_char, bool& dos_line_endings)
{
    // fallback defaults: compact representation with no indentation
    indent = -1;
    indent_char = ' ';
    dos_line_endings = false;

    for (int counter = 0; counter < 100; ++counter)
    {
        auto c = f.get();
        if (c == '\r' && f.peek() == '\n')
        {
            dos_line_endings = true;
        }
        else if (c == '\n')
        {
            // first newline found, indent=0 means "use newlines" in json::dump()
            indent = 0;
        }
        else if (c == ' ' || c == '\t')
        {
            if (indent >= 0)
            {
                // we're past the newline, identifying whitespace leading to the first "
                indent++;
                indent_char = (char)c; // ignore weirdnesses like mixed whitespace
            }
        }
        else if  (c == '"')
        {
            // reached first key-value content, which must be indented, so we're done
            break;
        }
    }
}

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
            return false; // for most variants it is detected from filename
        case Cap::UserComments:
        case Cap::FuzzyTranslations:
            return false;
    }
    return false; // silence VC++ warning
}


bool JSONCatalog::CanLoadFile(const wxString& extension)
{
    return extension == "json" || extension == "arb";
}


std::shared_ptr<JSONCatalog> JSONCatalog::Open(const wxString& filename)
{
    try
    {
        const auto ext = str::to_utf8(wxFileName(filename).GetExt().Lower());
        std::ifstream f(filename.fn_str(), std::ios::binary);
        auto data = json_t::parse(f);

        auto cat = CreateForJSON(std::move(data), ext);
        if (!cat)
            throw JSONUnrecognizedFileException();

        f.clear();
        f.seekg(0, std::ios::beg);
        DetectFileFormatting(f, cat->m_formatting.indent, cat->m_formatting.indent_char, cat->m_formatting.dos_line_endings);

        cat->Parse();

        return cat;
    }
    catch (json::exception& e)
    {
        throw JSONFileException(wxString::Format(_("Reading file content failed with the following error: %s"), e.what()));
    }
}


bool JSONCatalog::Save(const wxString& filename, bool /*save_mo*/,
                        ValidationResults& validation_results,
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
        std::ofstream f(tempfile.FileName().fn_str(), std::ios::binary);
        f << SaveToBuffer();
    }

    if ( !tempfile.Commit() )
    {
        wxLogError(_(L"Couldn’t save file %s."), filename.c_str());
        return false;
    }

    validation_results = Validate();

    SetFileName(filename);
    return true;
}


std::string JSONCatalog::SaveToBuffer()
{
    auto s = m_doc.dump(m_formatting.indent, m_formatting.indent_char, /*ensure_ascii=*/false);
    if (s.empty())
        return s; // shouldn't be possible...

    // all POSIX text files must end in newline, but json::dump() doesn't produce it
    if (s.back() != '\n')
        s.push_back('\n');

    if (m_formatting.dos_line_endings)
    {
        boost::replace_all(s, "\n", "\r\n");
    }
    return s;
}


class GenericJSONItem : public JSONCatalogItem
{
public:
    GenericJSONItem(int id, const std::string& key, json_t& node) : JSONCatalogItem(id, node)
    {
        m_string = str::to_wx(key);
        if (node.is_null())
        {
            m_translations.push_back(wxString());
            m_isTranslated = false;
        }
        else
        {
            auto trans = str::to_wx(node.get<std::string>());
            m_translations.push_back(trans);
            m_isTranslated = !trans.empty();
        }
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
            if (val.is_string() || val.is_null())
            {
                m_items.push_back(std::make_shared<GenericJSONItem>(++id, prefix + el.key(), val));
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


// Support for Flutter ARB files:
// https://github.com/google/app-resource-bundle/wiki/ApplicationResourceBundleSpecification

class FlutterItem : public GenericJSONItem
{
public:
    FlutterItem(int id, const std::string& key, json_t& node, const json_t *metadata)
        : GenericJSONItem(id, key, node), m_metadata(metadata)
    {
        if (metadata)
        {
            if (metadata->contains("context"))
            {
                m_hasContext = true;
                m_context = str::to_wx(metadata->at("context").get<std::string>());
            }
            if (metadata->contains("description"))
            {
                m_extractedComments.push_back(str::to_wx(metadata->at("description").get<std::string>()));
            }
        }
    }

protected:
    const json_t *m_metadata;
};


class FlutterCatalog : public JSONCatalog
{
public:
    using JSONCatalog::JSONCatalog;

    static bool SupportsFile(const json_t& doc, const std::string& extension)
    {
        return extension == "arb" || (doc.is_object() && doc.contains("@@locale"));
    }

    bool HasCapability(Catalog::Cap cap) const override
    {
        if (cap == Cap::LanguageSetting)
            return true;
        return JSONCatalog::HasCapability(cap);
    }

    void SetLanguage(Language lang) override
    {
        JSONCatalog::SetLanguage(lang);
        m_doc["@@locale"] = lang.Code();
    }

    void Parse() override
    {
        m_language = Language::TryParse(m_doc.value("@@locale", ""));

        int id = 0;
        ParseSubtree(id, m_doc, "");
    }

private:
    void ParseSubtree(int& id, json_t& node, const std::string& prefix)
    {
        // find() is O(n) in ordered_json and so looking up @key metadata nodes in the
        // main loop would result in O(n^2) complexity. Split the iteration into two and
        // first find metadata, making the fuction O(n*log(n)). Using ordered_map would
        // yield O(n), but with possibly high constant, so don't do that for now.
        std::map<std::string, json_t*> metadata;
        for (auto& el : node.items())
        {
            auto& key = el.key();
            if (!key.empty() && key.front() == '@')
                metadata.emplace(key.substr(1), &el.value());
        }

        for (auto& el : node.items())
        {
            auto& key = el.key();
            if (key.empty() || key.front() == '@')
                continue;

            auto& val = el.value();
            if (val.is_string())
            {
                auto mi = metadata.find(key);
                auto meta = (mi != metadata.end()) ? mi->second : nullptr;
                m_items.push_back(std::make_shared<FlutterItem>(++id, prefix + el.key(), val, meta));
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


// Support for WebExtension messages.json:
// https://developer.chrome.com/docs/extensions/mv3/i18n-messages/
// https://developer.chrome.com/docs/extensions/reference/i18n/
// https://developer.mozilla.org/en-US/docs/Mozilla/Add-ons/WebExtensions/Internationalization
// https://developer.mozilla.org/en-US/docs/Mozilla/Add-ons/WebExtensions/API/i18n/Locale-Specific_Message_reference

class WebExtensionCatalog : public JSONCatalog
{
public:
    using JSONCatalog::JSONCatalog;

    static bool SupportsFile(const json_t& doc)
    {
        if (!doc.is_object() || doc.empty())
            return false;

        auto first = doc.begin().value();
        return first.is_object() && first.contains("message");
    }

    void Parse() override
    {
        int id = 0;
        for (auto& el : m_doc.items())
        {
            auto& val = el.value();
            if (!val.is_object())
                throw JSONUnrecognizedFileException();

            m_items.push_back(std::make_shared<Item>(++id, el.key(), val));
        }

        if (m_items.empty())
            throw JSONUnrecognizedFileException();
    }

protected:
    class Item : public JSONCatalogItem
    {
    public:
        Item(int id, const std::string& key, json_t& node) : JSONCatalogItem(id, node)
        {
            m_string = str::to_wx(key);

            auto trans = str::to_wx(node.value("message", ""));
            m_translations.push_back(trans);
            m_isTranslated = !trans.empty();

            auto desc = node.value("description", "");
            if (!desc.empty())
                m_extractedComments.push_back(str::to_wx(desc));
        }

        void UpdateInternalRepresentation() override
        {
            m_node["message"] = str::to_utf8(GetTranslation());
        }

        std::string GetInternalFormatFlag() const override { return "ph-dollars"; }
    };
};


class LocalazyCatalog : public JSONCatalog
{
public:
    using JSONCatalog::JSONCatalog;

    static bool SupportsFile(const json_t& doc)
    {
        return doc.value("generator", "") == "Localazy";
    }

    bool HasCapability(Catalog::Cap cap) const override
    {
        if (cap == Cap::LanguageSetting)
            return true;
        return JSONCatalog::HasCapability(cap);
    }

    void Parse() override
    {
        m_header.SetHeader("X-Generator", m_doc.value("generator", ""));
        m_header.SetHeader("X-Localazy-Project", m_doc.value("projectId", ""));
        m_language = Language::FromLanguageTag(m_doc.value("targetLocale", ""));

        int id = 0;
        for (auto& file : m_doc.at("files"))
        {
            auto filename = file.value("name", "");

            for (auto& tr : file.at("translations"))
            {
                // FIXME: for now, skip plural forms and string lists
                if (!tr.at("source").is_string())
                    continue;

                m_items.push_back(std::make_shared<Item>(++id, filename, tr));
            }
        }
    }

    void SetLanguage(Language lang) override
    {
        JSONCatalog::SetLanguage(lang);
        m_doc["targetLocale"] = lang.LanguageTag();
    }

protected:
    class Item : public JSONCatalogItem
    {
    public:
        Item(int id, const std::string& filename, json_t& node)
            : JSONCatalogItem(id, node), m_filename(filename)
        {
            m_string = str::to_wx(node.at("source").get<std::string>());
            auto trans = str::to_wx(node.value("value", ""));
            m_translations.push_back(trans);
            m_isTranslated = !trans.empty();

            if (node.contains("meta"))
            {
                auto& meta = node["meta"];
                m_moreFlags = str::to_wx(meta.value("placeholders", ""));
                if (meta.contains("key"))
                    m_extractedComments.push_back("ID: " + str::to_wx(meta["key"].get<std::string>()));
            }

            if (node.contains("context"))
            {
                auto& ctxt = node["context"];
                auto desc = ctxt.value("description", "");

                if (ctxt.contains("description"))
                    m_extractedComments.push_back(str::to_wx(ctxt.at("description").get<std::string>()));

                if (ctxt.contains("screenshots"))
                {
                    if (!m_extractedComments.empty())
                        m_extractedComments.push_back("");
                    m_extractedComments.push_back(_("Screenshots:"));
                    for (auto& link : ctxt.at("screenshots"))
                        m_extractedComments.push_back(str::to_wx(link.get<std::string>()));
                }
            }
        }

        void UpdateInternalRepresentation() override
        {
            m_node["value"] = str::to_utf8(GetTranslation());
        }

        wxArrayString GetReferences() const override
        {
            wxArrayString refs;
            if (!m_filename.empty())
                refs.push_back(str::to_wx(m_filename));
            return refs;
        }

    private:
        std::string m_filename;
    };

};


std::shared_ptr<JSONCatalog> JSONCatalog::CreateForJSON(json_t&& doc, const std::string& extension)
{
    // try specialized implementations first:
    if (FlutterCatalog::SupportsFile(doc, extension))
        return std::shared_ptr<JSONCatalog>(new FlutterCatalog(std::move(doc)));
    if (WebExtensionCatalog::SupportsFile(doc))
        return std::shared_ptr<JSONCatalog>(new WebExtensionCatalog(std::move(doc)));
    if (LocalazyCatalog::SupportsFile(doc))
        return std::shared_ptr<JSONCatalog>(new LocalazyCatalog(std::move(doc)));

    // then fall back to generic:
    if (GenericJSONCatalog::SupportsFile(doc))
        return std::shared_ptr<JSONCatalog>(new GenericJSONCatalog(std::move(doc)));

    return nullptr;
}
