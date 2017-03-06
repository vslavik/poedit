/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 1999-2017 Vaclav Slavik
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

#include "fileviewer.h"

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

#include "customcontrols.h"
#include "hidpi.h"
#include "utility.h"
#include "unicode_helpers.h"

namespace
{

#ifdef __WXOSX__
const int FRAME_STYLE = wxDEFAULT_FRAME_STYLE | wxFRAME_TOOL_WINDOW;
#else
const int FRAME_STYLE = wxDEFAULT_FRAME_STYLE;
#endif

} // anonymous namespace

FileViewer *FileViewer::ms_instance = nullptr;

FileViewer *FileViewer::GetAndActivate()
{
    if (!ms_instance)
        ms_instance = new FileViewer(nullptr);
    ms_instance->Show();
    ms_instance->Raise();
    return ms_instance;
}

FileViewer::FileViewer(wxWindow*)
        : wxFrame(nullptr, wxID_ANY, _("Source file"), wxDefaultPosition, wxDefaultSize, FRAME_STYLE)
{
    SetName("fileviewer");

    wxPanel *panel = new wxPanel(this, -1);
    wxSizer *sizer = new wxBoxSizer(wxVERTICAL);
    panel->SetSizer(sizer);
#ifdef __WXOSX__
    panel->SetWindowVariant(wxWINDOW_VARIANT_SMALL);
#endif

    wxSizer *barsizer = new wxBoxSizer(wxHORIZONTAL);
    sizer->Add(barsizer, wxSizerFlags().Expand().PXBorderAll());

    barsizer->Add(new wxStaticText(panel, wxID_ANY,
                                   _("Source file occurrence:")),
                  wxSizerFlags().Center().PXBorder(wxRIGHT));

    m_file = new wxChoice(panel, wxID_ANY);
    barsizer->Add(m_file, wxSizerFlags(1).Center());

    m_openInEditor = new wxButton(panel, wxID_ANY, MSW_OR_OTHER(_("Open in editor"), _("Open in Editor")));
    barsizer->Add(m_openInEditor, wxSizerFlags().Center().Border(wxLEFT, PX(10)));

    m_text = new wxStyledTextCtrl(panel, wxID_ANY,
                                  wxDefaultPosition, wxDefaultSize,
                                  wxBORDER_THEME);
    SetupTextCtrl();
    sizer->Add(m_text, 1, wxEXPAND);

    m_error = new wxStaticText(panel, wxID_ANY, "");
    m_error->SetForegroundColour(ExplanationLabel::GetTextColor());
    m_error->SetFont(m_error->GetFont().Larger().Larger());
    sizer->Add(m_error, wxSizerFlags(1).Center().Border(wxTOP|wxBOTTOM, PX(80)));

    RestoreWindowState(this, wxSize(PX(600), PX(400)));

    wxSizer *topsizer = new wxBoxSizer(wxVERTICAL);
    topsizer->Add(panel, wxSizerFlags(1).Expand());
    SetSizer(topsizer);

    // avoid flicker with these initial settings:
    m_file->Disable();
    sizer->Hide(m_error);
    sizer->Hide(m_text);

    Layout();

    m_file->Bind(wxEVT_CHOICE, &FileViewer::OnChoice, this);
    m_openInEditor->Bind(wxEVT_BUTTON, &FileViewer::OnEditFile, this);

#ifdef __WXOSX__
    wxAcceleratorEntry entries[] = {
        { wxACCEL_CMD, 'W', wxID_CLOSE }
    };
    wxAcceleratorTable accel(WXSIZEOF(entries), entries);
    SetAcceleratorTable(accel);

    Bind(wxEVT_MENU, [=](wxCommandEvent&){ Destroy(); }, wxID_CLOSE);
#endif
}


FileViewer::~FileViewer()
{
    ms_instance = nullptr;
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
                                      #ifdef __WXOSX__
                                         font.GetPointSize() - 1
                                      #else
                                         font.GetPointSize()
                                      #endif
                                         );
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
                return filename; // good, found the file
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

    return wxFileName(); // invalid
}


void FileViewer::ShowReferences(CatalogPtr catalog, CatalogItemPtr item, int defaultReference)
{
    m_basePath = catalog->GetSourcesBasePath();
    if (m_basePath.empty())
        m_basePath = wxPathOnly(catalog->GetFileName());

    if (item)
        m_references = item->GetReferences();

    m_file->Clear();
    m_file->Enable(m_references.size() > 1);

    if (m_references.empty())
    {
        ShowError(_("No references for the selected item."));
    }
    else
    {
        for (auto& r: m_references)
            m_file->Append(bidi::platform_mark_direction(r));
        m_file->SetSelection(defaultReference);
        SelectReference(m_references[defaultReference]);
    }
}

void FileViewer::SelectReference(const wxString& ref)
{
    const wxFileName filename = GetFilename(ref);
    if (!filename.IsOk())
    {
        ShowError(wxString::Format(_("Error opening file %s!"), filename.GetFullPath()));
        m_openInEditor->Disable();
        return;
    }

    wxFFile file;
    wxString data;

    if ( !filename.IsFileReadable() ||
         !file.Open(filename.GetFullPath()) ||
         !file.ReadAll(&data, wxConvAuto()) )
    {
        ShowError(wxString::Format(_("Error opening file %s!"), filename.GetFullPath()));
        m_openInEditor->Disable();
        return;
    }

    m_openInEditor->Enable();

    m_error->GetContainingSizer()->Hide(m_error);
    m_text->GetContainingSizer()->Show(m_text);
    Layout();

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

void FileViewer::ShowError(const wxString& msg)
{
    m_error->SetLabel(msg);
    m_error->GetContainingSizer()->Show(m_error);
    m_text->GetContainingSizer()->Hide(m_text);
    Layout();
}


void FileViewer::OnChoice(wxCommandEvent &event)
{
    SelectReference(m_references[event.GetSelection()]);
}

void FileViewer::OnEditFile(wxCommandEvent&)
{
    wxFileName filename = GetFilename(bidi::strip_control_chars(m_file->GetStringSelection()));
    if (filename.IsOk())
        wxLaunchDefaultApplication(filename.GetFullPath());
}
