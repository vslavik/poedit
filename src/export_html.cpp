/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 2003 Christophe Hermier
 *  Copyright (C) 2013-2023 Vaclav Slavik
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

#include "catalog.h"
#include "utility.h"
#include "str_helpers.h"

#include <wx/intl.h>

namespace
{

inline std::string fmt_trans(const wxString& s)
{
    wxString out = EscapeMarkup(s);
    out.Replace("\n", "\n<br>");
    return str::to_utf8(out);
}

template<typename T1, typename T2>
inline void TableRow(std::ostream& f, const T1& col1, const T2& col2)
{
    f << "<tr>"
      << "<td>" << str::to_utf8(col1) << "</td>"
      << "<td>" << str::to_utf8(col2) << "</td>"
      << "</tr>\n";
}

extern const char *CSS_STYLE;

} // anonymous namespace

void Catalog::ExportToHTML(std::ostream& f)
{
    const bool translated = HasCapability(Catalog::Cap::Translations);
    const auto lang = translated ? GetLanguage() : Language();
    const auto srclang = GetSourceLanguage();

    f << "<!DOCTYPE html>\n"
         "<html>\n"
         "<head>\n"
         "  <title>" << str::to_utf8(m_header.Project) << "</title>\n"
         "  <meta http-equiv='Content-Type' content='text/html; charset=utf-8'>\n"
         "  <style>\n" << CSS_STYLE << "\n"
         "  </style>\n"
         "</head>\n"
         "<body>\n"
         "<div class='container'>\n";

    // Metadata section:

    f << "<table class='metadata'>\n";
    if (!m_header.Project.empty())
        TableRow(f, _("Project:"), m_header.Project);
    if (translated && lang.IsValid())
        TableRow(f, _("Language:"), lang.DisplayName());
    f << "</table>\n";


    // Statistics:
    if (translated)
    {
        int all = 0;
        int fuzzy = 0;
        int untranslated = 0;
        int unfinished = 0;
        GetStatistics(&all, &fuzzy, nullptr, &untranslated, &unfinished);
        int percent = (all == 0 ) ? 0 : (100 * (all - unfinished) / all);

        f << "<div class='stats'>\n"
          << "  <div class='graph'>\n";
        if (all > unfinished)
          f << "    <div class='percent-done' style='width: " << 100.0 * (all - unfinished) / all << "%'>&nbsp;</div>\n";
        if (fuzzy > 0)
          f << "    <div class='percent-fuzzy' style='width: " << 100.0 * fuzzy / all << "%'>&nbsp;</div>\n";
        if (untranslated > 0)
          f << "    <div class='percent-untrans' style='width: " << 100.0 * untranslated / all << "%'>&nbsp;</div>\n";
        f << "  </div>\n"
          << "  <div class='legend'>";
        f << str::to_utf8(wxString::Format(_("Translated: %d of %d (%d %%)"), all - unfinished, all, percent));
        if (unfinished > 0)
            f << str::to_utf8(L"  •  ") << str::to_utf8(wxString::Format(_("Remaining: %d"), unfinished));
        f << "  </div>\n"
          << "</div>\n";
    }
    else
    {
        int all = (int)items().size();
        f << "<div class='stats'>\n"
          << "  <div class='graph'>\n"
          << "    <div class='percent-untrans' style='width: 100%'>&nbsp;</div>\n"
          << "  </div>\n"
          << "  <div class='legend'>"
          << str::to_utf8(wxString::Format(wxPLURAL("%d entry", "%d entries", all), all))
          << "  </div>\n"
          << "</div>\n";
    }

    // Translations:

    std::string lang_src, lang_tra;
    if (srclang.IsValid())
        lang_src = " lang='" + srclang.LanguageTag() + "'";
    if (lang.IsValid())
    {
        lang_tra = " lang='" + lang.LanguageTag() + "'";
        if (lang.IsRTL())
            lang_tra += " dir='rtl'";
    }

    wxString thead_src;
    if (UsesSymbolicIDsForSource())
    {
        thead_src = _("Source text ID");
    }
    else
    {
        thead_src = srclang.IsValid()
                    ? (wxString::Format(_(L"Source text — %s"), srclang.DisplayName()))
                    : _("Source text");
    }
    auto thead_tra = wxString::Format(_(L"Translation — %s"),
                                      lang.IsValid() ? lang.DisplayName() : _("unknown language"));

    f << "<table class='translations'>\n"
         "  <thead>\n"
         "    <tr>\n"
         "      <th>" << str::to_utf8(thead_src) << "</th>\n";
    if (translated)
    {
        f << "      <th>" << str::to_utf8(thead_tra) << "</th>\n";
    }
    f << "    </tr>\n"
         "  </thead>\n"
         "  <tbody>\n";

    for (auto& item: items())
    {
        bool hasComments = item->HasComment() || item->HasExtractedComments();

        std::string klass("i");
        if (!item->IsTranslated())
            klass += " untrans";
        if (item->IsFuzzy())
            klass += " fuzzy";
        if (hasComments)
            klass += " with-comments";
        f << "<tr class='" << klass << "'>\n";

        // Source string:
        f << "<td class='src' " << lang_src << ">\n";
        if (item->HasContext())
            f << " <span class='msgctxt'>" << fmt_trans(item->GetContext()) << "</span>";
        if (item->HasPlural())
        {
            f << "<ol class='plurals'>\n"
              << "  <li>" << fmt_trans(item->GetString()) << "</li>\n"
              << "  <li>" << fmt_trans(item->GetPluralString()) << "</li>\n"
              << "</ol>\n";
        }
        else
        {
            f << fmt_trans(item->GetString());
        }
        f << "</td>\n";

        // Translation:
        if (translated)
        {
            f << "<td class='tra' " << lang_tra << ">\n";
            if (item->HasPlural())
            {
                if (item->IsTranslated())
                {
                    f << "<ol class='plurals'>\n";
                    for (auto t: item->GetTranslations())
                        f << "  <li>" << fmt_trans(t) << "</li>\n";
                    f << "</ol>\n";
                }
            }
            else
            {
                f << fmt_trans(item->GetTranslation());
            }
            f << "</td>\n";
        }

        // Notes, if present:
        if (hasComments)
        {
            f << "</tr>\n"
              << "<tr class='comments'>\n"
              << "  <td colspan='" << (translated ? 2 : 1) << "'><div>";
            if (item->HasExtractedComments())
            {
                f << "<p>\n";
                for (auto& n: item->GetExtractedComments())
                    f << fmt_trans(n) << "<br>\n";
                f << "</p>\n";
            }
            if (item->HasComment())
            {
                f << "<p>\n"
                  << fmt_trans(item->GetComment())
                  << "</p>\n";
            }
            f << "</div></td>\n";
        }

        f << "</tr>\n";
    }

    f << "</tbody>\n"
         "</table>\n"
         "</div>\n"
         "</body>\n"
         "</html>\n";
}


