/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 2018-2020 Vaclav Slavik
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
#include "crowdin_gui.h"

#include <wx/intl.h>
#include <wx/log.h>

#include <boost/algorithm/string.hpp>

#include <memory>
#include <mutex>
#include <set>
#include <sstream>

using namespace pugi;


namespace
{

// FIXME: This should be per-document mutex in XLIFFCatalog, but that is
//        currently not accessible from XLIFFCatalogItem, hence a global.
std::mutex gs_documentMutex;


inline bool has_child_elements(xml_node node)
{
    return node.find_child([](xml_node n){ return n.type() == node_element; });
}

inline std::string get_node_markup(xml_node node)
{
    std::ostringstream s;
    node.print(s, "", format_raw);
    return s.str();
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

inline xml_attribute attribute(xml_node node, const char *name)
{
    auto a = node.attribute(name);
    return a ? a : node.append_attribute(name);
}

inline std::string get_node_text(xml_node node, bool isPlainText)
{
    if (isPlainText)
        return node.text().get();
    else
        return get_subtree_markup(node);
}

inline void apply_placeholders(std::string& s, const XLIFFStringMetadata& metadata)
{
    for (auto& ph: metadata.substitutions)
        boost::replace_all(s, ph.markup, ph.placeholder);
}

inline std::string get_node_text_with_metadata(xml_node node, const XLIFFStringMetadata& metadata)
{
    auto s = get_node_text(node, metadata.isPlainText);
    if (!metadata.isPlainText)
        apply_placeholders(s, metadata);
    return s;
}

bool set_node_text_with_metadata(xml_node node, std::string&& text, const XLIFFStringMetadata& metadata)
{
    if (metadata.isPlainText)
    {
        node.text() = text.c_str();
        return true;
    }
    else
    {
        std::string s(std::move(text));
        for (auto& ph: metadata.substitutions)
            boost::replace_all(s, ph.placeholder, ph.markup);

        remove_all_children(node);
        auto result = node.append_buffer(s.c_str(), s.size(), parse_default, encoding_utf8);
        switch (result.status)
        {
            case status_no_document_element:
                node.text() = s.c_str();
                return true;
            case status_ok:
                return true;
            default:
                return false;
        }
    }
}



class MetadataExtractor : public pugi::xml_tree_walker
{
public:
    XLIFFStringMetadata metadata;
    std::string extractedText;

    bool begin(xml_node& node) override
    {
        const bool has_children = has_child_elements(node);
        metadata.isPlainText = !has_children;
        extractedText = get_node_text(node, metadata.isPlainText);
        return has_children;
    }

    bool for_each(pugi::xml_node& node) override
    {
        if (node.type() == node_element)
            OnTag(node.name(), node);
        return true;
    }

    bool end(xml_node&) override
    {
        if (!metadata.isPlainText)
            FinalizeMetadata();
        apply_placeholders(extractedText, metadata);
        return true;
    }

protected:
    virtual void OnTag(const std::string& name, pugi::xml_node node) = 0;
    virtual std::string ExtractPlaceholderDisplay(pugi::xml_node node) = 0;

    enum PlaceholderKind
    {
        SINGLE,
        GROUP
    };

    void AddPlaceholder(pugi::xml_node node, PlaceholderKind kind)
    {
        std::string id = node.attribute("id").value();
        if (id.empty())
            return;  // malformed - no ID, can't do anything about it

        PlaceholderInfo phi {kind, id};

        phi.markup = get_node_markup(node);
        if (m_foundMarkup.find(phi.markup) != m_foundMarkup.end())
            return;
        m_foundMarkup.insert(phi.markup);

        std::string subst;
        switch (kind)
        {
            case SINGLE:
            {
                subst = ExtractPlaceholderDisplay(node);
                if (subst.empty() || boost::all(subst, boost::is_space()))
                    subst = id;
                subst = PrettifyPlaceholder(subst);
                break;
            }
            case GROUP:
            {
                subst = "<g>";

                auto inner = get_subtree_markup(node);
                auto pos = phi.markup.find(inner);
                if (pos == std::string::npos)
                    return;  // something is very wrong, can't find inner content
                phi.markupClosing = phi.markup.substr(pos + inner.length());
                phi.markup = phi.markup.substr(0, pos);
                break;
            }
        }

        auto iexisting = m_placeholders.find(subst);
        if (iexisting == m_placeholders.end())
        {
            m_placeholders.emplace(subst, phi);
        }
        else
        {
            // conflict between duplicate placeholders; use ID instead of equiv-text
            auto existing = iexisting->second;
            m_placeholders.erase(iexisting);
            switch (kind)
            {
                case SINGLE:
                {
                    m_placeholders.emplace(PrettifyPlaceholder(existing.id), existing);
                    m_placeholders.emplace(PrettifyPlaceholder(id), phi);
                    break;
                }
                case GROUP:
                {
                    m_placeholders.emplace("<g id=\"" + existing.id + "\">", existing);
                    m_placeholders.emplace("<g id=\"" + id + "\">", phi);
                    break;
                }
            }
        }
    }

    void FinalizeMetadata()
    {
        // Construct substitutions table for metadata. While doing so, verify
        // that no placeholder conflicts with plain text, to ensure roundtripping
        // is safe.

        std::string removedPlaceholderMarkup = extractedText;
        for (auto& ph: m_placeholders)
        {
            boost::erase_all(removedPlaceholderMarkup, ph.second.markup);
            if (ph.second.kind == GROUP)
                boost::erase_all(removedPlaceholderMarkup, ph.second.markupClosing);
        }

        for (auto& ph: m_placeholders)
        {
            auto phtext = ph.first;
            switch (ph.second.kind)
            {
                case SINGLE:
                {
                    while (removedPlaceholderMarkup.find(phtext) != std::string::npos)
                        phtext = phtext.front() + phtext + phtext.back();
                    metadata.substitutions.push_back({phtext, ph.second.markup});
                    break;
                }
                case GROUP:
                {
                    std::string phclose({'<', '/', phtext[1], '>'});
                    while (removedPlaceholderMarkup.find(phtext) != std::string::npos || removedPlaceholderMarkup.find(phclose) != std::string::npos)
                    {
                        phtext.insert(1, 1, phtext[1]);
                        phclose.insert(2, 1, phclose[2]);
                    }
                    metadata.substitutions.push_back({phtext, ph.second.markup});
                    metadata.substitutions.push_back({phclose, ph.second.markupClosing});
                    break;
                }
            }
        }
    }

    inline std::string PrettifyPlaceholder(const std::string& s) const
    {
        if (s.empty())
            return "{}";

        const char f = s.front();
        const char b = s.back();
        if ((f == '{' && b == '}') || (f == '%' && b == '%') || (f == '<' && b == '>'))
            return s;  // {foo} {{foo}} %foo% <foo> </foo>
        else
            return '{' + s + '}';
    }

private:
    struct PlaceholderInfo
    {
        PlaceholderKind kind;
        std::string id;
        std::string markup, markupClosing;
    };

    std::map<std::string, PlaceholderInfo> m_placeholders;
    std::set<std::string> m_foundMarkup;
};


class XLIFF12MetadataExtractor : public MetadataExtractor
{
protected:
    void OnTag(const std::string& name, pugi::xml_node node) override
    {
        if (name == "x")
            AddPlaceholder(node, SINGLE);
        else if (name == "g")
            AddPlaceholder(node, GROUP);
    }

    std::string ExtractPlaceholderDisplay(pugi::xml_node node) override
    {
        return node.attribute("equiv-text").value();
    }
};


class XLIFF2MetadataExtractor : public MetadataExtractor
{
protected:
    void OnTag(const std::string& name, pugi::xml_node node) override
    {
        if (name == "ph")
            AddPlaceholder(node, SINGLE);
        else if (name == "pc")
            AddPlaceholder(node, GROUP);
    }

    std::string ExtractPlaceholderDisplay(pugi::xml_node node) override
    {
        auto disp = node.attribute("disp");
        if (disp)
            return disp.value();
        else
            return node.attribute("equiv").value();
    }
};



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
    if (xliff_version == "1.0")
        cat.reset(new XLIFF1Catalog(filename, std::move(doc), 0));
    else if (xliff_version == "1.1")
        cat.reset(new XLIFF1Catalog(filename, std::move(doc), 1));
    else if (xliff_version == "1.2")
        cat.reset(new XLIFF1Catalog(filename, std::move(doc), 2));
    else if (xliff_version == "2.0")
        cat.reset(new XLIFF2Catalog(filename, std::move(doc)));
    else
        throw XLIFFReadException(filename, wxString::Format(_("unsupported XLIFF version (%s)"), xliff_version));

    cat->SetFileName(filename);
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


Catalog::ValidationResults XLIFFCatalog::Validate(bool)
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

        XLIFF12MetadataExtractor extractor;
        source.traverse(extractor);
        m_metadata = std::move(extractor.metadata);

        m_string = str::to_wx(extractor.extractedText);

        // TODO: switch to textual IDs in CatalogItem
        std::string id = node.attribute("id").value();
        // some tools (e.g. Xcode, tool-id="com.apple.dt.xcode") use ID same as text
        if (!id.empty() && id != m_string)
            m_extractedComments.push_back("ID: " + str::to_wx(id));

        auto target = node.child("target");
        if (target)
        {
            auto trans_text = str::to_wx(get_node_text_with_metadata(target, m_metadata));
            m_translations.push_back(trans_text);
            m_isTranslated = !trans_text.empty();
            std::string state = target.attribute("state").value();
            if (state == "needs-adaptation" || state == "needs-l10n")
                m_isFuzzy = true;
            else if (m_isTranslated && (state == "new" || state == "needs-translation"))
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

        // modifications in the pugixml tree can affect other nodes, we must lock the entire document
        std::lock_guard<std::mutex> lock(gs_documentMutex);

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
            if (!set_node_text_with_metadata(target, str::to_utf8(trans), m_metadata))
            {
                // TRANSLATORS: Shown as error if a translation of XLIFF markup is not valid XML
                SetIssue(Issue::Error, _("Broken markup in translation string."));
            }
        }
        else // no translation
        {
            remove_all_children(target);
        }

        Impl_UpdateTargetState(target, !trans.empty(), m_isFuzzy);
    }

