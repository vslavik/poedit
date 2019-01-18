/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 2018-2019 Vaclav Slavik
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

#include "tmx_io.h"

#include <wx/time.h>
#include <time.h>
#ifdef _WIN32
    #define timegm _mkgmtime
#endif

#include <wx/translation.h>

#include "errors.h"
#include "pugixml.h"
#include "version.h"

using namespace pugi;

namespace
{

std::string extract_date(xml_node node, const std::string& fallback = std::string())
{
    std::string d = node.attribute("changedate").value();
    if (d.empty())
        d = node.attribute("creationdate").value();
    return d.empty() ? fallback : d;
}

const char *extract_lang(xml_node node)
{
    const char *l = node.attribute("xml:lang").as_string(nullptr);
    if (!l)
        l = node.attribute("lang").as_string(nullptr); // TMX 1.1
    return l ? l : "";
}

std::wstring extract_seg(xml_node node)
{
    // Of the markings within <seg>, only <ph> (placeholder) would make sense
    // to extract and substitute with e.g. %s -- but because that's not used
    // in TM search anyway, it's OK to ignore it, at least for now.
    std::string text;
    for (auto child: node.child("seg").children())
    {
        if (child.type() == pugi::node_pcdata)
            text += child.value();
    }
    return pugi::as_wide(text);
}

} // anonymous namespace


void TMX::ImportFromFile(std::istream& file, TranslationMemory& tm)
{
    xml_document doc;
    auto result = doc.load(file);
    if (!result)
        throw std::runtime_error(result.description());

    auto root = doc.child("tmx");
    if (!root)
        throw Exception(_("The TMX file is malformed."));

    std::string defaultSrclang;
    std::string defaultDate;
    auto header = root.child("header");
    if (header)
    {
        defaultSrclang = header.attribute("srclang").value();
        if (defaultSrclang == "*all*")
            defaultSrclang.clear();
        defaultDate = extract_date(header);
    }

    int counter = 0;
    auto body = root.child("body");
    if (!body)
        throw Exception(_("The TMX file is malformed."));

    tm.ImportData([=,&counter,&body](auto& writer)
    {
        for (auto tu: body.children("tu"))
        {
            auto tuDate = extract_date(tu, defaultDate);
            std::string tuSrclang = tu.attribute("srclang").value();
            if (tuSrclang.empty())
                tuSrclang = defaultSrclang;

            std::wstring source;
            for (auto tuv: tu.children("tuv"))
            {
                if (extract_lang(tuv) == tuSrclang)
                {
                    source = extract_seg(tuv);
                    break;
                }
            }
            if (source.empty())
                continue;

            for (auto tuv: tu.children("tuv"))
            {
                auto tuvLang = extract_lang(tuv);
                if (tuvLang == tuSrclang)
                    continue;

                auto srclang = Language::TryParse(tuSrclang);
                auto lang = Language::TryParse(tuvLang);
                if (!srclang.IsValid() || !lang.IsValid())
                    continue;

                auto trans = extract_seg(tuv);
                if (trans.empty())
                    continue;

                time_t creationTime = 0;
                auto tuvDate = extract_date(tu, tuDate);
                if (!tuvDate.empty())
                {
                    struct tm t {};
                    std::istringstream s(tuvDate.c_str());
                    s >> std::get_time(&t, "%Y%m%dT%H%M%SZ"); // YYYYMMDDThhmmssZ
                    if (!s.fail())
                        creationTime = timegm(&t);
                }

                writer.Insert(srclang, lang, source, trans, creationTime);
                counter++;
            }
        }
    });

    if (counter == 0)
        throw Exception(_("No translations were found in the TMX file."));
}




void TMX::ExportToFile(TranslationMemory& tm, std::ostream& file)
{
    class Exporter : public TranslationMemory::IOInterface
    {
    public:
        Exporter()
        {
            auto root = m_doc.append_child("tmx");
            root.append_attribute("version") = "1.4";
            auto header = root.append_child("header");
            header.append_attribute("creationtool") = "Poedit";
            header.append_attribute("creationtoolversion") = POEDIT_VERSION;
            header.append_attribute("datatype") = "PlainText";
            header.append_attribute("segtype") = "sentence";
            header.append_attribute("adminlang") = "en";
            header.append_attribute("srclang") = "en"; // reasonable default for gettext
            header.append_attribute("o-tmf") = "PoeditTM";
            m_body = root.append_child("body");
        }

        void Insert(const Language& srclang,
                    const Language& lang,
                    const std::wstring& source,
                    const std::wstring& trans,
                    time_t creationTime) override
        {
            auto tu = m_body.append_child("tu");
            auto srctag = srclang.LanguageTag();
            if (srctag != "en")
                tu.append_attribute("srclang") = srctag.c_str();

            if (creationTime > 0)
            {
                struct tm t;
                wxGmtime_r(&creationTime, &t);
                std::ostringstream s;
                s << std::put_time(&t, "%Y%m%dT%H%M%SZ"); // YYYYMMDDThhmmssZ
                tu.append_attribute("creationdate") = s.str().c_str();
            }

            {
                auto tuv = tu.append_child("tuv");
                tuv.append_attribute("xml:lang") = srctag.c_str();
                tuv.append_child("seg").text() = pugi::as_utf8(source).c_str();
            }
            {
                auto tuv = tu.append_child("tuv");
                tuv.append_attribute("xml:lang") = lang.LanguageTag().c_str();
                tuv.append_child("seg").text() = pugi::as_utf8(trans).c_str();
            }
        }

        void Save(std::ostream& f)
        {
            m_doc.save(f);
        }

    private:
        xml_document m_doc;
        xml_node m_body;
    };

    Exporter e;
    tm.ExportData(e);
    e.Save(file);
}