namespace
{

const char *CSS_STYLE = R"(

/*
Based on Minimal CSS (minimalcss.com) under the MIT license.
*/

/* Reset */
* { margin: 0; padding: 0; -webkit-box-sizing: border-box; -moz-box-sizing: border-box; box-sizing: border-box; }

/* Layout */
body { font: 14px/20px -apple-system, system-ui, BlinkMacSystemFont, "Segoe UI", Roboto, "Helvetica Neue", Arial, sans-serif; }

.container { position: relative; max-width: 90%; margin: 0 auto; }

/* Typography */
a { color: #105CB6; text-decoration: none; }
a:hover, a:focus { color: #105CB6; text-decoration: underline; }
a:active { color: #105CB6; }

h1 { font-size: 24px; line-height: 20px; margin: 10px 0; }
h2 { font-size: 20px; line-height: 20px; margin: 10px 0; }
h3 { font-size: 16px; line-height: 20px; margin: 10px 0; }
h4 { font-size: 14px; line-height: 20px; margin: 10px 0; }
h5 { font-size: 12px; line-height: 20px; margin: 10px 0; }
h5 { font-size: 10px; line-height: 20px; margin: 10px 0; }

p { margin-bottom: 10px; }

.float-left     { float: left; }
.float-right    { float: right; }
img.float-left  { float: left; margin: 0 20px 20px 0; }
img.float-right { float: right; margin: 0 0 20px 20px; }
img.center      { margin: 0 auto; display: block; }

.text-left    { text-align: left; }
.text-center  { text-align: center; }
.text-right   { text-align: right; }
.text-justify { text-align: justify; }

/* List */
ul { list-style-position:inside; }
ol { list-style-position:inside; }

/* Table */
table {
  border-collapse: collapse;
  border-spacing: 0;
}
th { font-weight: bold; }
tfoot { font-style: italic; }

/* Metadata part */

.metadata {
  margin-top: 15px;
  margin-bottom: 10px;
  font-size: 90%;
}
table.metadata td {
  padding-right: 20px;
}

.stats {
  padding-top: 5px;
  padding-bottom: 20px;
}
.graph {
  width: 100%;
}
.graph div { float: left; }
.graph div:first-child { border-top-left-radius: 3px; border-bottom-left-radius: 3px; }
.graph div:last-child { border-top-right-radius: 3px; border-bottom-right-radius: 3px; }
.legend {
  font-size: smaller;
  padding-top: 12px;
  text-align: center;
}

/* Translations */
table.translations {
  width: 100%;
  table-layout: fixed;
}
table.translations th, table.translations td {
  padding: 5px 10px;
  vertical-align: top;
  border-bottom: 1px solid #F1F1F1;
}
table.translations th {
  text-align: left;
}
table.translations th:first-child, table.translations td:first-child { padding-left: 0; }
table.translations th:last-child, table.translations td:last-child { padding-right: 0; }

.with-comments td {
  border-bottom: none !important;
}
tr.comments div {
  float: left;
  max-width: 90%;
  font-size: smaller;
}
tr.comments div p:last-child { margin-bottom: 0; }
tr.comments td { padding-top: 0; }

.msgctxt {
  font-size: smaller;
  border-radius: 2px;
  padding: 2px 4px;
  margin-right: 4px;
}


/* Colors */

body { background-color: #fff; color: #333; }

.percent-done    { background-color: rgb(146, 236, 106); height: 10px; }
.percent-fuzzy   { background-color: rgb(255, 149, 0); height: 10px; }
.percent-untrans { background-color: #F1F1F1; height: 10px; }
.legend          { color: #aaa; }
tr.comments div  { color: #aaa; }
.fuzzy .tra      { color: rgb(230, 134, 0); }

.msgctxt         { color: rgb(70, 109, 137); background-color: rgb(217, 232, 242); }


@media (prefers-color-scheme: dark) {
    body             { background-color: rgb(45, 42, 41); color: #eee; }
    .percent-untrans { background-color: rgba(255, 255, 255, 0.3); }
    .legend          { color: rgba(255, 255, 255, 0.6); }
    tr.comments div  { color: rgba(255, 255, 255, 0.6); }
    .fuzzy .tra      { color: rgb(253, 178, 72); }
    .msgctxt         { color: rgb(180, 222, 254); background-color: rgba(67, 94, 147, 0.6); }
    table.translations th, table.translations td { border-bottom: 1px solid #333; }
}

)";

} // anonymous namespace