    // overridable for XLIFF 1.x differences
    virtual void Impl_UpdateTargetState(pugi::xml_node& target, bool isTranslated, bool isFuzzy)
    {
        target.remove_attribute("state-qualifier");

        if (isTranslated)
            attribute(target, "state") = isFuzzy ? "needs-l10n" : "translated";
        else
            attribute(target, "state") = "needs-translation";
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
            if (!file.empty())
                refs.push_back(file + line);
        }
        return refs;
    }
};


class XLIFF10CatalogItem : public XLIFF12CatalogItem
{
public:
    using XLIFF12CatalogItem::XLIFF12CatalogItem;

    void Impl_UpdateTargetState(pugi::xml_node& target, bool isTranslated, bool isFuzzy) override
    {
        if (isTranslated && !isFuzzy)
            target.remove_attribute("state");
        else
            attribute(target, "state") = "needs-translation";
    }
};

void XLIFF1Catalog::Parse(pugi::xml_node root)
{
    int id = 0;
    
    for (auto file: root.children("file"))
    {
        // TODO: Only first `file` node atthributes are used for now
        //       what works well in case of either all next are same within
        //       same `xliff` or if the only single `file` node is there
        //       (what is always the only case for downloaded from Crowdin)
        //       But should be taken into account and improved for probable
        //       more wide varietty of cases in the future.
        if(id == 0)
        {
            m_sourceLanguage = Language::TryParse(file.attribute("source-language").value());
            SetLanguage(Language::TryParse(file.attribute("target-language").value()));
        }
        
        for (auto unit: file.select_nodes(".//trans-unit"))
        {
            auto node = unit.node();
            if (strcmp(node.attribute("translate").value(), "no") == 0)
                continue;

            if (m_subversion == 0)
                m_items.push_back(std::make_shared<XLIFF10CatalogItem>(++id, node));
            else
                m_items.push_back(std::make_shared<XLIFF12CatalogItem>(++id, node));
        }
    }
}


