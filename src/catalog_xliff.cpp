/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 2018-2025 Vaclav Slavik
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

// Flags required for correct parsing of XML files with no loss of information
// FIXME: This includes parse_eol, which is undesirable: it converts files to Unix
//        line endings on save. OTOH without it, we'd have to do the conversion
//        manually both ways when extracting _and_ editing text.
constexpr auto PUGI_PARSE_FLAGS = parse_full | parse_ws_pcdata | parse_fragment;

// Skip over a tag, starting at its '<' with forward iterator or '>' with reverse;
// return iterator right after the tag or end if malformed
template<typename Iter>
inline Iter skip_over_tag(Iter begin, Iter end)
{
    const char closing = (*begin == '<') ? '>' : '<';
    Iter i = begin;
    for (++i; i != end && *i != closing; ++i)
    {
        const char c = *i;
        if (c == '\'' || c == '"')
        {
            ++i;
            while (i != end && *i != c)
                ++i;
            if (i == end)
                return end;
        }
    }

    return (i == end) ? end : ++i;
}


// does the node have any <elements> as children?
inline bool has_child_elements(xml_node node)
{
    return node.find_child([](xml_node n){ return n.type() == node_element; });
}

inline bool is_self_closing(xml_node node)
{
    return node.type() == node_element && !node.first_child();
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

inline bool has_multiple_text_children(xml_node node)
{
    bool alreadyFoundOne = false;
    for (auto child = node.first_child(); child; child = child.next_sibling())
    {
        if (child.type() == node_pcdata || child.type() == node_cdata)
        {
            if (alreadyFoundOne)
                return true;
            else
                alreadyFoundOne = true;
        }
    }
    return false;
}

inline std::string get_node_text(xml_node node)
{
    // xml_node::text() returns the first text child, but that's not enough,
    // because some (weird) files in the wild mix text and CDATA content
    if (has_multiple_text_children(node))
    {
        std::string s;
        for (auto child = node.first_child(); child; child = child.next_sibling())
            if (child.type() == node_pcdata || child.type() == node_cdata)
                s.append(child.text().get());
        return s;
    }
    else
    {
        return node.text().get();
    }
}

inline void set_node_text(xml_node node, const std::string& text)
{
    // see get_node_text() for explanation
    if (has_multiple_text_children(node))
        remove_all_children(node);

    node.text() = text.c_str();
}

inline std::string get_node_text_or_markup(xml_node node, bool isPlainText)
{
    if (isPlainText)
        return get_node_text(node);
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
    auto s = get_node_text_or_markup(node, metadata.isPlainText);
    if (!metadata.isPlainText)
        apply_placeholders(s, metadata);
    return s;
}

bool set_node_text_with_metadata(xml_node node, std::string&& text, const XLIFFStringMetadata& metadata)
{
    if (metadata.isPlainText)
    {
        set_node_text(node, text);
        return true;
    }
    else
    {
        std::string s(std::move(text));
        for (auto& ph: metadata.substitutions)
            boost::replace_all(s, ph.placeholder, ph.markup);

        remove_all_children(node);
        auto result = node.append_buffer(s.c_str(), s.size(), PUGI_PARSE_FLAGS, encoding_utf8);
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

/// Check if a string contains only digit (e.g. "42")
inline bool is_numeric_only(const std::string& s)
{
    return std::all_of(s.begin(), s.end(), [](char c){ return c >= '0' && c <= '9'; });
}


inline void xliff_prefix_id(std::string& id, xml_node node, const char *nameAttr)
{
    std::string prefix;
    for (auto p = node.parent(); p; p = p.parent())
    {
        if (strcmp(p.name(), "group") == 0)
        {
            auto gid = p.attribute(nameAttr);
            if (!gid)
                gid = p.attribute("id");
            if (gid)
            {
                prefix.insert(0, " / ");
                prefix.insert(0, gid.value());
                continue; // go to grandparent
            }
        }
        // stop at first non-group or unnamed parent:
        break;
    }

    if (!prefix.empty())
        id.insert(0, prefix);
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
        extractedText = get_node_text_or_markup(node, metadata.isPlainText);
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

        if (kind == GROUP && is_self_closing(node))
            kind = SINGLE;

        auto markup = get_node_markup(node);
        if (m_foundMarkup.find(markup) != m_foundMarkup.end())
            return;
        m_foundMarkup.insert(markup);

        PlaceholderInfo phi {kind, id};
        std::string subst;

        switch (kind)
        {
            case SINGLE:
            {
                phi.markup = markup;
                subst = ExtractPlaceholderDisplay(node);
                if (subst.empty() || boost::all(subst, boost::is_space()))
                    subst = id;
                subst = PrettifyPlaceholder(subst);
                break;
            }
            case GROUP:
            {
                subst = "<g>";

                // Locate closing tag. Since we know this is a well-formed XML node,
                // it ends with </name> and searching for the last '<' gives us the position.
                auto opening_tag_end = skip_over_tag(markup.begin(), markup.end());
                auto closing_tag_start = skip_over_tag(markup.rbegin(), markup.rend());

                wxASSERT( closing_tag_start != markup.rend() );
                wxASSERT( closing_tag_start.base() > opening_tag_end );

                phi.markup.assign(markup.begin(), opening_tag_end);
                phi.markupClosing.assign(closing_tag_start.base(), markup.end());

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
            if (!ph.second.markupClosing.empty())
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
                    const bool hasClosingMarkup = !ph.second.markupClosing.empty();
                    std::string phclose({'<', '/', phtext[1], '>'});
                    while (removedPlaceholderMarkup.find(phtext) != std::string::npos ||
                           (hasClosingMarkup && removedPlaceholderMarkup.find(phclose) != std::string::npos))
                    {
                        phtext.insert(1, 1, phtext[1]);
                        if (hasClosingMarkup)
                            phclose.insert(2, 1, phclose[2]);
                    }
                    metadata.substitutions.push_back({phtext, ph.second.markup});
                    if (hasClosingMarkup)
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



XLIFFReadException::XLIFFReadException(const wxString& what)
    : XLIFFException(wxString::Format(_(L"Error while loading XLIFF file: %s"), what))
{}


XLIFFCatalogItem::document_lock::document_lock(XLIFFCatalogItem *parent)
    : std::lock_guard<std::mutex>(parent->m_owner.m_documentMutex)
{
}


bool XLIFFCatalog::HasCapability(Catalog::Cap cap) const
{
    switch (cap)
    {
        case Cap::Translations:
            return true;
        case Cap::LanguageSetting:
            return true;
        case Cap::UserComments:
            return false; // FIXME: for now
        case Cap::FuzzyTranslations:
            return true;
    }
    return false; // silence VC++ warning
}


bool XLIFFCatalog::CanLoadFile(const wxString& extension)
{
    return extension == "xlf" || extension == "xliff";
}


std::shared_ptr<XLIFFCatalog> XLIFFCatalog::OpenImpl(const wxString& filename, InstanceCreatorImpl& creator)
{
    xml_document doc;
    auto result = doc.load_file(filename.fn_str(), PUGI_PARSE_FLAGS);
    if (!result)
        BOOST_THROW_EXCEPTION(XLIFFReadException(result.description()));

    auto xliff_root = doc.child("xliff");
    std::string xliff_version = xliff_root.attribute("version").value();

    auto cat = creator.CreateFromDoc(std::move(doc), xliff_version);
    if (!cat)
        BOOST_THROW_EXCEPTION(XLIFFReadException(wxString::Format(_("unsupported version (%s)"), xliff_version)));

    cat->Parse(xliff_root);

    return cat;
}


std::shared_ptr<XLIFFCatalog> XLIFFCatalog::Open(const wxString& filename)
{
    struct Creator : public InstanceCreatorImpl
    {
        std::shared_ptr<XLIFFCatalog> CreateFromDoc(pugi::xml_document&& doc, const std::string& xliff_version) override
        {
            if (xliff_version == "1.0")
                return std::make_shared<XLIFF1Catalog>(std::move(doc), 0);
            else if (xliff_version == "1.1")
                return std::make_shared<XLIFF1Catalog>(std::move(doc), 1);
            else if (xliff_version == "1.2")
                return std::make_shared<XLIFF1Catalog>(std::move(doc), 2);
            else if (xliff_version == "2.0" || xliff_version == "2.1")
                return std::make_shared<XLIFF2Catalog>(std::move(doc));
            else
                return nullptr;
        }
    };

    Creator c;
    return OpenImpl(filename, c);
}


bool XLIFFCatalog::Save(const wxString& filename, bool /*save_mo*/,
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

    if ( !tempfile.Commit() )
    {
        wxLogError(_(L"Couldn’t save file %s."), filename.c_str());
        return false;
    }

    validation_results = Validate();

    SetFileName(filename);
    return true;
}


std::string XLIFFCatalog::SaveToBuffer()
{
    std::ostringstream s;
    m_doc.save(s, "\t", format_raw);
    return s.str();
}


std::string XLIFFCatalog::GetXPathValue(const char* xpath) const
{
    auto x = m_doc.child("xliff").select_node(xpath);
    auto v = x.attribute().value();
    if (v)
        return v;
     v = x.node().value();
     if (v)
         return v;
     return "";
}




class XLIFF12CatalogItem : public XLIFFCatalogItem
{
public:
    XLIFF12CatalogItem(XLIFF1Catalog& owner, int itemId, xml_node node) : XLIFFCatalogItem(owner, itemId, node)
    {
        auto source = node.child("source");

        XLIFF12MetadataExtractor extractor;
        source.traverse(extractor);
        m_metadata = std::move(extractor.metadata);

        m_string = str::to_wx(extractor.extractedText);

        std::string id = node.attribute("resname").value();
        if (id.empty())
            id = node.attribute("id").value();
        // some tools (e.g. Xcode, tool-id="com.apple.dt.xcode") use ID same as text; numeric only is useless to translator too
        if (!id.empty() && id != m_string && !is_numeric_only(id))
        {
            xliff_prefix_id(id, node, "resname");
            m_symbolicId = str::to_wx(id);
        }

        auto target = node.child("target");
        if (target)
        {
            auto trans_text = str::to_wx(get_node_text_with_metadata(target, m_metadata));
            m_translations.push_back(trans_text);
            m_isTranslated = !trans_text.empty();
            std::string state = target.attribute("state").value();
            if (state == "needs-adaptation" || state == "needs-l10n")
                m_isFuzzy = true;
            else if (m_isTranslated && (state == "new" || state == "needs-translation" || state == "needs-review-translation" || state == "needs-review-l10n"))
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
        document_lock lock(this);

        auto target = m_node.child("target");
        if (!target)
        {
            auto ws_after = m_node.first_child();
            auto prev = m_node.child("seg-source");
            if (!prev)
                prev = m_node.child("source");
            target = m_node.insert_child_after("target", prev);
            // indent the <target> tag in the same way <source> is indented under its parent:
            if (ws_after.type() == node_pcdata)
                m_node.insert_child_after(node_pcdata, prev).text() = ws_after.text().get();
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
            // Ensure the node is shown as <target></target> rather than <target/>.
            // The spec is unclear in this regard and the former is more expected.
            target.text() = "";
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
    bool extractedLanguage = false;

    for (auto file: root.children("file"))
    {
        // In XLIFF, embedded sub-files may have different languages, although it is
        // unclear how common this is practice. Poedit doesn't support this yet, so
        // take the first file's language only.
        if (!extractedLanguage)
        {
            m_sourceLanguage = Language::FromLanguageTag(file.attribute("source-language").value());
            m_language = Language::FromLanguageTag(file.attribute("target-language").value());
            extractedLanguage = true;
        }

        for (auto unit: file.select_nodes(".//trans-unit"))
        {
            auto node = unit.node();
            if (strcmp(node.attribute("translate").value(), "no") == 0)
                continue;

            if (m_subversion == 0)
                m_items.push_back(std::make_shared<XLIFF10CatalogItem>(*this, ++id, node));
            else
                m_items.push_back(std::make_shared<XLIFF12CatalogItem>(*this, ++id, node));
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
    XLIFF2CatalogItem(XLIFF2Catalog& owner, int itemId, xml_node node) : XLIFFCatalogItem(owner, itemId, node)
    {
        auto source = node.child("source");

        XLIFF2MetadataExtractor extractor;
        source.traverse(extractor);
        m_metadata = std::move(extractor.metadata);

        m_string = str::to_wx(extractor.extractedText);

        std::string id = unit().attribute("name").value();
        if (id.empty())
            id = unit().attribute("id").value();
        // some tools (e.g. Xcode, tool-id="com.apple.dt.xcode") use ID same as text; numeric only is useless to translator too
        if (!id.empty() && id != m_string && !is_numeric_only(id))
        {
            xliff_prefix_id(id, unit(), "name");
            m_symbolicId = str::to_wx(id);
        }

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
        document_lock lock(this);

        auto target = m_node.child("target");
        if (!target)
        {
            auto ws_after = m_node.first_child();
            auto source = m_node.child("source");
            target = m_node.insert_child_after("target", source);
            // indent the <target> tag in the same way <source> is indented under its parent:
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
            // Ensure the node is shown as <target></target> rather than <target/>.
            // The spec is unclear in this regard and the former is more expected.
            target.text() = "";
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
    m_sourceLanguage = Language::FromLanguageTag(root.attribute("srcLang").value());
    m_language = Language::FromLanguageTag(root.attribute("trgLang").value());

    int id = 0;
    for (auto segment: root.select_nodes(".//segment"))
    {
        auto node = segment.node();
        if (strcmp(node.parent().attribute("translate").value(), "no") == 0)
            continue;

        m_items.push_back(std::make_shared<XLIFF2CatalogItem>(*this, ++id, node));
    }
}


void XLIFF2Catalog::SetLanguage(Language lang)
{
    XLIFFCatalog::SetLanguage(lang);
    attribute(GetXMLRoot(), "trgLang") = lang.LanguageTag().c_str();
}
