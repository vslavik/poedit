/*
 *  This file is part of Poedit (http://www.poedit.net)
 *
 *  Copyright (C) 2003 Christophe Hermier
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

#include <wx/intl.h>
#include <wx/colour.h>
#include <wx/utils.h>
#include <wx/textfile.h>

#include "catalog.h"
#include "utility.h"

namespace
{

// colours used in the list:
// (FIXME: this is duplicated with code in edframe.cpp, get rid of this
// duplication, preferably by making the colours customizable and stored
// in wxConfig)

#define g_darkColourFactor 0.95
#define DARKEN_COLOUR(r,g,b) (wxColour(int((r)*g_darkColourFactor),\
                                       int((g)*g_darkColourFactor),\
                                       int((b)*g_darkColourFactor)))
#define LIST_COLOURS(r,g,b) { wxColour(r,g,b), DARKEN_COLOUR(r,g,b) }
wxColour
    g_ItemColourNormal[2] =       LIST_COLOURS(0xFF,0xFF,0xFF), // white
    g_ItemColourUntranslated[2] = LIST_COLOURS(0xA5,0xEA,0xEF), // blue
    g_ItemColourFuzzy[2] =        LIST_COLOURS(0xF4,0xF1,0xC1); // yellow


} // anonymous namespace

bool Catalog::ExportToHTML(const wxString& filename)
{
    size_t i;
    wxTextFile f;

    if ( wxFileExists(filename) )
    {
        wxRemoveFile ( filename);
    }

    if (!f.Create(filename))
    {
        return false;
    }

    // TODO use some kind of HTML template system to allow different styles

    wxString line;

    // HTML HEADER
    f.AddLine(_T("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\" \"http://www.w3.org/TR/html4/loose.dtd\">"));
    f.AddLine(_T("<html>"));

    f.AddLine(_T("<head>"));
    line.Printf(_T("<title> %s - %s / %s - Poedit Export </title>"),
                EscapeMarkup(m_header.Project).c_str(),
                EscapeMarkup(m_header.Language).c_str(),
                EscapeMarkup(m_header.Country).c_str());
    f.AddLine(line);
    f.AddLine(_T("<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\">" ) );
    f.AddLine(_T("</head>"));
    f.AddLine(_T("<body bgcolor='#FFFFFF'>"));

    line.Printf(_T("<h1> %s : %s / %s</h1>"),
                EscapeMarkup(m_header.Project).c_str(),
                EscapeMarkup(m_header.Language).c_str(),
                EscapeMarkup(m_header.Country).c_str());
    f.AddLine(line);


    // po file header information :

    // String here are duplicates from the ones in setting.xrc
    // TODO find a way if possible to synchronize them

    f.AddLine(_T("<table align=center border=1 cellspacing=2 cellpadding=4>"));

    line.Printf(_T("<tr><th colspan=2>%s</th></tr>"),
                _("Project info"));
    f.AddLine(line);
    wxString line_format = _T("<tr><td>%s</td><td>%s</td></tr>");
    line.Printf(line_format,
                _("Project name and version:"),
                EscapeMarkup(m_header.Project).c_str());
    f.AddLine(line);
    line.Printf(line_format, _("Language:"),
                EscapeMarkup(m_header.Language).c_str());
    f.AddLine(line);
    line.Printf(line_format, _("Country:"),
                EscapeMarkup(m_header.Country).c_str());
    f.AddLine(line);
    line.Printf(line_format, _("Team:"),
                EscapeMarkup(m_header.Team).c_str());
    f.AddLine(line);
    line.Printf(_T("<tr><td>%s</td><td><a href=\"mailto:%s\">%s</a></td></tr>"),
                _("Team's email address:"),
                EscapeMarkup(m_header.TeamEmail).c_str(),
                EscapeMarkup(m_header.TeamEmail).c_str());
    f.AddLine(line);
    line.Printf(line_format, _("Charset:"),
                EscapeMarkup(m_header.Charset).c_str());
    f.AddLine(line);

    f.AddLine( _T("</table>") );
    // statistics

    int all = 0;
    int fuzzy = 0;
    int untranslated = 0;
    int badtokens = 0;
    int unfinished = 0;
    GetStatistics(&all, &fuzzy, &badtokens, &untranslated, &unfinished);

    int percent = (all == 0 ) ? 0 : (100 * (all - unfinished) / all);
    line.Printf(_("%i %% translated, %i strings (%i fuzzy, %i bad tokens, %i not translated)"),
               percent, all, fuzzy, badtokens, untranslated);

    f.AddLine(line);


    // data printed in a table :
    f.AddLine(_T("<table border=1 cellspacing=2 cellpadding=4>"));

    f.AddLine(_T("<tr>"));
    f.AddLine(_T("<th>"));
    f.AddLine(_("Source"));
    f.AddLine(_T("</th>"));
    f.AddLine(_T("<th>"));
    f.AddLine(_("Translation"));
    f.AddLine(_T("</th>"));
    f.AddLine(_T("<th>"));
    f.AddLine(_("Notes"));
    f.AddLine(_T("</th>"));
    f.AddLine(_T("</tr>"));

    for (i = 0; i < GetCount(); i++)
    {
        const CatalogItem& data = m_items[i];

        wxColour bgcolor = g_ItemColourNormal[i % 2];
        wxString source_string = data.GetString();

        wxString translation = data.GetTranslation();
        if (translation.empty())
        {
            translation = _T(" ");
            bgcolor = g_ItemColourUntranslated[i % 2];
        }

        wxString flags;

        if (data.IsAutomatic())
        {
            flags += EscapeMarkup(_("Automatic translation"));
            flags += _T("<BR>");
        }
        if (data.IsFuzzy())
        {
            bgcolor = g_ItemColourFuzzy[i % 2];
            flags += EscapeMarkup(_("Fuzzy translation"));
            flags += _T("<BR>");
        }
        if (flags.empty())
        {
            flags = _T(" ");
        }

        wxString tr;
        tr.Printf(_T("<tr bgcolor='#%0X%0X%0X'>"),
                  bgcolor.Red(), bgcolor.Green(), bgcolor.Blue());
        f.AddLine(tr);

        f.AddLine(_T("<td>"));
        f.AddLine(EscapeMarkup(source_string));
        f.AddLine(_T("</td>"));
        f.AddLine(_T("<td>"));
        f.AddLine(EscapeMarkup(translation));
        f.AddLine(_T("</td>"));
        f.AddLine(_T("<td>"));
        f.AddLine(_T("<font size=\"-1\">"));
        f.AddLine(flags);
        f.AddLine(_T("</font>"));
        f.AddLine(_T("</td>"));
        f.AddLine(_T("</tr>"));
    }

    f.AddLine(_T("</table>"));
    f.AddLine(_T("</body>"));
    f.AddLine(_T("</html>"));

    bool written = f.Write(wxTextFileType_None, wxConvUTF8);

    f.Close();

    return written;
}
