/*

    poedit, a wxWindows i18n catalogs editor

    ---------------
      export_html.cpp
    
      Catalog export into HTML file
    
      (c) Christophe Hermier, 2003

      $Id$

*/


#ifdef __GNUG__
#pragma implementation
#endif

#include <wx/intl.h>
#include <wx/colour.h>
#include <wx/utils.h>
#include <wx/textfile.h>

#include "catalog.h"

// colours used in the list:
// (FIXME: this is duplicated with code in edframe.cpp, get rid of this
// duplication, preferably by making the colours customizable and stored
// in wxConfig)

#define g_darkColourFactor 0.95
#define DARKEN_COLOUR(r,g,b) (wxColour(int((r)*g_darkColourFactor),\
                                       int((g)*g_darkColourFactor),\
                                       int((b)*g_darkColourFactor)))
#define LIST_COLOURS(r,g,b) { wxColour(r,g,b), DARKEN_COLOUR(r,g,b) }
static wxColour 
    g_ItemColourNormal[2] =       LIST_COLOURS(0xFF,0xFF,0xFF), // white
    g_ItemColourUntranslated[2] = LIST_COLOURS(0xA5,0xEA,0xEF), // blue
    g_ItemColourFuzzy[2] =        LIST_COLOURS(0xF4,0xF1,0xC1); // yellow


static wxString trans2utf(const wxString& s)
{
#if wxUSE_UNICODE
    return s;
#else
    return wxString(s.wc_str(wxConvLocal), wxConvUTF8);
#endif
}


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
	line.Printf(_T("<title> %s - %s / %s - poEdit Export </title>"),
                m_header.Project.c_str(),
                m_header.Language.c_str(),
                m_header.Country.c_str());
    f.AddLine(line);
    f.AddLine(_T("<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\">" ) );
    f.AddLine(_T("</head>"));
    f.AddLine(_T("<body bgcolor='#FFFFFF'>"));

	line.Printf(_T("<h1> %s : %s / %s</h1>"),
               m_header.Project.c_str(),
               m_header.Language.c_str(),
               m_header.Country.c_str());
    f.AddLine(line);


	// po file header information :

	// String here are duplicates from the ones in setting.xrc
	// TODO find a way if possible to synchronize them

	f.AddLine(_T("<table align=center border=1 cellspacing=2 cellpadding=4>"));

	line.Printf(_T("<tr><th colspan=2>%s</th></tr>"),
                trans2utf(_("Project info")).c_str());
	f.AddLine(line);
	wxString line_format = _T("<tr><td>%s</td><td>%s</td></tr>");
	line.Printf(line_format,
                trans2utf(_("Project name and version:")).c_str(),
                m_header.Project.c_str());
	f.AddLine(line);
	line.Printf(line_format, trans2utf(_("Language:")).c_str(),
                m_header.Language.c_str());
	f.AddLine(line);
	line.Printf(line_format, trans2utf(_("Country:")).c_str(),
                m_header.Country.c_str());
	f.AddLine(line);
    line.Printf(line_format, trans2utf(_("Team:")).c_str(),
                m_header.Team.c_str());
	f.AddLine(line);
	line.Printf(_T("<tr><td>%s</td><td><a href=\"mailto:%s\">%s</a></td></tr>"),
                trans2utf(_("Team's email address:")).c_str(),
                m_header.TeamEmail.c_str(), m_header.TeamEmail.c_str());
	f.AddLine(line);
	line.Printf(line_format, trans2utf(_("Charset:")).c_str(),
                m_header.Charset.c_str());
	f.AddLine(line);

	f.AddLine( _T("</table>") );
	// statistics

    int all = 0;
	int fuzzy = 0;
	int untranslated = 0;
    GetStatistics(&all, &fuzzy, &untranslated);
	line.Printf(
            trans2utf(_("%i strings (%i fuzzy, %i not translated)")).c_str(),
            all, fuzzy, untranslated);
	f.AddLine(line);


	// data printed in a table :
	f.AddLine(_T("<table border=1 cellspacing=2 cellpadding=4>"));

	f.AddLine(_T("<tr>"));
	f.AddLine(_T("<th>"));
	f.AddLine(trans2utf(_("Original string")).c_str());
	f.AddLine(_T("</th>"));        
	f.AddLine(_T("<th>"));		
	f.AddLine(trans2utf(_("Translation")).c_str());
	f.AddLine(_T("</th>"));
	f.AddLine(_T("</th>"));        
	f.AddLine(_T("<th>"));
	f.AddLine(trans2utf(_("Notes")).c_str());
	f.AddLine(_T("</th>"));
	f.AddLine(_T("</tr>"));
	
	for (i = 0; i < m_count; i++)
    {
        const CatalogData& data = m_dataArray[i];

		wxColour bgcolor = g_ItemColourNormal[i % 2];
		wxString original_string = data.GetString();

		wxString translation = data.GetTranslation();
		if (translation.IsEmpty())
		{
			translation = _T("&nbsp;");
			bgcolor = g_ItemColourUntranslated[i % 2];
		}

		wxString flags;

		if (data.IsAutomatic())
		{
			flags += trans2utf(_("Automatic translation"));
			flags += _T("<BR>");
		}
		if (data.IsFuzzy())
		{
			bgcolor = g_ItemColourFuzzy[i % 2];
			flags += trans2utf(_("Fuzzy translation"));
			flags += _T("<BR>");
		}
		if (flags.IsEmpty())
		{
			flags = _T("&nbsp;");
		}
        
		wxString tr;
		tr.Printf(_T("<tr bgcolor='#%0X%0X%0X'>"),
                  bgcolor.Red(), bgcolor.Green(), bgcolor.Blue());
        f.AddLine(tr);

        f.AddLine(_T("<td>"));
		f.AddLine(original_string);
        f.AddLine(_T("</td>"));
        f.AddLine(_T("<td>"));
		f.AddLine(translation);
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
