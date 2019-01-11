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

#include "catalog_xliff.h"

#include "qa_checks.h"
#include "configuration.h"
#include "str_helpers.h"
#include "utility.h"

#include <wx/intl.h>
#include <wx/log.h>

#include <memory>
#include <sstream>

using namespace pugi;


namespace
{

inline bool has_child_elements(xml_node node)
{
    return node.find_child([](xml_node n){ return n.type() == node_element; });
}

std::string get_subtree_markup(xml_node node)
{
    std::ostringstream s;
    for (auto c: node.children())
        c.print(s, "", format_raw);
    return s.str();
}

inline void remove_all_children(xml_node node)
{
    while (auto last = node.last_child())
        node.remove_child(last);
}

inline std::string get_node_text(xml_node node, bool isPlainText)
{
    if (isPlainText)
        return node.text().get();
    else
        return get_subtree_markup(node);
}

bool set_node_text(xml_node node, const std::string& text, bool isPlainText)
{
    if (isPlainText)
    {
        node.text() = text.c_str();
        return true;
    }
    else
    {
        remove_all_children(node);
        auto result = node.append_buffer(text.c_str(), text.size(), parse_default, encoding_utf8);
        return result.status == status_ok;
    }
}

inline xml_attribute attribute(xml_node node, const char *name)
{
    auto a = node.attribute(name);
    return a ? a : node.append_attribute(name);
}

} // anonymous namespace



XLIFFReadException::XLIFFReadException(const wxString& filename, const wxString& what)
    : XLIFFException(wxString::Format(_(L"Error loading file “%s”: %s."), filename, what))
{}


bool XLIFFCatalog::HasCapability(Catalog::Cap cap) const
{
    switch (cap)
    {
        case Cap::Translations:
            return true;
        case Cap::LanguageSetting:
            return false; // FIXME: for now
        case Cap::UserComments:
            return false; // FIXME: for now
    }
    return false; // silence VC++ warning
}


bool XLIFFCatalog::CanLoadFile(const wxString& extension)
{
    return extension == "xlf" || extension == "xliff";
}


std::shared_ptr<XLIFFCatalog> XLIFFCatalog::Open(const wxString& filename)
{
    constexpr auto parse_flags = parse_full | parse_ws_pcdata | parse_fragment;

    xml_document doc;
    auto result = doc.load_file(filename.fn_str(), parse_flags);
    if (!result)
        throw XLIFFReadException(filename, result.description());

    std::shared_ptr<XLIFFCatalog> cat;

    auto xliff_root = doc.child("xliff");
    std::string xliff_version = xliff_root.attribute("version").value();
    if (xliff_version == "1.2")
        cat.reset(new XLIFF12Catalog(filename, std::move(doc)));
    else if (xliff_version == "2.0")
        cat.reset(new XLIFF2Catalog(filename, std::move(doc)));
    else
        throw XLIFFReadException(filename, wxString::Format(_("unsupported XLIFF version (%s)"), xliff_version));

    cat->Parse(xliff_root);

    return cat;
}


bool XLIFFCatalog::Save(const wxString& filename, bool /*save_mo*/,
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

    m_doc.save_file(tempfile.FileName().fn_str(), "\t", format_raw);

    if ( !tempfile.Commit() )
    {
        wxLogError(_(L"Couldn’t save file %s."), filename.c_str());
        return false;
    }

    m_fileName = filename;
    return true;
}


std::string XLIFFCatalog::SaveToBuffer()
{
    std::ostringstream s;
    m_doc.save(s, "\t", format_raw);
    return s.str();
}


Catalog::ValidationResults XLIFFCatalog::Validate()
{
    // FIXME: move this elsewhere, remove #include "qa_checks.h", configuration.h

    ValidationResults res;

    for (auto& i: m_items)
        i->ClearIssue();

    res.errors = 0;

    if (Config::ShowWarnings())
        res.warnings = QAChecker::GetFor(*this)->Check(*this);

    return res;
}






class XLIFF12CatalogItem : public XLIFFCatalogItem
{
public:
    XLIFF12CatalogItem(int itemId, xml_node node) : XLIFFCatalogItem(itemId, node)
    {
        auto source = node.child("source");
        m_isPlainText = !has_child_elements(source);
        m_string = str::to_wx(get_node_text(source, m_isPlainText));

        // TODO: switch to textual IDs in CatalogItem
        std::string id = node.attribute("id").value();
        // some tools (e.g. Xcode, tool-id="com.apple.dt.xcode") use ID same as text
        if (!id.empty() && id != m_string)
            m_extractedComments.push_back("ID: " + str::to_wx(id));

        auto target = node.child("target");
        if (target)
        {
            auto trans_text = str::to_wx(get_node_text(target, m_isPlainText));
            m_translations.push_back(trans_text);
            m_isTranslated = !trans_text.empty();
            std::string state = target.attribute("state").value();
            if (state == "needs-adaptation" || state == "needs-l10n")
                m_isFuzzy = true;
        }
        else
        {
            m_translations.push_back("");
        }

        for (auto note: node.children("note"))
        {
            std::string noteText = note.text().get();
            if (noteText == "No comment provided by engineer.")  // Xcode does that
                continue;

            if (!m_extractedComments.empty())
                m_extractedComments.push_back("");
            m_extractedComments.push_back(str::to_wx(noteText));
        }
    }

