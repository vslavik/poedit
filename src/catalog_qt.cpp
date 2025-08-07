/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 2025 Vaclav Slavik
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

#include "catalog_qt.h"

#include "configuration.h"
#include "str_helpers.h"
#include "utility.h"

#include <wx/intl.h>
#include <wx/log.h>
#include <wx/tokenzr.h> // FIXME

#include <regex>
#include <sstream>

using namespace pugi;

namespace
{

// FIXME: This is all sorts of horrid and duplicates CommentDialog code.
//        Proper solution is to remove this from comment storage, update POCatalog code,
//        and just use plain values in Set/GetComment(), not #-prefixed.

wxString RemoveStartHashFromComment(const wxString& comment)
{
    wxString tmpComment;
    wxStringTokenizer tkn(comment, "\n\r");
    while (tkn.HasMoreTokens())
        tmpComment << tkn.GetNextToken().Mid(2) << "\n";
    return tmpComment.Strip(wxString::both);
}

wxString AddStartHashToComment(const wxString& comment)
{
    wxString tmpComment;
    wxStringTokenizer tkn(comment, "\n\r");
    while (tkn.HasMoreTokens())
        tmpComment << "# " << tkn.GetNextToken() << "\n";
    return tmpComment;
}

} // anonymous namespace


QtLinguistReadException::QtLinguistReadException(const wxString& what)
    : QtLinguistException(wxString::Format(_(L"Error while loading Qt translation file: %s"), what))
{}


QtLinguistCatalogItem::document_lock::document_lock(QtLinguistCatalogItem *parent)
    : std::lock_guard<std::mutex>(parent->m_owner.m_documentMutex)
{
}


QtLinguistCatalogItem::QtLinguistCatalogItem(QtLinguistCatalog& owner, int itemId, xml_node node)
    : m_owner(owner), m_node(node)
{
    m_id = itemId;

    auto messageId = node.attribute("id").value();
    if (*messageId)
        m_symbolicId = str::to_wx(messageId);

    auto source = node.child("source");
    if (source)
    {
        auto sourceText = get_node_text(source);
        m_string = str::to_wx(sourceText);

        static std::regex s_formatString(R"(%L?(\d\d?|n))", std::regex_constants::ECMAScript | std::regex_constants::optimize);
        if (std::regex_search(sourceText, s_formatString))
            m_moreFlags = ", qt-format";
    }

    auto oldsource = node.child("oldsource");
    if (oldsource)
    {
        m_oldMsgid.push_back(str::to_wx(get_node_text(oldsource)));
    }

    auto translation = node.child("translation");
    if (translation)
    {
        auto trans_text = str::to_wx(get_node_text(translation));
        m_translations.push_back(trans_text);
        m_isTranslated = !trans_text.empty();

        auto type = translation.attribute("type").value();
        if (m_isTranslated && strcmp(type, "unfinished") == 0)
        {
            m_isFuzzy = true;
        }
    }
    else
    {
        m_translations.push_back(wxEmptyString);
        m_isTranslated = false;
        m_isFuzzy = false;
    }

    // Qt uses <comment> for disambiguation, documented as msgctxt equivalent:
    auto comment = node.child("comment");
    if (comment)
        SetContext(str::to_wx(get_node_text(comment)));

    // Actual comments:
    auto extracomment = node.child("extracomment");
    if (extracomment)
    {
        std::string commentText = get_node_text(extracomment);
        m_extractedComments.push_back(str::to_wx(commentText));
    }

    auto translatorcomment = node.child("translatorcomment");
    if (translatorcomment)
    {
        m_comment = AddStartHashToComment(str::to_wx(get_node_text(translatorcomment)));
    }
}


wxArrayString QtLinguistCatalogItem::GetReferences() const
{
    wxArrayString refs;

    for (auto location: m_node.children("location"))
    {
        auto filename = location.attribute("filename").value();
        if (*filename)
        {
            wxString ref = str::to_wx(filename);

            auto line = location.attribute("line").value();
            if (*line > 0)
                ref += wxString::Format(wxT(":%s"), str::to_wx(line));
            refs.Add(ref);
        }
    }

    return refs;
}


