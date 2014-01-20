/*
 *  This file is part of Poedit (http://www.poedit.net)
 *
 *  Copyright (C) 1999-2014 Vaclav Slavik
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

#include <wx/filename.h>
#include <wx/log.h>
#include <wx/button.h>
#include <wx/panel.h>
#include <wx/stattext.h>
#include <wx/choice.h>
#include <wx/config.h>
#include <wx/sizer.h>
#include <wx/settings.h>
#include <wx/listctrl.h>
#include <wx/fontenum.h>
#include <wx/ffile.h>
#include <wx/utils.h>
#include <wx/stc/stc.h>

#include "fileviewer.h"
#include "utility.h"

FileViewer::FileViewer(wxWindow *parent,
                       const wxString& basePath,
                       const wxArrayString& references,
                       int startAt)
        : wxFrame(parent, -1, _("Source file")),
          m_references(references)
{
    m_basePath = basePath;

    SetName("fileviewer");

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

    wxButton *edit = new wxButton(panel, wxID_ANY, _("Open In Editor"));
    barsizer->Add(edit, wxSizerFlags().Center().Border(wxLEFT, 10));

    m_text = new wxStyledTextCtrl(panel, wxID_ANY,
                                  wxDefaultPosition, wxDefaultSize,
                                  wxBORDER_THEME);
    SetupTextCtrl();
    sizer->Add(m_text, 1, wxEXPAND);

    RestoreWindowState(this, wxSize(600, 400));

    wxSizer *topsizer = new wxBoxSizer(wxVERTICAL);
    topsizer->Add(panel, wxSizerFlags(1).Expand());
    SetSizer(topsizer);
    Layout();

    choice->Bind(wxEVT_CHOICE, &FileViewer::OnChoice, this);
    edit->Bind(wxEVT_BUTTON, &FileViewer::OnEditFile, this);

    ShowReference(m_references[startAt]);

#ifdef __WXOSX__
    wxAcceleratorEntry entries[] = {
        { wxACCEL_CMD,  'W', wxID_CLOSE }
    };
    wxAcceleratorTable accel(WXSIZEOF(entries), entries);
    SetAcceleratorTable(accel);

    Bind(wxEVT_MENU, [=](wxCommandEvent&){ Destroy(); }, wxID_CLOSE);
#endif
}


FileViewer::~FileViewer()
{
    SaveWindowState(this);
}


void FileViewer::SetupTextCtrl()
{
    wxStyledTextCtrl& t = *m_text;

    wxFont font = wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT);

#ifdef __WXGTK__
    font.SetFaceName("monospace");
#else
    static const char *s_monospaced_fonts[] = {
#ifdef __WXMSW__
        "Consolas",
        "Lucida Console",
#endif
#ifdef __WXOSX__
        "Menlo",
        "Monaco",
#endif
        NULL
    };

    for ( const char **f = s_monospaced_fonts; *f; ++f )
    {
        if ( wxFontEnumerator::IsValidFacename(*f) )
        {
            font.SetFaceName(*f);
            break;
        }
    }
#endif

    // style used:
    wxString fontspec = wxString::Format("face:%s,size:%d",
                                         font.GetFaceName().c_str(),
                                         font.GetPointSize());
    const wxString DEFAULT     = fontspec + ",fore:black,back:white";
    const wxString STRING      = fontspec + ",bold,fore:#882d21";
    const wxString COMMENT     = fontspec + ",fore:#487e18";
    const wxString KEYWORD     = fontspec + ",fore:#2f00f9";
    const wxString LINENUMBERS = fontspec + ",fore:#5d8bab";


    // current line marker
    t.MarkerDefine(1, wxSTC_MARK_BACKGROUND, wxNullColour, wxColour(255,255,0));

    // set fonts
    t.StyleSetSpec(wxSTC_STYLE_DEFAULT, DEFAULT);
    t.StyleSetSpec(wxSTC_STYLE_LINENUMBER, LINENUMBERS);

    // line numbers margin size:
    t.SetMarginType(0, wxSTC_MARGIN_NUMBER);
    t.SetMarginWidth(0,
                     t.TextWidth(wxSTC_STYLE_LINENUMBER, "9999 "));
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

    t.StyleSetSpec(wxSTC_PAS_STRING, STRING);
    t.StyleSetSpec(wxSTC_PAS_COMMENT, COMMENT);
    t.StyleSetSpec(wxSTC_PAS_COMMENT2, COMMENT);
    t.StyleSetSpec(wxSTC_PAS_COMMENTLINE, COMMENT);
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


wxFileName FileViewer::GetFilename(wxString ref) const
{
    if ( ref.length() >= 3 &&
         ref[1] == _T(':') &&
         (ref[2] == _T('\\') || ref[2] == _T('/')) )
    {
        // This is an absolute Windows path (c:\foo... or c:/foo...); fix
        // the latter case.
        ref.Replace("/", "\\");
    }

    wxPathFormat pathfmt = ref.Contains(_T('\\')) ? wxPATH_WIN : wxPATH_UNIX;
    wxFileName filename(ref.BeforeLast(_T(':')), pathfmt);

    if ( filename.IsRelative() )
    {
        wxFileName relative(filename);
        wxString basePath(m_basePath);

        // Sometimes, the path in source reference is not relative to the PO
        // file's location, but is relative to e.g. the root directory. See
        // https://code.djangoproject.com/ticket/13936 for exhaustive
        // discussion with plenty of examples.
        //
        // Deal with this by trying parent directories of m_basePath too. So if
        // a file named project/locales/cs/foo.po has a reference to src/main.c,
        // try not only project/locales/cs/src/main.c, but also
        // project/locales/src/main.c and project/src/main.c etc.
        while ( !basePath.empty() )
        {
            filename = relative;
            filename.MakeAbsolute(basePath);
            if ( filename.FileExists() )
            {
                break; // good, found the file
            }
            else
            {
                // remove the last path component
                size_t last = basePath.find_last_of("\\/");
                if ( last == wxString::npos )
                    break;
                else
                    basePath.erase(last);
            }
        }
    }

    return filename;
}


void FileViewer::ShowReference(const wxString& ref)
{
    const wxFileName filename = GetFilename(ref);
    wxFFile file;
    wxString data;

    if ( !filename.IsFileReadable() ||
         !file.Open(filename.GetFullPath()) ||
         !file.ReadAll(&data, wxConvAuto()) )
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
    m_text->SetValue(data);
    m_text->SetReadOnly(true);

    m_text->MarkerDeleteAll(1);
    m_text->MarkerAdd((int)linenum - 1, 1);

    // Center the highlighted line:
    int lineHeight = m_text->TextHeight((int)linenum);
    int linesInWnd = m_text->GetSize().y / lineHeight;
    m_text->ScrollToLine(wxMax(0, (int)linenum - linesInWnd/2));

}


void FileViewer::OnChoice(wxCommandEvent &event)
{
    ShowReference(m_references[event.GetSelection()]);
}

void FileViewer::OnEditFile(wxCommandEvent&)
{
    wxLaunchDefaultApplication(GetFilename(m_current).GetFullPath());
}