void XLIFF1Catalog::SetLanguage(Language lang)
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

        XLIFF2MetadataExtractor extractor;
        source.traverse(extractor);
        m_metadata = std::move(extractor.metadata);

        m_string = str::to_wx(extractor.extractedText);

        // TODO: switch to textual IDs in CatalogItem
        std::string id = unit().attribute("id").value();
        // some tools (e.g. Xcode, tool-id="com.apple.dt.xcode") use ID same as text
        if (!id.empty() && id != m_string)
            m_extractedComments.push_back("ID: " + str::to_wx(id));

        auto target = node.child("target");
        if (target)
        {
            auto trans_text = str::to_wx(get_node_text_with_metadata(target, m_metadata));
            m_translations.push_back(trans_text);
            m_isTranslated = !trans_text.empty();
        }
        else
        {
            m_translations.push_back("");
        }

        std::string state = node.attribute("state").value();
        std::string substate = node.attribute("subState").value();
        m_isFuzzy = (m_isTranslated && state == "initial") || (substate == "poedit:fuzzy");

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

        // modifications in the pugixml tree can affect other nodes, we must lock the entire document
        std::lock_guard<std::mutex> lock(gs_documentMutex);

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

            if (!set_node_text_with_metadata(target, str::to_utf8(trans), m_metadata))
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
    SetLanguage(Language::TryParse(root.attribute("trgLang").value()));

    int id = 0;
    for (auto segment: root.select_nodes(".//segment"))
    {
        auto node = segment.node();
        if (strcmp(node.parent().attribute("translate").value(), "no") == 0)
            continue;

        m_items.push_back(std::make_shared<XLIFF2CatalogItem>(++id, node));
    }
}


void XLIFF2Catalog::SetLanguage(Language lang)
{
    XLIFFCatalog::SetLanguage(lang);
    attribute(GetXMLRoot(), "trgLang") = lang.LanguageTag().c_str();
}
