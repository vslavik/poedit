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

#include "catalog_resx.h"

#include "configuration.h"
#include "str_helpers.h"
#include "utility.h"

#include <wx/intl.h>
#include <wx/log.h>

#include <boost/algorithm/string.hpp>

#include <memory>
#include <set>
#include <sstream>

using namespace pugi;


namespace
{

bool check_resmimetype(const xml_node& node)
{
    for (auto header: node.children("resheader"))
    {
        auto name = header.attribute("name").value();
        if (strcmp(name, "resmimetype") == 0)
        {
            auto value = header.child("value");
            return value && strcmp(value.text().get(), "text/microsoft-resx") == 0;
        }
    }
    
    return false;
}

} // anonymous namespace


RESXReadException::RESXReadException(const wxString& what)
    : RESXException(wxString::Format(_(L"Error while loading RESX file: %s"), what))
{}


RESXCatalogItem::document_lock::document_lock(RESXCatalogItem *parent)
    : std::lock_guard<std::mutex>(parent->m_owner.m_documentMutex)
{
}


RESXCatalogItem::RESXCatalogItem(RESXCatalog& owner, int itemId, xml_node node) 
    : m_owner(owner), m_node(node)
{
    m_id = itemId;
    
    m_string = node.attribute("name").value();
    if (m_string.empty())
        BOOST_THROW_EXCEPTION(RESXReadException(_("The file is malformed.")));

    auto value = node.child("value");
    if (value)
    {
        auto trans_text = str::to_wx(get_node_text(value));
        m_translations.push_back(trans_text);
        m_isTranslated = !trans_text.empty();
    }
    else
    {
        m_translations.push_back("");
        m_isTranslated = false;
    }
    
    m_isFuzzy = false;
    
    auto comment = node.child("comment");
    if (comment)
    {
        auto commentText = get_node_text(comment);
        if (!commentText.empty())
        {
            m_extractedComments.push_back(str::to_wx(commentText));
        }
    }
}


void RESXCatalogItem::UpdateInternalRepresentation()
{
    wxASSERT(m_translations.size() == 1); // RESX doesn't support plurals
    
    // modifications in the pugixml tree can affect other nodes, we must lock the entire document
    document_lock lock(this);
    
    auto value = m_node.child("value");
    if (!value)
        value = m_node.append_child("value");

    set_node_text(value, str::to_utf8(GetTranslation()));
}


bool RESXCatalog::HasCapability(Catalog::Cap cap) const
{
    switch (cap)
    {
        case Cap::Translations:
            return true;
        case Cap::FuzzyTranslations:
            return false;
        case Cap::LanguageSetting:
            return false;
        case Cap::UserComments:
            return false;  // RESX <comment> elements are generated comments
    }
    return false;
}


bool RESXCatalog::CanLoadFile(const wxString& extension)
{
    return extension == "resx";
}


std::shared_ptr<RESXCatalog> RESXCatalog::Open(const wxString& filename)
{
    xml_document doc;
    auto result = doc.load_file(filename.fn_str(), PUGI_PARSE_FLAGS);
    if (!result)
        BOOST_THROW_EXCEPTION(RESXReadException(result.description()));

    auto root = doc.child("root");
    if (!root || !check_resmimetype(root))
        BOOST_THROW_EXCEPTION(RESXReadException(_("The file is malformed.")));

    auto cat = std::shared_ptr<RESXCatalog>(new RESXCatalog(std::move(doc)));
    cat->Parse(root);

    return cat;
}


void RESXCatalog::Parse(pugi::xml_node root)
{
    int id = 0;
    
    // Parse all <data> elements - these contain the translatable strings
    for (auto data : root.children("data"))
    {
        // Skip data elements that have type attribute (these are usually binary resources)
        if (data.attribute("type"))
            continue;
            
        // Skip data elements with xml:space="preserve" and no value (metadata)
        if (data.attribute("xml:space") && !data.child("value"))
            continue;
            
        std::string name = data.attribute("name").value();
        if (name.empty())
            continue;
            
        m_items.push_back(std::make_shared<RESXCatalogItem>(*this, ++id, data));
    }
}


bool RESXCatalog::Save(const wxString& filename, bool /*save_mo*/,
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

    m_doc.save_file(tempfile.FileName().fn_str(), "\t", format_raw);

    if (!tempfile.Commit())
    {
        wxLogError(_(L"Couldn’t save file %s."), filename.c_str());
        return false;
    }

    validation_results = Validate();

    SetFileName(filename);
    return true;
}


std::string RESXCatalog::SaveToBuffer()
{
    std::ostringstream s;
    m_doc.save(s, "\t", format_raw);
    return s.str();
}


void RESXCatalog::SetLanguage(Language lang)
{
    // RESX files don't store language information in the file itself
    // The language is typically determined by the filename (e.g., Resources.fr.resx)
    m_language = lang;
}