    void UpdateInternalRepresentation() override
    {
        wxASSERT( m_translations.size() == 1 ); // no plurals

        auto target = m_node.child("target");
        if (!target)
        {
            auto ws_after = m_node.first_child();
            auto source = m_node.child("source");
            target = m_node.insert_child_after("target", source);
            // add appropriate padding:
            if (ws_after.type() == node_pcdata)
                m_node.insert_child_after(node_pcdata, source).text() = ws_after.text().get();
        }

        auto trans = GetTranslation();
        if (!trans.empty())
        {
            target.remove_attribute("state-qualifier");
            attribute(target, "state") = m_isFuzzy ? "needs-adaptation" : "translated";
            if (!set_node_text(target, str::to_utf8(trans), m_isPlainText))
            {
                // TRANSLATORS: Shown as error if a translation of XLIFF markup is not valid XML
                SetIssue(Issue::Error, _("Broken markup in translation string."));
            }
        }
        else // no translation
        {
            attribute(target, "state") = "needs-translation";
            target.remove_attribute("state-qualifier");
            remove_all_children(target);
        }
    }

    wxArrayString GetReferences() const override
    {
        wxArrayString refs;
        for (auto loc: m_node.select_nodes(".//context-group[@purpose='location']"))
        {
            wxString file, line;
            for (auto ctxt: loc.node().children("context"))
            {
                auto type = ctxt.attribute("context-type").value();
                if (strcmp(type, "sourcefile") == 0)
                    file = str::to_wx(ctxt.text().get());
                else if (strcmp(type, "linenumber") == 0)
                    line = ":" + str::to_wx(ctxt.text().get());
            }
            refs.push_back(file + line);
        }
        return refs;
    }
};


void XLIFF12Catalog::Parse(pugi::xml_node root)
{
    int id = 0;
    for (auto file: root.children("file"))
    {
        m_sourceLanguage = Language::TryParse(file.attribute("source-language").value());
        m_language = Language::TryParse(file.attribute("target-language").value());
        for (auto unit: file.select_nodes(".//trans-unit"))
            m_items.push_back(std::make_shared<XLIFF12CatalogItem>(++id, unit.node()));
    }
}


void XLIFF12Catalog::SetLanguage(Language lang)
{
    XLIFFCatalog::SetLanguage(lang);

    for (auto file: GetXMLRoot().children("file"))
    {
        attribute(file, "target-language") = lang.LanguageTag().c_str();
    }
}





class XLIFF2CatalogItem : public XLIFFCatalogItem
{
public:
    XLIFF2CatalogItem(int itemId, xml_node node) : XLIFFCatalogItem(itemId, node)
    {
        auto source = node.child("source");
        m_isPlainText = !has_child_elements(source);
        m_string = str::to_wx(get_node_text(source, m_isPlainText));

        // TODO: switch to textual IDs in CatalogItem
        std::string id = unit().attribute("id").value();
        // some tools (e.g. Xcode, tool-id="com.apple.dt.xcode") use ID same as text
        if (!id.empty() && id != m_string)
            m_extractedComments.push_back("ID: " + str::to_wx(id));

        auto target = node.child("target");
        if (target)
        {
            auto trans_text = str::to_wx(get_node_text(target, m_isPlainText));
            m_translations.push_back(trans_text);
            m_isTranslated = !trans_text.empty();
        }
        else
        {
            m_translations.push_back("");
        }

        std::string state = node.attribute("subState").value();
        m_isFuzzy = (state == "poedit:fuzzy");

        for (auto note: unit().select_nodes(".//note[not(@category='location')]"))
        {
            std::string noteText = note.node().text().get();

            if (!m_extractedComments.empty())
                m_extractedComments.push_back("");
            m_extractedComments.push_back(str::to_wx(noteText));
        }
    }

    void UpdateInternalRepresentation() override
    {
        wxASSERT( m_translations.size() == 1 ); // no plurals

        auto target = m_node.child("target");
        if (!target)
        {
            auto ws_after = m_node.first_child();
            auto source = m_node.child("source");
            target = m_node.insert_child_after("target", source);
            // add appropriate padding:
            if (ws_after.type() == node_pcdata)
                m_node.insert_child_after(node_pcdata, source).text() = ws_after.text().get();
        }

        auto trans = GetTranslation();
        if (!trans.empty())
        {
            attribute(m_node, "state") = "translated";
            if (m_isFuzzy)
                attribute(m_node, "subState") = "poedit:fuzzy";
            else
                m_node.remove_attribute("subState");

            if (!set_node_text(target, str::to_utf8(trans), m_isPlainText))
            {
                // TRANSLATORS: Shown as error if a translation of XLIFF markup is not valid XML
                SetIssue(Issue::Error, _("Broken markup in translation string."));
            }
        }
        else // no translation
        {
            m_node.remove_attribute("state");
            m_node.remove_attribute("subState");
            remove_all_children(target);
        }
    }

    wxArrayString GetReferences() const override
    {
        wxArrayString refs;
        for (auto note: unit().select_nodes(".//note[@category='location']"))
        {
            refs.push_back(str::to_wx(note.node().text().get()));
        }
        return refs;
    }


protected:
    xml_node unit() const { return m_node.parent(); }
};


void XLIFF2Catalog::Parse(pugi::xml_node root)
{
    m_sourceLanguage = Language::TryParse(root.attribute("srcLang").value());
    m_language = Language::TryParse(root.attribute("trgLang").value());

    int id = 0;
    for (auto segment: root.select_nodes(".//segment"))
        m_items.push_back(std::make_shared<XLIFF2CatalogItem>(++id, segment.node()));
}


void XLIFF2Catalog::SetLanguage(Language lang)
{
    XLIFFCatalog::SetLanguage(lang);
    attribute(GetXMLRoot(), "trgLang") = lang.LanguageTag().c_str();
}
