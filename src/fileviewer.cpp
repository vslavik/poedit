/*
 *  This file is part of Poedit (http://www.poedit.net)
 *
 *  Copyright (C) 1999-2005 Vaclav Slavik
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

#include <wx/wxprec.h>

#include "fileviewer.h"

#include <wx/filename.h>
#include <wx/log.h>
#include <wx/panel.h>
#include <wx/stattext.h>
#include <wx/choice.h>
#include <wx/config.h>
#include <wx/sizer.h>
#include <wx/settings.h>
#include <wx/listctrl.h>
#include <wx/fontenum.h>
#include <wx/stc/stc.h>

FileViewer::FileViewer(wxWindow *parent,
                       const wxString& basePath,
                       const wxArrayString& references,
                       size_t startAt)
        : wxFrame(parent, -1, _("Source file")),
          m_references(references)
{
    m_basePath = basePath;

    wxPanel *panel = new wxPanel(this, -1);
    wxSizer *sizer = new wxBoxSizer(wxVERTICAL);
    panel->SetSizer(sizer);

    wxSizer *barsizer = new wxBoxSizer(wxHORIZONTAL);
    sizer->Add(barsizer, wxSizerFlags().Expand().Border());

    barsizer->Add(new wxStaticText(panel, wxID_ANY,
                                   _("Source file occurrence:")),
                  wxSizerFlags().Center().Border(wxRIGHT));

    wxChoice *choice = new wxChoice(panel, wxID_ANY);
    barsizer->Add(choice, wxSizerFlags(1).Center());

    for (size_t i = 0; i < references.Count(); i++)
        choice->Append(references[i]);
    choice->SetSelection(startAt);

    m_text = new wxStyledTextCtrl(panel, wxID_ANY,
                                  wxDefaultPosition, wxDefaultSize,
                                  wxBORDER_THEME);
    SetupTextCtrl();
    sizer->Add(m_text, 1, wxEXPAND);

    wxConfigBase *cfg = wxConfig::Get();
    int width = cfg->Read(_T("fileviewer/frame_w"), 600);
    int height = cfg->Read(_T("fileviewer/frame_h"), 400);
    SetClientSize(width, height);

#ifndef __WXGTK__
    int posx = cfg->Read(_T("fileviewer/frame_x"), -1);
    int posy = cfg->Read(_T("fileviewer/frame_y"), -1);
    Move(posx, posy);
#endif

    wxSizer *topsizer = new wxBoxSizer(wxVERTICAL);
    topsizer->Add(panel, wxSizerFlags(1).Expand());
    SetSizer(topsizer);
    Layout();

    ShowReference(m_references[startAt]);
}


FileViewer::~FileViewer()
{
    wxSize sz = GetClientSize();
    wxPoint pos = GetPosition();
    wxConfigBase *cfg = wxConfig::Get();
    cfg->Write(_T("fileviewer/frame_w"), (long)sz.x);
    cfg->Write(_T("fileviewer/frame_h"), (long)sz.y);
    cfg->Write(_T("fileviewer/frame_x"), (long)pos.x);
    cfg->Write(_T("fileviewer/frame_y"), (long)pos.y);
}


void FileViewer::SetupTextCtrl()
{
    wxStyledTextCtrl& t = *m_text;

    wxFont font = wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT);

#ifdef __WXGTK__
    font.SetFaceName(_T("monospace"));
#else
    static const wxChar *s_monospaced_fonts[] = {
#ifdef __WXMSW__
        _T("Consolas"),
        _T("Lucida Console"),
#endif
#ifdef __WXOSX__
        _T("Menlo"),
        _T("Monaco"),
#endif
        NULL
    };

    for ( const wxChar **f = s_monospaced_fonts; *f; ++f )
    {
        if ( wxFontEnumerator::IsValidFacename(*f) )
        {
            font.SetFaceName(*f);
            break;
        }
    }
#endif

    // style used:
    wxString fontspec = wxString::Format(_T("face:%s,size:%d"),
                                         font.GetFaceName().c_str(),
                                         font.GetPointSize());
    const wxString STRING      = fontspec + _T(",bold,fore:#882d21");
    const wxString COMMENT     = fontspec + _T(",fore:#487e18");
    const wxString KEYWORD     = fontspec + _T(",fore:#2f00f9");
    const wxString LINENUMBERS = fontspec + _T(",fore:#5d8bab");


    // current line marker
    t.MarkerDefine(1, wxSTC_MARK_BACKGROUND, wxNullColour, wxColour(255,255,0));

    // set fonts
    t.StyleSetSpec(wxSTC_STYLE_DEFAULT, fontspec);
    t.StyleSetSpec(wxSTC_STYLE_LINENUMBER, LINENUMBERS);

    // line numbers margin size:
    t.SetMarginType(0, wxSTC_MARGIN_NUMBER);
    t.SetMarginWidth(0,
                     t.TextWidth(wxSTC_STYLE_LINENUMBER, _T("9999 ")));
    t.SetMarginWidth(1, 0);
    t.SetMarginWidth(2, 3);

    // set syntax highlighting styling
    t.StyleSetSpec(wxSTC_C_STRING, STRING);
    t.StyleSetSpec(wxSTC_C_COMMENT, COMMENT);
    t.StyleSetSpec(wxSTC_C_COMMENTLINE, COMMENT);

    t.StyleSetSpec(wxSTC_P_STRING, STRING);
    t.StyleSetSpec(wxSTC_P_COMMENTLINE, COMMENT);
    t.StyleSetSpec(wxSTC_P_COMMENTBLOCK, COMMENT);

    t.StyleSetSpec(wxSTC_LUA_STRING, STRING);
    t.StyleSetSpec(wxSTC_LUA_LITERALSTRING, STRING);
    t.StyleSetSpec(wxSTC_LUA_COMMENT, COMMENT);
    t.StyleSetSpec(wxSTC_LUA_COMMENTLINE, COMMENT);

    t.StyleSetSpec(wxSTC_HPHP_HSTRING, STRING);
    t.StyleSetSpec(wxSTC_HPHP_SIMPLESTRING, STRING);
    t.StyleSetSpec(wxSTC_HPHP_COMMENT, COMMENT);
    t.StyleSetSpec(wxSTC_HPHP_COMMENTLINE, COMMENT);

    t.StyleSetSpec(wxSTC_TCL_COMMENT, COMMENT);
    t.StyleSetSpec(wxSTC_TCL_COMMENTLINE, COMMENT);
    t.StyleSetSpec(wxSTC_TCL_BLOCK_COMMENT, COMMENT);

#if wxCHECK_VERSION(2,9,0)
    t.StyleSetSpec(wxSTC_PAS_STRING, STRING);
    t.StyleSetSpec(wxSTC_PAS_COMMENT, COMMENT);
    t.StyleSetSpec(wxSTC_PAS_COMMENT2, COMMENT);
    t.StyleSetSpec(wxSTC_PAS_COMMENTLINE, COMMENT);
#endif
}


int FileViewer::GetLexer(const wxString& ext)
{
    struct LexerInfo
    {
        const char *ext;
        int lexer;
    };

    static const LexerInfo s_lexer[] = {
        { "c", wxSTC_LEX_CPP },
        { "cpp", wxSTC_LEX_CPP },
        { "cc", wxSTC_LEX_CPP },
        { "cxx", wxSTC_LEX_CPP },
        { "h", wxSTC_LEX_CPP },
        { "hxx", wxSTC_LEX_CPP },
        { "hpp", wxSTC_LEX_CPP },
        { "py", wxSTC_LEX_PYTHON },
        { "htm", wxSTC_LEX_HTML },
        { "html", wxSTC_LEX_HTML },
        { "php", wxSTC_LEX_PHPSCRIPT },
        { "xml", wxSTC_LEX_XML },
        { "pas", wxSTC_LEX_PASCAL },
        { NULL, -1 }
    };

    wxString e = ext.Lower();

    for ( const LexerInfo *i = s_lexer; i->ext; ++i )
    {
        if ( e == wxString::FromAscii(i->ext) )
            return i->lexer;
    }

    return wxSTC_LEX_NULL;
}


void FileViewer::ShowReference(const wxString& ref)
{
    wxPathFormat pathfmt = ref.Contains(_T('\\')) ? wxPATH_WIN : wxPATH_UNIX;
    wxFileName filename(ref.BeforeLast(_T(':')), pathfmt);
    filename.MakeAbsolute(m_basePath);

    if ( !filename.IsFileReadable() )
    {
        wxLogError(_("Error opening file %s!"), filename.GetFullPath().c_str());
        return;
    }

    m_current = ref;

    // support GNOME's xml2po's extension to references in the form of
    // filename:line(xml_node):
    wxString linenumStr = ref.AfterLast(_T(':')).BeforeFirst(_T('('));

    long linenum;
    if (!linenumStr.ToLong(&linenum))
        linenum = 0;

    m_text->SetReadOnly(false);
    m_text->LoadFile(filename.GetFullPath());
    m_text->SetLexer(GetLexer(filename.GetExt()));
    m_text->SetReadOnly(true);

    m_text->MarkerDeleteAll(1);
    m_text->MarkerAdd(linenum, 1);

    // Center the highlighted line:
    int lineHeight = m_text->TextHeight(linenum);
    int linesInWnd = m_text->GetSize().y / lineHeight;
    m_text->ScrollToLine(wxMax(0, linenum - linesInWnd/2));

}



BEGIN_EVENT_TABLE(FileViewer, wxFrame)
    EVT_CHOICE(wxID_ANY, FileViewer::OnChoice)
END_EVENT_TABLE()

void FileViewer::OnChoice(wxCommandEvent &event)
{
    ShowReference(m_references[event.GetSelection()]);
}