void QtLinguistCatalogItem::UpdateInternalRepresentation()
{
    // modifications in the pugixml tree can affect other nodes, we must lock the entire document
    document_lock lock(this);

    auto translation = m_node.child("translation");

    if (!translation)
    {
        auto source = m_node.child("source");
        if (source)
            translation = m_node.insert_child_after("translation", source);
        else
            translation = m_node.append_child("translation");
    }

    if (m_isFuzzy || !m_isTranslated)
    {
        attribute(translation, "type") = "unfinished";
    }
    else
    {
        translation.remove_attribute("type");
    }

    set_node_text(translation, str::to_utf8(GetTranslation()));

    if (HasComment())
    {
        auto comment = m_node.child("translatorcomment");
        if (!comment)
            comment = m_node.append_child("translatorcomment");
        set_node_text(comment, str::to_utf8(RemoveStartHashFromComment(GetComment())));
    }
    else
    {
        m_node.remove_child("translatorcomment");
    }
}


bool QtLinguistCatalog::HasCapability(Catalog::Cap cap) const
{
    switch (cap)
    {
        case Cap::Translations:
        case Cap::FuzzyTranslations:
        case Cap::LanguageSetting:
        case Cap::UserComments:
            return true;
    }

    return false;
}


bool QtLinguistCatalog::CanLoadFile(const wxString& extension)
{
    return extension == "ts";
}


std::shared_ptr<QtLinguistCatalog> QtLinguistCatalog::Open(const wxString& filename)
{
    xml_document doc;
    auto result = doc.load_file(filename.fn_str(), PUGI_PARSE_FLAGS);
    if (!result)
        BOOST_THROW_EXCEPTION(QtLinguistReadException(result.description()));

    auto root = doc.child("TS");
    if (!root)
        BOOST_THROW_EXCEPTION(QtLinguistReadException(_("The file is malformed.")));

    auto cat = std::shared_ptr<QtLinguistCatalog>(new QtLinguistCatalog(std::move(doc)));
    cat->Parse(root);

    return cat;
}


void QtLinguistCatalog::Parse(pugi::xml_node root)
{
    // See https://doc.qt.io/qt-6/linguist-ts-file-format.html for format spec

    m_sourceLanguage = Language::FromLanguageTag(root.attribute("sourcelanguage").value());
    m_language = Language::FromLanguageTag(root.attribute("language").value());

    int id = 0;

    for (auto context : root.children("context"))
    {
        auto name = str::to_wx(context.child("name").text().get());
        ParseSubtree(id, context, name);
    }

    // also handle messages directly under TS (some files have this structure)
    ParseSubtree(id, root, wxEmptyString);
}


void QtLinguistCatalog::ParseSubtree(int& id, pugi::xml_node root, [[maybe_unused]]const wxString& context)
{
    // TODO: "context" in QT Linguist is something like "part of source code", e.g. specific file
    //       or component such as "MainWindow". It doesn't have equivalent in Poedit, so just ignore it
    //       for now, until UI solution for both this and e.g. XLIFF's <file> sections is implemented.

    for (auto message : root.children("message"))
    {
        auto numerus = message.attribute("numerus").value();
        if (strcmp(numerus, "yes") == 0)
            continue;  // FIXME: not implemented yet

        auto type = message.child("translation").attribute("type").value();
        if (strcmp(type, "vanished") == 0 || strcmp(type, "obsolete") == 0)
            continue;  // skip deleted messages

        m_items.push_back(std::make_shared<QtLinguistCatalogItem>(*this, ++id, message));
    }
}


bool QtLinguistCatalog::Save(const wxString& filename, bool /*save_mo*/,
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

    // format_no_empty_element_tags (i.e. <translation></translation> is convention in .ts files
    m_doc.save_file(tempfile.FileName().fn_str(), "\t", format_raw | format_no_empty_element_tags);

    if (!tempfile.Commit())
    {
        wxLogError(_(L"Couldn't save file %s."), filename.c_str());
        return false;
    }

    validation_results = Validate();

    SetFileName(filename);
    return true;
}


std::string QtLinguistCatalog::SaveToBuffer()
{
    std::ostringstream s;
    // format_no_empty_element_tags (i.e. <translation></translation> is convention in .ts files
    m_doc.save(s, "\t", format_raw | format_no_empty_element_tags);
    return s.str();
}


void QtLinguistCatalog::SetLanguage(Language lang)
{
    m_language = lang;
    attribute(GetXMLRoot(), "language") = lang.LanguageTag().c_str();
}
