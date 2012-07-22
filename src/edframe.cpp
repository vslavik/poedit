/*
 *  This file is part of Poedit (http://www.poedit.net)
 *
 *  Copyright (C) 1999-2012 Vaclav Slavik
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

#include <wx/wx.h>
#include <wx/config.h>
#include <wx/html/htmlwin.h>
#include <wx/statline.h>
#include <wx/sizer.h>
#include <wx/fs_mem.h>
#include <wx/datetime.h>
#include <wx/tokenzr.h>
#include <wx/xrc/xmlres.h>
#include <wx/settings.h>
#include <wx/button.h>
#include <wx/statusbr.h>
#include <wx/splitter.h>
#include <wx/fontutil.h>
#include <wx/textfile.h>
#include <wx/wupdlock.h>
#include <wx/aboutdlg.h>
#include <wx/iconbndl.h>
#include <wx/clipbrd.h>
#include <wx/dnd.h>

#ifdef USE_SPARKLE
#include "osx/sparkle.h"
#endif // USE_SPARKLE

#ifdef USE_SPELLCHECKING

#ifdef __WXGTK__
    #include <gtk/gtk.h>
    extern "C" {
    #include <gtkspell/gtkspell.h>
    }
#endif // __WXGTK__

#endif // USE_SPELLCHECKING

#include <map>
#include <algorithm>

#include "catalog.h"
#include "edapp.h"
#include "edframe.h"
#include "propertiesdlg.h"
#include "prefsdlg.h"
#include "fileviewer.h"
#include "findframe.h"
#include "transmem.h"
#include "isocodes.h"
#include "progressinfo.h"
#include "commentdlg.h"
#include "manager.h"
#include "pluralforms/pl_evaluate.h"
#include "attentionbar.h"
#include "utility.h"

#include <wx/listimpl.cpp>
WX_DEFINE_LIST(PoeditFramesList);
PoeditFramesList PoeditFrame::ms_instances;


// this should be high enough to not conflict with any wxNewId-allocated value,
// but there's a check for this in the PoeditFrame ctor, too
const wxWindowID ID_POEDIT_FIRST = wxID_HIGHEST + 10000;
const unsigned   ID_POEDIT_STEP  = 1000;

const wxWindowID ID_POPUP_REFS   = ID_POEDIT_FIRST + 1*ID_POEDIT_STEP;
const wxWindowID ID_POPUP_TRANS  = ID_POEDIT_FIRST + 2*ID_POEDIT_STEP;
const wxWindowID ID_POPUP_DUMMY  = ID_POEDIT_FIRST + 3*ID_POEDIT_STEP;
const wxWindowID ID_BOOKMARK_GO  = ID_POEDIT_FIRST + 4*ID_POEDIT_STEP;
const wxWindowID ID_BOOKMARK_SET = ID_POEDIT_FIRST + 5*ID_POEDIT_STEP;

const wxWindowID ID_POEDIT_LAST  = ID_POEDIT_FIRST + 6*ID_POEDIT_STEP;

const wxWindowID ID_LIST = wxNewId();
const wxWindowID ID_TEXTORIG = wxNewId();
const wxWindowID ID_TEXTORIGPLURAL = wxNewId();
const wxWindowID ID_TEXTTRANS = wxNewId();
const wxWindowID ID_TEXTCOMMENT = wxNewId();



#ifdef __VISUALC__
// Disabling the useless and annoying MSVC++'s
// warning C4800: 'long' : forcing value to bool 'true' or 'false'
// (performance warning):
#pragma warning ( disable : 4800 )
#endif


// I don't like this global flag, but all PoeditFrame instances must share it
bool g_focusToText = false;

/*static*/ PoeditFrame *PoeditFrame::Find(const wxString& filename)
{
    for (PoeditFramesList::const_iterator n = ms_instances.begin();
         n != ms_instances.end(); ++n)
    {
        if ((*n)->m_fileName == filename)
            return *n;
    }
    return NULL;
}

/*static*/ PoeditFrame *PoeditFrame::Create(const wxString& filename)
{
    PoeditFrame *f;
    if (!filename)
    {
        f = new PoeditFrame;
    }
    else
    {
        f = PoeditFrame::Find(filename);
        if (!f)
        {
            f = new PoeditFrame();
            f->Show(true);
            f->ReadCatalog(filename);
        }
    }
    f->Show(true);

    if (g_focusToText)
        f->m_textTrans->SetFocus();
    else
        f->m_list->SetFocus();

    return f;
}


class ListHandler;
class TextctrlHandler : public wxEvtHandler
{
    public:
        TextctrlHandler(PoeditFrame* frame) : m_list(frame->m_list) {}

    private:
#ifdef __WXMSW__
        // We use wxTE_RICH2 style, which allows for pasting rich-formatted
        // text into the control. We want to allow only plain text (all the
        // formatting done is Poedit's syntax highlighting), so we need to
        // override copy/cut/paste command.s Plus, the richedit control
        // (or wx's use of it) has a bug in it that causes it to copy wrong
        // data when copying from the same text control to itself after its
        // content was programatically changed:
        // https://sourceforge.net/tracker/index.php?func=detail&aid=1910234&group_id=27043&atid=389153

        bool DoCopy(wxTextCtrl *textctrl)
        {
            long from, to;
            textctrl->GetSelection(&from, &to);
            if ( from == to )
                return false;

            const wxString sel = textctrl->GetRange(from, to);

            wxClipboardLocker lock;
            wxCHECK_MSG( !!lock, false, _T("failed to lock clipboard") );

            wxClipboard::Get()->SetData(new wxTextDataObject(sel));
            return true;
        }

        void OnCopy(wxClipboardTextEvent& event)
        {
            wxTextCtrl *textctrl = dynamic_cast<wxTextCtrl*>(event.GetEventObject());
            wxCHECK_RET( textctrl, _T("wrong use of event handler") );

            DoCopy(textctrl);
        }

        void OnCut(wxClipboardTextEvent& event)
        {
            wxTextCtrl *textctrl = dynamic_cast<wxTextCtrl*>(event.GetEventObject());
            wxCHECK_RET( textctrl, _T("wrong use of event handler") );

            if ( !DoCopy(textctrl) )
                return;

            long from, to;
            textctrl->GetSelection(&from, &to);
            textctrl->Remove(from, to);
        }

        void OnPaste(wxClipboardTextEvent& event)
        {
            wxTextCtrl *textctrl = dynamic_cast<wxTextCtrl*>(event.GetEventObject());
            wxCHECK_RET( textctrl, _T("wrong use of event handler") );

            wxClipboardLocker lock;
            wxCHECK_RET( !!lock, _T("failed to lock clipboard") );

            wxTextDataObject d;
            wxClipboard::Get()->GetData(d);

            long from, to;
            textctrl->GetSelection(&from, &to);
            textctrl->Replace(from, to, d.GetText());
        }
#endif // __WXMSW__

        DECLARE_EVENT_TABLE()

        PoeditListCtrl *m_list;

        friend class ListHandler;
};

BEGIN_EVENT_TABLE(TextctrlHandler, wxEvtHandler)
#ifdef __WXMSW__
    EVT_TEXT_COPY(-1, TextctrlHandler::OnCopy)
    EVT_TEXT_CUT(-1, TextctrlHandler::OnCut)
    EVT_TEXT_PASTE(-1, TextctrlHandler::OnPaste)
#endif
END_EVENT_TABLE()


class TransTextctrlHandler : public TextctrlHandler
{
    public:
        TransTextctrlHandler(PoeditFrame* frame)
            : TextctrlHandler(frame), m_frame(frame) {}

    private:
        void OnText(wxCommandEvent& event)
        {
            m_frame->UpdateFromTextCtrl();
            event.Skip();
        }

        PoeditFrame *m_frame;

        DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(TransTextctrlHandler, TextctrlHandler)
    EVT_TEXT(-1, TransTextctrlHandler::OnText)
END_EVENT_TABLE()


// special handling of events in listctrl
class ListHandler : public wxEvtHandler
{
    public:
        ListHandler(PoeditFrame *frame) :
                 wxEvtHandler(), m_frame(frame) {}

    private:
        void OnSel(wxListEvent& event) { m_frame->OnListSel(event); }
        void OnActivated(wxListEvent& event) { m_frame->OnListActivated(event); }
        void OnRightClick(wxMouseEvent& event) { m_frame->OnListRightClick(event); }
        void OnFocus(wxFocusEvent& event) { m_frame->OnListFocus(event); }

        DECLARE_EVENT_TABLE()

        PoeditFrame *m_frame;
};

BEGIN_EVENT_TABLE(ListHandler, wxEvtHandler)
   EVT_LIST_ITEM_SELECTED  (ID_LIST, ListHandler::OnSel)
   EVT_LIST_ITEM_ACTIVATED (ID_LIST, ListHandler::OnActivated)
   EVT_RIGHT_DOWN          (          ListHandler::OnRightClick)
   EVT_SET_FOCUS           (          ListHandler::OnFocus)
END_EVENT_TABLE()


class UnfocusableTextCtrl : public wxTextCtrl
{
    public:
        UnfocusableTextCtrl(wxWindow *parent,
                            wxWindowID id,
                            const wxString &value = wxEmptyString,
                            const wxPoint &pos = wxDefaultPosition,
                            const wxSize &size = wxDefaultSize,
                            long style = 0,
                            const wxValidator& validator = wxDefaultValidator,
                            const wxString &name = wxTextCtrlNameStr)
           : wxTextCtrl(parent, id, value, pos, size, style, validator, name) {}
        virtual bool AcceptsFocus() const { return false; }
};


BEGIN_EVENT_TABLE(PoeditFrame, wxFrame)
   EVT_MENU           (wxID_EXIT,                 PoeditFrame::OnQuit)
   EVT_MENU           (wxID_CLOSE,                PoeditFrame::OnCloseCmd)
   EVT_MENU           (wxID_HELP,                 PoeditFrame::OnHelp)
   EVT_MENU           (wxID_ABOUT,                PoeditFrame::OnAbout)
   EVT_MENU           (wxID_NEW,                  PoeditFrame::OnNew)
   EVT_MENU           (XRCID("menu_new_from_pot"),PoeditFrame::OnNew)
   EVT_MENU           (wxID_OPEN,                 PoeditFrame::OnOpen)
   EVT_MENU           (wxID_SAVE,                 PoeditFrame::OnSave)
   EVT_MENU           (wxID_SAVEAS,               PoeditFrame::OnSaveAs)
   EVT_MENU           (XRCID("menu_export"),      PoeditFrame::OnExport)
   EVT_MENU_RANGE     (wxID_FILE1, wxID_FILE9,    PoeditFrame::OnOpenHist)
   EVT_MENU           (XRCID("menu_catproperties"), PoeditFrame::OnProperties)
   EVT_MENU           (wxID_PREFERENCES,          PoeditFrame::OnPreferences)
   EVT_MENU           (XRCID("menu_update"),      PoeditFrame::OnUpdate)
   EVT_MENU           (XRCID("menu_update_from_pot"),PoeditFrame::OnUpdate)
   EVT_MENU           (XRCID("menu_purge_deleted"), PoeditFrame::OnPurgeDeleted)
   EVT_MENU           (XRCID("menu_fuzzy"),       PoeditFrame::OnFuzzyFlag)
   EVT_MENU           (XRCID("menu_quotes"),      PoeditFrame::OnQuotesFlag)
   EVT_MENU           (XRCID("menu_lines"),       PoeditFrame::OnLinesFlag)
   EVT_MENU           (XRCID("menu_comment_win"), PoeditFrame::OnCommentWinFlag)
   EVT_MENU           (XRCID("menu_auto_comments_win"), PoeditFrame::OnAutoCommentsWinFlag)
   EVT_MENU           (XRCID("sort_by_order"),    PoeditFrame::OnSortByFileOrder)
   EVT_MENU           (XRCID("sort_by_source"),    PoeditFrame::OnSortBySource)
   EVT_MENU           (XRCID("sort_by_translation"), PoeditFrame::OnSortByTranslation)
   EVT_MENU           (XRCID("sort_untrans_first"), PoeditFrame::OnSortUntranslatedFirst)
   EVT_MENU           (XRCID("menu_copy_from_src"), PoeditFrame::OnCopyFromSource)
   EVT_MENU           (XRCID("menu_clear"),       PoeditFrame::OnClearTranslation)
   EVT_MENU           (XRCID("menu_references"),  PoeditFrame::OnReferencesMenu)
   EVT_MENU           (wxID_FIND,                 PoeditFrame::OnFind)
   EVT_MENU           (XRCID("menu_comment"),     PoeditFrame::OnEditComment)
   EVT_MENU           (XRCID("menu_manager"),     PoeditFrame::OnManager)
   EVT_MENU           (XRCID("go_done_and_next"),   PoeditFrame::OnDoneAndNext)
   EVT_MENU           (XRCID("go_prev"),            PoeditFrame::OnPrev)
   EVT_MENU           (XRCID("go_next"),            PoeditFrame::OnNext)
   EVT_MENU           (XRCID("go_prev_page"),       PoeditFrame::OnPrevPage)
   EVT_MENU           (XRCID("go_next_page"),       PoeditFrame::OnNextPage)
   EVT_MENU           (XRCID("go_prev_unfinished"), PoeditFrame::OnPrevUnfinished)
   EVT_MENU           (XRCID("go_next_unfinished"), PoeditFrame::OnNextUnfinished)
   EVT_MENU_RANGE     (ID_POPUP_REFS, ID_POPUP_REFS + 999, PoeditFrame::OnReference)
#ifdef USE_TRANSMEM
   EVT_MENU_RANGE     (ID_POPUP_TRANS, ID_POPUP_TRANS + 999,
                       PoeditFrame::OnAutoTranslate)
   EVT_MENU           (XRCID("menu_auto_translate"), PoeditFrame::OnAutoTranslateAll)
#endif
   EVT_MENU_RANGE     (ID_BOOKMARK_GO, ID_BOOKMARK_GO + 9,
                       PoeditFrame::OnGoToBookmark)
   EVT_MENU_RANGE     (ID_BOOKMARK_SET, ID_BOOKMARK_SET + 9,
                       PoeditFrame::OnSetBookmark)
   EVT_CLOSE          (                PoeditFrame::OnCloseWindow)
   EVT_TEXT           (ID_TEXTCOMMENT,PoeditFrame::OnCommentWindowText)
   EVT_IDLE           (PoeditFrame::OnIdle)
   EVT_SIZE           (PoeditFrame::OnSize)
   EVT_END_PROCESS    (-1, PoeditFrame::OnEndProcess)
END_EVENT_TABLE()



class PoeditDropTarget : public wxFileDropTarget
{
public:
    PoeditDropTarget(PoeditFrame *win) : m_win(win) {}

    virtual bool OnDropFiles(wxCoord /*x*/, wxCoord /*y*/,
                             const wxArrayString& files)
    {
        if ( files.size() != 1 )
        {
            wxLogError(_("You can't drop more than one file on Poedit window."));
            return false;
        }

        wxFileName f(files[0]);

        if ( f.GetExt().Lower() != _T("po") )
        {
            wxLogError(_("File '%s' is not a message catalog."),
                       f.GetFullPath().c_str());
            return false;
        }

        if ( !f.FileExists() )
        {
            wxLogError(_("File '%s' doesn't exist."), f.GetFullPath().c_str());
            return false;
        }

        m_win->OpenFile(f.GetFullPath());

        return true;
    }

private:
    PoeditFrame *m_win;
};


// Frame class:

PoeditFrame::PoeditFrame() :
    wxFrame(NULL, -1, _("Poedit"),
            wxDefaultPosition, wxDefaultSize,
            wxDEFAULT_FRAME_STYLE | wxNO_FULL_REPAINT_ON_RESIZE,
            _T("mainwin")),
#ifdef USE_GETTEXT_VALIDATION
    m_itemBeingValidated(-1), m_gettextProcess(NULL),
#endif
    m_catalog(NULL),
#ifdef USE_TRANSMEM
    m_transMem(NULL),
    m_transMemLoaded(false),
#endif
    m_list(NULL),
    m_modified(false),
    m_hasObsoleteItems(false),
    m_dontAutoclearFuzzyStatus(false),
    m_setSashPositionsWhenMaximized(false)
{
    // make sure that the [ID_POEDIT_FIRST,ID_POEDIT_LAST] range of IDs is not
    // used for anything else:
    wxASSERT_MSG( wxGetCurrentId() < ID_POEDIT_FIRST ||
                  wxGetCurrentId() > ID_POEDIT_LAST,
                  _T("detected ID values conflict!") );
    wxRegisterId(ID_POEDIT_LAST);

#if defined(__WXMSW__)
    const int SPLITTER_FLAGS = wxSP_NOBORDER;
#elif defined(__WXMAC__)
    // wxMac doesn't show XORed line:
    const int SPLITTER_FLAGS = wxSP_LIVE_UPDATE;
#else
    const int SPLITTER_FLAGS = wxSP_3DBORDER;
#endif

    wxConfigBase *cfg = wxConfig::Get();

    m_displayQuotes = (bool)cfg->Read(_T("display_quotes"), (long)false);
    m_displayLines = (bool)cfg->Read(_T("display_lines"), (long)false);
    m_displayCommentWin =
        (bool)cfg->Read(_T("display_comment_win"), (long)false);
    m_displayAutoCommentsWin =
        (bool)cfg->Read(_T("display_auto_comments_win"), (long)false);
    m_commentWindowEditable =
        (bool)cfg->Read(_T("comment_window_editable"), (long)false);
    g_focusToText = (bool)cfg->Read(_T("focus_to_text"), (long)false);

#if defined(__WXGTK__)
    wxIconBundle appicons;
    appicons.AddIcon(wxArtProvider::GetIcon(_T("poedit"), wxART_FRAME_ICON, wxSize(16,16)));
    appicons.AddIcon(wxArtProvider::GetIcon(_T("poedit"), wxART_FRAME_ICON, wxSize(32,32)));
    appicons.AddIcon(wxArtProvider::GetIcon(_T("poedit"), wxART_FRAME_ICON, wxSize(48,48)));
    SetIcons(appicons);
#elif defined(__WXMSW__)
    SetIcon(wxICON(appicon));
#endif

    m_boldGuiFont = wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT);
    m_boldGuiFont.SetWeight(wxFONTWEIGHT_BOLD);

    wxMenuBar *MenuBar = wxXmlResource::Get()->LoadMenuBar(_T("mainmenu"));
    if (MenuBar)
    {
        wxString menuName(_("&File"));
        menuName.Replace(wxT("&"), wxEmptyString);
        m_history.UseMenu(MenuBar->GetMenu(MenuBar->FindMenu(menuName)));
        SetMenuBar(MenuBar);
        m_history.AddFilesToMenu();
        m_history.Load(*cfg);
#ifndef USE_TRANSMEM
        MenuBar->Enable(XRCID("menu_auto_translate"), false);
#endif
        AddBookmarksMenu(MenuBar->GetMenu(MenuBar->FindMenu(_("&Go"))));

#if USE_SPARKLE
        Sparkle_AddMenuItem(_("Check for Updates...").utf8_str());
#endif
    }
    else
    {
        wxLogError(_T("Cannot load main menu from resource, something must have went terribly wrong."));
        wxLog::FlushActive();
    }

    wxXmlResource::Get()->LoadToolBar(this, _T("toolbar"));

    GetMenuBar()->Check(XRCID("menu_quotes"), m_displayQuotes);
    GetMenuBar()->Check(XRCID("menu_lines"), m_displayLines);
    GetMenuBar()->Check(XRCID("menu_comment_win"), m_displayCommentWin);
    GetMenuBar()->Check(XRCID("menu_auto_comments_win"), m_displayAutoCommentsWin);

    CreateStatusBar(1, wxST_SIZEGRIP);

    m_splitter = new wxSplitterWindow(this, -1,
                                      wxDefaultPosition, wxDefaultSize,
                                      SPLITTER_FLAGS);
    // make only the upper part grow when resizing
    m_splitter->SetSashGravity(1.0);

    wxSizer *mainSizer = new wxBoxSizer(wxHORIZONTAL);
    mainSizer->Add(m_splitter, wxSizerFlags(1).Expand());
    SetSizer(mainSizer);

    wxPanel *topPanel = new wxPanel(m_splitter, wxID_ANY);

    m_attentionBar = new AttentionBar(topPanel);

    m_list = new PoeditListCtrl(topPanel,
                                ID_LIST,
                                wxDefaultPosition, wxDefaultSize,
                                wxLC_REPORT | wxLC_SINGLE_SEL,
                                m_displayLines);

    wxSizer *topSizer = new wxBoxSizer(wxVERTICAL);
    topSizer->Add(m_attentionBar, wxSizerFlags().Expand());
    topSizer->Add(m_list, wxSizerFlags(1).Expand());
    topPanel->SetSizer(topSizer);

    m_bottomSplitter = new wxSplitterWindow(m_splitter, -1,
                                            wxDefaultPosition, wxDefaultSize,
                                            SPLITTER_FLAGS);
    // left part (translation) should grow, not comments one:
    m_bottomSplitter->SetSashGravity(1.0);

    m_bottomLeftPanel = new wxPanel(m_bottomSplitter);
    m_bottomRightPanel = new wxPanel(m_bottomSplitter);

    wxStaticText *labelSource =
        new wxStaticText(m_bottomLeftPanel, -1, _("Source text:"));
    labelSource->SetFont(m_boldGuiFont);

    wxStaticText *labelTrans =
        new wxStaticText(m_bottomLeftPanel, -1, _("Translation:"));
    labelTrans->SetFont(m_boldGuiFont);

    m_labelComment = new wxStaticText(m_bottomRightPanel, -1, _("Comment:"));
    m_labelComment->SetFont(m_boldGuiFont);

    m_labelAutoComments = new wxStaticText(m_bottomRightPanel, -1, _("Automatic comments:"));
    m_labelAutoComments->SetFont(m_boldGuiFont);

    m_textComment = NULL;
    m_textAutoComments = new UnfocusableTextCtrl(m_bottomRightPanel,
                                ID_TEXTORIG, wxEmptyString,
                                wxDefaultPosition, wxDefaultSize,
                                wxTE_MULTILINE | wxTE_RICH2 | wxTE_READONLY);
    // This call will force the creation of the right kind of control
    // for the m_textComment member
    UpdateCommentWindowEditable();

    m_labelSingular = new wxStaticText(m_bottomLeftPanel, -1, _("Singular:"));
    m_labelPlural = new wxStaticText(m_bottomLeftPanel, -1, _("Plural:"));
    m_textOrig = new UnfocusableTextCtrl(m_bottomLeftPanel,
                                ID_TEXTORIG, wxEmptyString,
                                wxDefaultPosition, wxDefaultSize,
                                wxTE_MULTILINE | wxTE_RICH2 | wxTE_READONLY);
    m_textOrigPlural = new UnfocusableTextCtrl(m_bottomLeftPanel,
                                ID_TEXTORIGPLURAL, wxEmptyString,
                                wxDefaultPosition, wxDefaultSize,
                                wxTE_MULTILINE | wxTE_RICH2 | wxTE_READONLY);

    m_textTrans = new wxTextCtrl(m_bottomLeftPanel,
                                ID_TEXTTRANS, wxEmptyString,
                                wxDefaultPosition, wxDefaultSize,
                                wxTE_MULTILINE | wxTE_RICH2);

    // in case of plurals form, this is the control for n=1:
    m_textTransSingularForm = NULL;

    m_pluralNotebook = new wxNotebook(m_bottomLeftPanel, -1);

    SetCustomFonts();
    SetAccelerators();

    wxSizer *leftSizer = new wxBoxSizer(wxVERTICAL);
    wxSizer *rightSizer = new wxBoxSizer(wxVERTICAL);

    wxFlexGridSizer *gridSizer = new wxFlexGridSizer(2);
    gridSizer->AddGrowableCol(1);
    gridSizer->AddGrowableRow(0);
    gridSizer->AddGrowableRow(1);
    gridSizer->Add(m_labelSingular, 0, wxALIGN_CENTER_VERTICAL | wxALL, 3);
    gridSizer->Add(m_textOrig, 1, wxEXPAND);
    gridSizer->Add(m_labelPlural, 0, wxALIGN_CENTER_VERTICAL | wxALL, 3);
    gridSizer->Add(m_textOrigPlural, 1, wxEXPAND);
    gridSizer->SetItemMinSize(m_textOrig, 1, 1);
    gridSizer->SetItemMinSize(m_textOrigPlural, 1, 1);

    leftSizer->Add(labelSource, 0, wxEXPAND | wxALL, 3);
    leftSizer->Add(gridSizer, 1, wxEXPAND);
    leftSizer->Add(labelTrans, 0, wxEXPAND | wxALL, 3);
    leftSizer->Add(m_textTrans, 1, wxEXPAND);
    leftSizer->Add(m_pluralNotebook, 1, wxEXPAND);
    rightSizer->Add(m_labelAutoComments, 0, wxEXPAND | wxALL, 3);
    rightSizer->Add(m_textAutoComments, 1, wxEXPAND);
    rightSizer->Add(m_labelComment, 0, wxEXPAND | wxALL, 3);
    rightSizer->Add(m_textComment, 1, wxEXPAND);

    m_bottomLeftPanel->SetAutoLayout(true);
    m_bottomLeftPanel->SetSizer(leftSizer);

    m_bottomRightPanel->SetAutoLayout(true);
    m_bottomRightPanel->SetSizer(rightSizer);

    m_bottomSplitter->SetMinimumPaneSize(150);
    m_bottomRightPanel->Show(false);
    m_bottomSplitter->Initialize(m_bottomLeftPanel);


    int restore_flags = WinState_Size;
    // NB: if this is the only Poedit frame opened, place it at remembered
    //     position, but don't do that if there already are other frames,
    //     because they would overlap and nobody could recognize that there are
    //     many of them
    if (ms_instances.GetCount() == 0)
        restore_flags |= WinState_Pos;
    RestoreWindowState(this, wxSize(780, 570), restore_flags);

    // This is a hack -- windows are not maximized immediately and so we can't
    // set correct sash position in ctor (unmaximized window may be too small
    // for sash position when maximized -- see bug #2120600)
    if ( cfg->Read(WindowStatePath(this) + _T("maximized"), long(0)) )
        m_setSashPositionsWhenMaximized = true;

    Layout();

    m_splitter->SetMinimumPaneSize(120);
    m_splitter->SplitHorizontally(topPanel, m_bottomSplitter, cfg->Read(_T("splitter"), 330L));

    m_list->PushEventHandler(new ListHandler(this));
    m_textTrans->PushEventHandler(new TransTextctrlHandler(this));
    m_textComment->PushEventHandler(new TextctrlHandler(this));

    ShowPluralFormUI(false);

    UpdateMenu();
    UpdateDisplayCommentWin();

    switch ( m_list->sortOrder.by )
    {
        case SortOrder::By_FileOrder:
            MenuBar->Check(XRCID("sort_by_order"), true);
            break;
        case SortOrder::By_Source:
            MenuBar->Check(XRCID("sort_by_source"), true);
            break;
        case SortOrder::By_Translation:
            MenuBar->Check(XRCID("sort_by_translation"), true);
            break;
    }
    MenuBar->Check(XRCID("sort_untrans_first"), m_list->sortOrder.untransFirst);

    ms_instances.Append(this);

    SetDropTarget(new PoeditDropTarget(this));
}



PoeditFrame::~PoeditFrame()
{
#ifdef USE_GETTEXT_VALIDATION
    if (m_gettextProcess)
        m_gettextProcess->Detach();
#endif

    ms_instances.DeleteObject(this);

    m_list->PopEventHandler(true/*delete*/);
    m_textTrans->PopEventHandler(true/*delete*/);
    for (size_t i = 0; i < m_textTransPlural.size(); i++)
        m_textTransPlural[i]->PopEventHandler(true/*delete*/);
    m_textComment->PopEventHandler(true/*delete*/);

    wxConfigBase *cfg = wxConfig::Get();
    cfg->SetPath(_T("/"));

    if (m_displayCommentWin || m_displayAutoCommentsWin)
    {
        cfg->Write(_T("bottom_splitter"),
                   (long)m_bottomSplitter->GetSashPosition());
    }
    cfg->Write(_T("splitter"), (long)m_splitter->GetSashPosition());
    cfg->Write(_T("display_quotes"), m_displayQuotes);
    cfg->Write(_T("display_lines"), m_displayLines);
    cfg->Write(_T("display_comment_win"), m_displayCommentWin);
    cfg->Write(_T("display_auto_comments_win"), m_displayAutoCommentsWin);

    SaveWindowState(this);

    m_history.Save(*cfg);

    // write all changes:
    cfg->Flush();

#ifdef USE_TRANSMEM
    if (m_transMem)
        m_transMem->Release();
#endif

    delete m_catalog;
    m_catalog = NULL;
    m_list->CatalogChanged(NULL);

    // shutdown the spellchecker:
    InitSpellchecker();
}


void PoeditFrame::SetAccelerators()
{
    wxAcceleratorEntry entries[7];

    entries[0].Set(wxACCEL_CTRL, WXK_PAGEUP,          XRCID("go_prev_page"));
    entries[1].Set(wxACCEL_CTRL, WXK_NUMPAD_PAGEUP,   XRCID("go_prev_page"));
    entries[2].Set(wxACCEL_CTRL, WXK_PAGEDOWN,        XRCID("go_next_page"));
    entries[3].Set(wxACCEL_CTRL, WXK_NUMPAD_PAGEDOWN, XRCID("go_next_page"));

    entries[4].Set(wxACCEL_CTRL, WXK_NUMPAD_UP,       XRCID("go_prev"));
    entries[5].Set(wxACCEL_CTRL, WXK_NUMPAD_DOWN,     XRCID("go_next"));

    entries[6].Set(wxACCEL_CTRL, WXK_NUMPAD_ENTER,    XRCID("go_done_and_next"));

    wxAcceleratorTable accel(WXSIZEOF(entries), entries);
    SetAcceleratorTable(accel);
}


#ifdef USE_SPELLCHECKING

#ifdef __WXGTK__
// helper functions that finds GtkTextView of wxTextCtrl:
static GtkTextView *GetTextView(wxTextCtrl *ctrl)
{
    GtkWidget *parent = ctrl->m_widget;
    GList *child = gtk_container_get_children(GTK_CONTAINER(parent));
    while (child)
    {
        if (GTK_IS_TEXT_VIEW(child->data))
        {
            return GTK_TEXT_VIEW(child->data);
        }
        child = child->next;
    }

    wxFAIL_MSG( _T("couldn't find GtkTextView for text control") );
    return NULL;
}
#endif // __WXGTK__

#ifdef __WXOSX__
#include "osx/spellchecker.h"
#endif // __WXOSX__

#ifdef __WXGTK__
static bool DoInitSpellchecker(wxTextCtrl *text,
                               bool enable, const wxString& lang)
{
    GtkTextView *textview = GetTextView(text);
    wxASSERT_MSG( textview, _T("wxTextCtrl is supposed to use GtkTextView") );
    GtkSpell *spell = gtkspell_get_from_text_view(textview);

    GError *err = NULL;

    if (enable)
    {
        if (spell)
            gtkspell_set_language(spell, lang.ToAscii(), &err);
        else
            gtkspell_new_attach(textview, lang.ToAscii(), &err);
    }
    else // !enable
    {
        // GtkSpell when used with Zemberek Enchant module doesn't work
        // correctly if you repeatedly attach and detach a speller to text
        // view. See http://www.poedit.net/trac/ticket/276 for details.
        //
        // To work around this, we set the language to a non-existent one
        // instead of detaching GtkSpell -- this has the same effect as
        // detaching the speller as far as the UI is concerned.
        if (spell)
            gtkspell_set_language(spell, "unknown_language", &err);
    }

    if (err)
        g_error_free(err);

    return err == NULL;
}
#endif // __WXGTK__

#ifdef __WXOSX__

#include "osx/spellchecker.h"

static bool SetSpellcheckerLang(const wxString& lang)
{
    // FIXME: if this fails, report an error in some unobtrusive way,
    //        tell the user to install cocoaSpell from
    //        http://people.ict.usc.edu/~leuski/cocoaspell/
    return SpellChecker_SetLang(lang.mb_str(wxConvUTF8));
}

static bool DoInitSpellchecker(wxTextCtrl *text,
                               bool enable, const wxString& /*lang*/)
{
    text->MacCheckSpelling(enable);
    return true;
}
#endif // __WXOSX__

static void ShowSpellcheckerHelp()
{
#if defined(__WXOSX__)
    #define SPELL_HELP_PAGE   "SpellcheckerMac"
#elif defined(__UNIX__)
    #define SPELL_HELP_PAGE   "SpellcheckerLinux"
#else
    #error "missing spellchecker instructions for platform"
#endif
    wxLaunchDefaultBrowser(
        wxString::FromAscii("http://www.poedit.net/trac/wiki/Doc/"
                            SPELL_HELP_PAGE));
}

#endif // USE_SPELLCHECKING

void PoeditFrame::InitSpellchecker()
{
#ifdef USE_SPELLCHECKING
    bool report_problem = false;

    wxString lang;
    if (m_catalog) lang = m_catalog->GetLocaleCode();
    bool enabled = m_catalog && !lang.empty() &&
                   wxConfig::Get()->Read(_T("enable_spellchecking"),
                                         (long)true);

#ifdef __WXOSX__
    if (enabled)
    {
        if ( !SetSpellcheckerLang(lang) )
        {
            enabled = false;
            report_problem = true;
        }
    }
#endif

    if ( !DoInitSpellchecker(m_textTrans, enabled, lang) )
        report_problem = true;

    for (size_t i = 0; i < m_textTransPlural.size(); i++)
    {
        if ( !DoInitSpellchecker(m_textTransPlural[i], enabled, lang) )
            report_problem = true;
    }

    if ( enabled && report_problem )
    {
        wxString langname = LookupLanguageName(lang);
        if ( !langname )
            langname = lang;

        AttentionMessage msg
        (
            _T("missing-spell-dict"),
            AttentionMessage::Warning,
            wxString::Format
            (
                // TRANSLATORS: %s is language name in its basic form (as you
                // would see e.g. in a list of supported languages).
                _("Spellchecker dictionary for %s isn't available, you need to install it."),
                langname.c_str()
            )
        );
        msg.AddAction(_("Learn more"), boost::bind(ShowSpellcheckerHelp));
        msg.AddDontShowAgain();
        m_attentionBar->ShowMessage(msg);
    }
#endif // USE_SPELLCHECKING
}



#ifdef USE_TRANSMEM
TranslationMemory *PoeditFrame::GetTransMem()
{
    wxConfigBase *cfg = wxConfig::Get();

    if (m_transMemLoaded)
        return m_transMem;
    else
    {
        wxString lang;

        lang = m_catalog->GetLocaleCode();
        if (!lang)
        {
            wxArrayString lngs;
            int index;
            wxStringTokenizer tkn(cfg->Read(_T("TM/languages"), wxEmptyString), _T(":"));

            lngs.Add(_("(none of these)"));
            while (tkn.HasMoreTokens()) lngs.Add(tkn.GetNextToken());
            if (lngs.GetCount() == 1)
            {
                m_transMemLoaded = true;
                m_transMem = NULL;
                return m_transMem;
            }
            index = wxGetSingleChoiceIndex(_("Select catalog's language"),
                                           _("Please select language code:"),
                                           lngs, this);
            if (index > 0)
                lang = lngs[index];
        }

        if (!lang.empty() && TranslationMemory::IsSupported(lang))
        {
            m_transMem = TranslationMemory::Create(lang);
            if (m_transMem)
                m_transMem->SetParams(cfg->Read(_T("TM/max_delta"), 2),
                                      cfg->Read(_T("TM/max_omitted"), 2));
        }
        else
            m_transMem = NULL;
        m_transMemLoaded = true;
        return m_transMem;
    }
}
#endif



void PoeditFrame::OnQuit(wxCommandEvent&)
{
    if ( !Close() )
        return;
    wxGetApp().ExitMainLoop();
}

void PoeditFrame::OnCloseCmd(wxCommandEvent&)
{
    Close();
}


void PoeditFrame::OpenFile(const wxString& filename)
{
    if ( !CanDiscardCurrentDoc() )
        return;
    DoOpenFile(filename);
}


void PoeditFrame::DoOpenFile(const wxString& filename)
{
    ReadCatalog(filename);

    if (g_focusToText)
        m_textTrans->SetFocus();
    else
        m_list->SetFocus();
}


bool PoeditFrame::CanDiscardCurrentDoc()
{
    if ( m_catalog && m_modified )
    {
        wxMessageDialog dlg
                        (
                            this,
                            _("Catalog modified. Do you want to save changes?"),
                            _("Save changes"),
                            wxYES_NO | wxCANCEL | wxICON_QUESTION
                        );
#if wxCHECK_VERSION(2,9,0)
        dlg.SetExtendedMessage(_("Your changes will be lost if you don't save them."));
        dlg.SetYesNoLabels
            (
                _("Save"),
            #ifdef __WXMSW__
                _("Don't save")
            #else
                _("Don't Save")
            #endif
            );
#endif

        int r = dlg.ShowModal();
        if ( r == wxID_YES )
        {
            if ( !WriteCatalog(m_fileName) )
                return false;
        }
        else if ( r == wxID_CANCEL )
        {
            return false;
        }
    }

    return true;
}


void PoeditFrame::OnCloseWindow(wxCloseEvent& event)
{
    if ( event.CanVeto() && !CanDiscardCurrentDoc() )
    {
        event.Veto();
        return;
    }

    CancelItemsValidation();
    Destroy();
}


void PoeditFrame::OnOpen(wxCommandEvent&)
{
    if ( !CanDiscardCurrentDoc() )
        return;

    wxString path = wxPathOnly(m_fileName);
    if (path.empty())
        path = wxConfig::Get()->Read(_T("last_file_path"), wxEmptyString);

    wxString name = wxFileSelector(_("Open catalog"),
                    path, wxEmptyString, wxEmptyString,
                    _("GNU gettext catalogs (*.po)|*.po|All files (*.*)|*.*"),
                    wxFD_OPEN | wxFD_FILE_MUST_EXIST, this);

    if (!name.empty())
    {
        wxConfig::Get()->Write(_T("last_file_path"), wxPathOnly(name));

        DoOpenFile(name);
    }
}



void PoeditFrame::OnOpenHist(wxCommandEvent& event)
{
    wxString f(m_history.GetHistoryFile(event.GetId() - wxID_FILE1));
    if ( !wxFileExists(f) )
    {
        wxLogError(_("File '%s' doesn't exist."), f.c_str());
        return;
    }

    OpenFile(f);
}


void PoeditFrame::OnSave(wxCommandEvent& event)
{
    if (m_fileName.empty())
        OnSaveAs(event);
    else
        WriteCatalog(m_fileName);
}


static wxString SuggestFileName(const Catalog *catalog)
{
    wxString name;
    if (catalog)
        name = catalog->GetLocaleCode();

    if (name.empty())
        return _T("default");
    else
        return name;
}

wxString PoeditFrame::GetSaveAsFilename(Catalog *cat, const wxString& current)
{
    wxString name(wxFileNameFromPath(current));
    wxString path(wxPathOnly(current));

    if (name.empty())
    {
        path = wxConfig::Get()->Read(_T("last_file_path"), wxEmptyString);
        name = SuggestFileName(cat) + _T(".po");
    }

    name = wxFileSelector(_("Save as..."), path, name, wxEmptyString,
                          _("GNU gettext catalogs (*.po)|*.po|All files (*.*)|*.*"),
                          wxFD_SAVE | wxFD_OVERWRITE_PROMPT, this);
    if (!name.empty())
    {
        wxConfig::Get()->Write(_T("last_file_path"), wxPathOnly(name));
    }

    return name;
}

void PoeditFrame::DoSaveAs(const wxString& filename)
{
    if (filename.empty())
        return;

    m_fileName = filename;
    WriteCatalog(filename);
}

void PoeditFrame::OnSaveAs(wxCommandEvent&)
{
    DoSaveAs(GetSaveAsFilename(m_catalog, m_fileName));
}


void PoeditFrame::OnExport(wxCommandEvent&)
{
    wxString name(wxFileNameFromPath(m_fileName));

    if (name.empty())
    {
        name = SuggestFileName(m_catalog) + _T(".html");
    }
    else
        name += _T(".html");

    name = wxFileSelector(_("Export as..."),
                          wxPathOnly(m_fileName), name, wxEmptyString,
                          _("HTML file (*.html)|*.html"),
                          wxFD_SAVE | wxFD_OVERWRITE_PROMPT, this);
    if (!name.empty())
    {
        wxConfig::Get()->Write(_T("last_file_path"), wxPathOnly(name));
        ExportCatalog(name);
    }
}

bool PoeditFrame::ExportCatalog(const wxString& filename)
{
    wxBusyCursor bcur;
    bool ok = m_catalog->ExportToHTML(filename);
    return ok;
}



void PoeditFrame::OnNew(wxCommandEvent& event)
{
    if ( !CanDiscardCurrentDoc() )
        return;

    bool isFromPOT = event.GetId() == XRCID("menu_new_from_pot");

    PropertiesDialog dlg(this);
    Catalog *catalog = new Catalog;

    if (isFromPOT)
    {
        wxString path = wxPathOnly(m_fileName);
        if (path.empty())
            path = wxConfig::Get()->Read(_T("last_file_path"), wxEmptyString);
        wxString pot_file =
            wxFileSelector(_("Open catalog template"),
                 path, wxEmptyString, wxEmptyString,
                 _("GNU gettext templates (*.pot)|*.pot|All files (*.*)|*.*"),
                 wxFD_OPEN | wxFD_FILE_MUST_EXIST, this);
        bool ok = false;
        if (!pot_file.empty())
        {
            wxConfig::Get()->Write(_T("last_file_path"), wxPathOnly(pot_file));
            ok = catalog->UpdateFromPOT(pot_file,
                                        /*summary=*/false,
                                        /*replace_header=*/true);
        }
        if (!ok)
        {
            delete catalog;
            return;
        }
    }
    else
    {
        catalog->CreateNewHeader();
    }

    dlg.TransferTo(catalog);
    if (dlg.ShowModal() == wxID_OK)
    {
        wxString file = GetSaveAsFilename(catalog, wxEmptyString);
        if (file.empty())
        {
            delete catalog;
            return;
        }

        CancelItemsValidation();

        dlg.TransferFrom(catalog);
        delete m_catalog;
        m_catalog = catalog;
        m_list->CatalogChanged(m_catalog);
        m_modified = true;
        DoSaveAs(file);
        if (!isFromPOT)
        {
            OnUpdate(event);
        }

        RestartItemsValidation();
    }
    else
    {
        delete catalog;
    }
    UpdateTitle();
    UpdateStatusBar();

#ifdef USE_TRANSMEM
    if (m_transMem)
    {
        m_transMem->Release();
        m_transMem = NULL;
    }
    m_transMemLoaded = false;
#endif

    InitSpellchecker();
}



void PoeditFrame::OnProperties(wxCommandEvent&)
{
    EditCatalogProperties();
}

void PoeditFrame::EditCatalogProperties()
{
    PropertiesDialog dlg(this);

    dlg.TransferTo(m_catalog);
    if (dlg.ShowModal() == wxID_OK)
    {
        dlg.TransferFrom(m_catalog);
        m_modified = true;
        RecreatePluralTextCtrls();
        UpdateTitle();
        UpdateMenu();
        InitSpellchecker();
    }
}



void PoeditFrame::EditPreferences()
{
    PreferencesDialog dlg(this);

    dlg.TransferTo(wxConfig::Get());
    if (dlg.ShowModal() == wxID_OK)
    {
        dlg.TransferFrom(wxConfig::Get());
        g_focusToText = (bool)wxConfig::Get()->Read(_T("focus_to_text"),
                                                     (long)false);

        SetCustomFonts();
        m_list->Refresh(); // if font changed

        UpdateCommentWindowEditable();
        InitSpellchecker();
    }
}

void PoeditFrame::OnPreferences(wxCommandEvent&)
{
    EditPreferences();
}



void PoeditFrame::UpdateCatalog(const wxString& pot_file)
{
    wxBusyCursor bcur;

    CancelItemsValidation();

    // This ensures that the list control won't be redrawn during Update()
    // call when a dialog box is hidden; another alternative would be to call
    // m_list->CatalogChanged(NULL) here
    wxWindowUpdateLocker locker(m_list);

    ProgressInfo progress(this, _("Updating catalog"));

    bool succ;
    if (pot_file.empty())
        succ = m_catalog->Update(&progress);
    else
        succ = m_catalog->UpdateFromPOT(pot_file);
    m_list->CatalogChanged(m_catalog);

    RestartItemsValidation();

    m_modified = succ || m_modified;
    if (!succ)
    {
        wxLogWarning(_("Entries in the catalog are probably incorrect."));
        wxLogError(
           _("Updating the catalog failed. Click on 'Details >>' for details."));
    }
}

void PoeditFrame::OnUpdate(wxCommandEvent& event)
{
    wxString pot_file;

    if (event.GetId() == XRCID("menu_update_from_pot"))
    {
        wxString path = wxPathOnly(m_fileName);
        if (path.empty())
            path = wxConfig::Get()->Read(_T("last_file_path"), wxEmptyString);
        pot_file =
            wxFileSelector(_("Open catalog template"),
                 path, wxEmptyString, wxEmptyString,
                 _("GNU gettext templates (*.pot)|*.pot|All files (*.*)|*.*"),
                 wxFD_OPEN | wxFD_FILE_MUST_EXIST, this);
        if (pot_file.empty())
            return;
        wxConfig::Get()->Write(_T("last_file_path"), wxPathOnly(pot_file));
    }

    UpdateCatalog(pot_file);

#ifdef USE_TRANSMEM
    if (wxConfig::Get()->Read(_T("use_tm_when_updating"), true) &&
        GetTransMem() != NULL)
    {
        AutoTranslateCatalog();
    }
#endif

    RefreshControls();
}



void PoeditFrame::OnListSel(wxListEvent& event)
{
    wxWindow *focus = wxWindow::FindFocus();
    bool hasFocus = (focus == m_textTrans) ||
                    (focus && focus->GetParent() == m_pluralNotebook);

    event.Skip();

    UpdateToTextCtrl();

    if (hasFocus)
    {
        if (m_textTrans->IsShown())
            m_textTrans->SetFocus();
        else if (!m_textTransPlural.empty())
            m_textTransPlural[0]->SetFocus();
    }
}


void PoeditFrame::OnListActivated(wxListEvent& event)
{
    if (m_catalog)
    {
        int ind = m_list->ListIndexToCatalog(event.GetIndex());
        if (ind >= (int)m_catalog->GetCount()) return;
        CatalogItem& entry = (*m_catalog)[ind];
        if (entry.GetValidity() == CatalogItem::Val_Invalid)
        {
            wxMessageBox(entry.GetErrorString(),
                         _("Gettext syntax error"),
                         wxOK | wxICON_ERROR);
        }
    }
}



void PoeditFrame::OnReferencesMenu(wxCommandEvent&)
{
    CatalogItem *entry = GetCurrentItem();
    if ( !entry )
        return;

    const wxArrayString& refs = entry->GetReferences();

    if (refs.GetCount() == 0)
        wxMessageBox(_("No references to this string found."));
    else if (refs.GetCount() == 1)
        ShowReference(0);
    else
    {
        wxString *table = new wxString[refs.GetCount()];
        for (unsigned i = 0; i < refs.GetCount(); i++)
            table[i] = refs[i];
        int result = wxGetSingleChoiceIndex(_("Please choose the reference you want to show:"), _("References"),
                          refs.GetCount(), table);
        delete[] table;
        if (result != -1)
            ShowReference(result);
    }
}


void PoeditFrame::OnReference(wxCommandEvent& event)
{
    ShowReference(event.GetId() - ID_POPUP_REFS);
}



void PoeditFrame::ShowReference(int num)
{
    CatalogItem *entry = GetCurrentItem();
    wxCHECK_RET( entry, _T("no entry selected") );

    wxBusyCursor bcur;

    wxString basepath;
    wxString cwd = wxGetCwd();

    if (!!m_fileName)
    {
        wxString path;

        if (wxIsAbsolutePath(m_catalog->Header().BasePath))
            path = m_catalog->Header().BasePath;
        else
            path = wxPathOnly(m_fileName) + _T("/") + m_catalog->Header().BasePath;

        if (path.Last() == _T('/') || path.Last() == _T('\\'))
            path.RemoveLast();

        if (wxIsAbsolutePath(path))
            basepath = path;
        else
            basepath = cwd + _T("/") + path;
    }

    FileViewer *w = new FileViewer(this, basepath,
                                   entry->GetReferences(),
                                   num);
    if (w->FileOk())
        w->Show(true);
    else
        w->Close();
}



void PoeditFrame::OnFuzzyFlag(wxCommandEvent& event)
{
    if (event.GetEventObject() == GetToolBar())
    {
        GetMenuBar()->Check(XRCID("menu_fuzzy"),
                            GetToolBar()->GetToolState(XRCID("menu_fuzzy")));
    }
    else
    {
        GetToolBar()->ToggleTool(XRCID("menu_fuzzy"),
                                 GetMenuBar()->IsChecked(XRCID("menu_fuzzy")));
    }

    // The user explicitly changed fuzzy status (e.g. to on). Normally, if the
    // user edits an entry, it's fuzzy flag is cleared, but if the user sets
    // fuzzy on to indicate the translation is problematic and then continues
    // editing the entry, we do not want to annoy him by changing fuzzy back on
    // every keystroke.
    m_dontAutoclearFuzzyStatus = true;

    UpdateFromTextCtrl();
}



void PoeditFrame::OnQuotesFlag(wxCommandEvent&)
{
    UpdateFromTextCtrl();
    m_displayQuotes = GetMenuBar()->IsChecked(XRCID("menu_quotes"));
    UpdateToTextCtrl();
}



void PoeditFrame::OnLinesFlag(wxCommandEvent&)
{
    m_displayLines = GetMenuBar()->IsChecked(XRCID("menu_lines"));
    m_list->SetDisplayLines(m_displayLines);
}



void PoeditFrame::OnCommentWinFlag(wxCommandEvent&)
{
    UpdateDisplayCommentWin();
}

void PoeditFrame::OnAutoCommentsWinFlag(wxCommandEvent&)
{
    UpdateDisplayCommentWin();
}


void PoeditFrame::OnCopyFromSource(wxCommandEvent&)
{
    if (!m_textTrans->IsShown())
    {
        // plural form entry:
        wxString orig = m_textOrigPlural->GetValue();
        for (size_t i = 0; i < m_textTransPlural.size(); i++)
            m_textTransPlural[i]->SetValue(orig);

        if (m_textTransSingularForm)
            m_textTransSingularForm->SetValue(m_textOrig->GetValue());
    }
    else
    {
        // singular form entry:
        m_textTrans->SetValue(m_textOrig->GetValue());
    }
}

void PoeditFrame::OnClearTranslation(wxCommandEvent&)
{
    if (!m_textTrans->IsShown())
    {
        // plural form entry:
        for (size_t i=0; i < m_textTransPlural.size(); i++)
            m_textTransPlural[i]->Clear();

        if (m_textTransSingularForm)
            m_textTransSingularForm->Clear();
    }
    else
    {
        // singular form entry:
        m_textTrans->Clear();
    }
}


void PoeditFrame::OnFind(wxCommandEvent&)
{
    FindFrame *f = (FindFrame*)FindWindow(_T("find_frame"));

    if (!f)
        f = new FindFrame(this, m_list, m_catalog, m_textOrig, m_textTrans, m_textComment, m_textAutoComments);
    f->Show(true);
}


CatalogItem *PoeditFrame::GetCurrentItem() const
{
    if ( !m_catalog || !m_list )
        return NULL;

    int item = m_list->GetSelectedCatalogItem();
    if ( item == -1 )
        return NULL;

    wxASSERT( item >= 0 && item < (int)m_catalog->GetCount() );

    return &(*m_catalog)[item];
}


static wxString TransformNewval(const wxString& val, bool displayQuotes)
{
    wxString newval(val);

    newval.Replace(_T("\n"), _T(""));
    if (displayQuotes)
    {
        if (newval.Len() > 0 && newval[0u] == _T('"'))
            newval.Remove(0, 1);
        if (newval.Len() > 0 && newval[newval.Length()-1] == _T('"'))
            newval.RemoveLast();
    }

    if (newval[0u] == _T('"')) newval.Prepend(_T("\\"));
    for (unsigned i = 1; i < newval.Len(); i++)
        if (newval[i] == _T('"') && newval[i-1] != _T('\\'))
        {
            newval = newval.Mid(0, i) + _T("\\\"") + newval.Mid(i+1);
            i++;
        }

    // string ending with [^\]\ is invalid:
    if (newval.length() > 1 &&
        newval[newval.length()-1] == _T('\\') &&
        newval[newval.length()-2] != _T('\\'))
    {
        newval.RemoveLast();
    }

    return newval;
}

void PoeditFrame::UpdateFromTextCtrl()
{
    CatalogItem *entry = GetCurrentItem();
    if ( !entry )
        return;

    wxString key = entry->GetString();
    bool newfuzzy = GetToolBar()->GetToolState(XRCID("menu_fuzzy"));

    const bool oldIsTranslated = entry->IsTranslated();
    bool allTranslated = true; // will be updated later
    bool anyTransChanged = false; // ditto

    if (entry->HasPlural())
    {
        wxArrayString str;
        for (unsigned i = 0; i < m_textTransPlural.size(); i++)
        {
            wxString val = TransformNewval(m_textTransPlural[i]->GetValue(),
                                           m_displayQuotes);
            str.Add(val);
            if ( val.empty() )
                allTranslated = false;
        }

        if ( str != entry->GetTranslations() )
        {
            anyTransChanged = true;
            entry->SetTranslations(str);
        }
    }
    else
    {
        wxString newval =
            TransformNewval(m_textTrans->GetValue(), m_displayQuotes);

        if ( newval.empty() )
            allTranslated = false;

        if ( newval != entry->GetTranslation() )
        {
            anyTransChanged = true;
            entry->SetTranslation(newval);
        }
    }

    if (entry->IsFuzzy() == newfuzzy && !anyTransChanged)
    {
        return; // not even fuzzy status changed, so return
    }

    // did something affecting statistics change?
    bool statisticsChanged = false;

    if (newfuzzy == entry->IsFuzzy() && !m_dontAutoclearFuzzyStatus)
        newfuzzy = false;


    GetToolBar()->ToggleTool(XRCID("menu_fuzzy"), newfuzzy);
    GetMenuBar()->Check(XRCID("menu_fuzzy"), newfuzzy);

    if ( entry->IsFuzzy() != newfuzzy )
    {
        entry->SetFuzzy(newfuzzy);
        statisticsChanged = true;
    }
    if ( oldIsTranslated != allTranslated )
    {
        entry->SetTranslated(allTranslated);
        statisticsChanged = true;
    }
    entry->SetModified(true);
    entry->SetAutomatic(false);

    RefreshSelectedItem();

    if ( statisticsChanged )
    {
        UpdateStatusBar();
    }
    // else: no point in recomputing stats

    if ( !IsModified() )
    {
        m_modified = true;
        UpdateTitle();
    }

#ifdef USE_GETTEXT_VALIDATION
    // check validity of this item:
    m_itemsToValidate.push_front(item);
#endif
}

namespace
{

struct EventHandlerDisabler
{
    EventHandlerDisabler(wxEvtHandler *h) : m_hnd(h)
        { m_hnd->SetEvtHandlerEnabled(false); }
    ~EventHandlerDisabler()
        { m_hnd->SetEvtHandlerEnabled(true); }

    wxEvtHandler *m_hnd;
};

void SetTranslationValue(wxTextCtrl *txt, const wxString& value)
{
    // disable EVT_TEXT forwarding -- the event is generated by
    // programmatic changes to text controls' content and we *don't*
    // want UpdateFromTextCtrl() to be called from here
    EventHandlerDisabler disabler(txt->GetEventHandler());
    txt->SetValue(value);
}

} // anonymous namespace

void PoeditFrame::UpdateToTextCtrl()
{
    CatalogItem *entry = GetCurrentItem();
    if ( !entry )
        return;

    wxString quote;
    wxString t_o, t_t, t_c, t_ac;
    if (m_displayQuotes)
        quote = _T("\"");
    t_o = quote + entry->GetString() + quote;
    t_o.Replace(_T("\\n"), _T("\\n\n"));
    t_c = entry->GetComment();
    t_c.Replace(_T("\\n"), _T("\\n\n"));

    for (unsigned i=0; i < entry->GetAutoComments().GetCount(); i++)
      t_ac += entry->GetAutoComments()[i] + _T("\n");
    t_ac.Replace(_T("\\n"), _T("\\n\n"));

    // remove "# " in front of every comment line
    t_c = CommentDialog::RemoveStartHash(t_c);

    m_textOrig->SetValue(t_o);

    if (entry->HasPlural())
    {
        wxString t_op = quote + entry->GetPluralString() + quote;
        t_op.Replace(_T("\\n"), _T("\\n\n"));
        m_textOrigPlural->SetValue(t_op);

        unsigned formsCnt = m_textTransPlural.size();
        for (unsigned j = 0; j < formsCnt; j++)
            SetTranslationValue(m_textTransPlural[j], wxEmptyString);

        unsigned i = 0;
        for (i = 0; i < std::min(formsCnt, entry->GetNumberOfTranslations()); i++)
        {
            t_t = quote + entry->GetTranslation(i) + quote;
            t_t.Replace(_T("\\n"), _T("\\n\n"));
            SetTranslationValue(m_textTransPlural[i], t_t);
            if (m_displayQuotes)
                m_textTransPlural[i]->SetInsertionPoint(1);
        }
    }
    else
    {
        t_t = quote + entry->GetTranslation() + quote;
        t_t.Replace(_T("\\n"), _T("\\n\n"));
        SetTranslationValue(m_textTrans, t_t);
        if (m_displayQuotes)
            m_textTrans->SetInsertionPoint(1);
    }

    if (m_displayCommentWin)
        m_textComment->SetValue(t_c);

    if (m_displayAutoCommentsWin)
        m_textAutoComments->SetValue(t_ac);

    // by default, editing fuzzy item unfuzzies it
    m_dontAutoclearFuzzyStatus = false;

    GetToolBar()->ToggleTool(XRCID("menu_fuzzy"), entry->IsFuzzy());
    GetMenuBar()->Check(XRCID("menu_fuzzy"), entry->IsFuzzy());

    ShowPluralFormUI(entry->HasPlural());
}



void PoeditFrame::ReadCatalog(const wxString& catalog)
{
    wxBusyCursor bcur;

    CancelItemsValidation();

    Catalog *cat = new Catalog(catalog);
    if (!cat->IsOk())
        return;

    delete m_catalog;
    m_catalog = cat;

    // this must be done as soon as possible, otherwise the list would be
    // confused
    m_list->CatalogChanged(m_catalog);

#ifdef USE_TRANSMEM
    if (m_transMem)
    {
        m_transMem->Release();
        m_transMem = NULL;
    }
    m_transMemLoaded = false;
#endif

    m_fileName = catalog;
    m_modified = false;

    RecreatePluralTextCtrls();
    RefreshControls();
    UpdateTitle();

    wxFileName fn(m_fileName);
    fn.Normalize(wxPATH_NORM_DOTS | wxPATH_NORM_ABSOLUTE);
    m_history.AddFileToHistory(fn.GetFullPath());

    InitSpellchecker();

    RestartItemsValidation();


    // FIXME: do this for Gettext PO files only
    if (wxConfig::Get()->Read(_T("translator_name"), _T("nothing")) == _T("nothing"))
    {
        AttentionMessage msg
            (
                _T("no-translator-info"),
                AttentionMessage::Info,
                _("You should set your email address in Preferences so that it can be used for Last-Translator header in GNU gettext files.")
            );
        msg.AddAction(_("Set email"),
                      boost::bind(&PoeditFrame::EditPreferences, this));
        msg.AddDontShowAgain();

        m_attentionBar->ShowMessage(msg);
    }

    // check if plural forms header is correct:
    if ( m_catalog->HasPluralItems() )
    {
        wxString err;

        if ( m_catalog->Header().GetHeader(_T("Plural-Forms")).empty() )
        {
            err = _("This catalog has entries with plural forms, but doesn't have Plural-Forms header configured.");
        }
        else if ( m_catalog->HasWrongPluralFormsCount() )
        {
            err = _("Entries in this catalog have different plural forms count from what catalog's Plural-Forms header says");
        }

        // FIXME: make this part of global error checking
        wxString plForms = m_catalog->Header().GetHeader(_T("Plural-Forms"));
        PluralFormsCalculator *plCalc =
                PluralFormsCalculator::make(plForms.ToAscii());
        if ( !plCalc )
        {
            if ( plForms.empty() )
            {
                err = _("Required header Plural-Forms is missing.");
            }
            else
            {
                err = wxString::Format(
                            _("Syntax error in Plural-Forms header (\"%s\")."),
                            plForms.c_str());
            }
        }
        delete plCalc;

        if ( !err.empty() )
        {
            AttentionMessage msg
                (
                    _T("malformed-plural-forms"),
                    AttentionMessage::Error,
                    err
                );
            msg.AddAction(_("Fix the header"),
                          boost::bind(&PoeditFrame::EditCatalogProperties, this));

            m_attentionBar->ShowMessage(msg);
        }
    }
}


void PoeditFrame::RefreshControls()
{
    m_itemsRefreshQueue.clear();

    if (!m_catalog)
        return;

    m_hasObsoleteItems = false;
    if (!m_catalog->IsOk())
    {
        wxLogError(_("Error loading message catalog file '%s'."), m_fileName.c_str());
        m_fileName = wxEmptyString;
        UpdateMenu();
        UpdateTitle();
        delete m_catalog;
        m_catalog = NULL;
        m_list->CatalogChanged(NULL);
        return;
    }

    wxBusyCursor bcur;
    UpdateMenu();

    // remember currently selected item:
    int selectedItem = m_list->GetSelectedCatalogItem();

    // update catalog view, this may involve reordering the items...
    m_list->CatalogChanged(m_catalog);

    // ...and so we need to restore selection now:
    if ( selectedItem != -1 )
        m_list->SelectCatalogItem(selectedItem);

    FindFrame *f = (FindFrame*)FindWindow(_T("find_frame"));
    if (f)
        f->Reset(m_catalog);

    UpdateTitle();
    UpdateStatusBar();
    Refresh();
}



void PoeditFrame::UpdateStatusBar()
{
    int all, fuzzy, untranslated, badtokens, unfinished;
    if (m_catalog)
    {
        wxString txt;

        m_catalog->GetStatistics(&all, &fuzzy, &badtokens, &untranslated, &unfinished);

        int percent = (all == 0 ) ? 0 : (100 * (all - unfinished) / all);

        wxString details;
        if ( fuzzy > 0 )
        {
            details += wxString::Format(wxPLURAL("%i fuzzy", "%i fuzzy", fuzzy), fuzzy);
        }
        if ( badtokens > 0 )
        {
            if ( !details.empty() )
                details += _T(", ");
            details += wxString::Format(wxPLURAL("%i bad token", "%i bad tokens", badtokens), badtokens);
        }
        if ( untranslated > 0 )
        {
            if ( !details.empty() )
                details += _T(", ");
            details += wxString::Format(wxPLURAL("%i not translated", "%i not translated", untranslated), untranslated);
        }
        if ( details.empty() )
        {
            txt.Printf(wxPLURAL("%i %% translated, %i string", "%i %% translated, %i strings", all),
                       percent, all);
        }
        else
        {
            txt.Printf(wxPLURAL("%i %% translated, %i string (%s)", "%i %% translated, %i strings (%s)", all),
                       percent, all, details.c_str());
        }

#ifdef USE_GETTEXT_VALIDATION
        if (!m_itemsToValidate.empty())
        {
            wxString progress;
            progress.Printf(wxPLURAL("[checking translations: %i left]", "[checking translations: %i left]",
                                     m_itemsToValidate.size()),
                            m_itemsToValidate.size());
            txt += _T("    ");
            txt += progress;
        }
#endif

        GetStatusBar()->SetStatusText(txt);
    }
}

void PoeditFrame::DoGiveHelp(const wxString& text, bool show)
{
    if (show || !text.empty())
        wxFrame::DoGiveHelp(text, show);
    else
        UpdateStatusBar();
}


void PoeditFrame::UpdateTitle()
{
    OSXSetModified(IsModified());

    wxString title;
    if ( !m_fileName.empty() )
    {
#if wxCHECK_VERSION(2,9,4)
        SetRepresentedFilename(m_fileName);
#endif
        wxFileName fn(m_fileName);
        title = wxString::Format(_T("%s.%s"), fn.GetName().c_str(), fn.GetExt().c_str());
#ifndef __WXOSX__
        if ( IsModified() )
            title += _(" (modified)");
        title += " - Poedit";
#endif
    }
    else
    {
        title = _T("Poedit");
    }

    SetTitle(title);
}



void PoeditFrame::UpdateMenu()
{
    wxMenuBar *menubar = GetMenuBar();
    wxToolBar *toolbar = GetToolBar();

    const bool editable = (m_catalog != NULL);

    menubar->Enable(wxID_SAVE, editable);
    menubar->Enable(wxID_SAVEAS, editable);
    menubar->Enable(XRCID("menu_export"), editable);
    toolbar->EnableTool(wxID_SAVE, editable);
    toolbar->EnableTool(XRCID("menu_update"), editable);
    toolbar->EnableTool(XRCID("menu_fuzzy"), editable);
    toolbar->EnableTool(XRCID("menu_comment"), editable);

    menubar->Enable(XRCID("menu_update"), editable);
    menubar->Enable(XRCID("menu_fuzzy"), editable);
    menubar->Enable(XRCID("menu_comment"), editable);
    menubar->Enable(XRCID("menu_copy_from_src"), editable);
    menubar->Enable(XRCID("menu_clear"), editable);
    menubar->Enable(XRCID("menu_references"), editable);
    menubar->Enable(wxID_FIND, editable);

    menubar->EnableTop(2, editable);

    m_textTrans->Enable(editable);
    m_textOrig->Enable(editable);
    m_textOrigPlural->Enable(editable);
    m_textComment->Enable(editable);
    m_textAutoComments->Enable(editable);
    m_list->Enable(editable);

    menubar->Enable(XRCID("menu_purge_deleted"),
                    editable && m_catalog->HasDeletedItems());

    const bool doupdate = editable &&
                          m_catalog->Header().SearchPaths.GetCount() > 0;
    toolbar->EnableTool(XRCID("menu_update"), doupdate);
    menubar->Enable(XRCID("menu_update"), doupdate);

#ifdef __WXGTK__
    if (!editable)
    {
        // work around a wxGTK bug: enabling wxTextCtrl makes it editable too
        // in wxGTK <= 2.8:
        m_textOrig->SetEditable(false);
        m_textOrigPlural->SetEditable(false);
    }
#endif

    menubar->EnableTop(3, editable);
    for (int i = 0; i < 10; i++)
    {
        menubar->Enable(ID_BOOKMARK_SET + i, editable);
        menubar->Enable(ID_BOOKMARK_GO + i,
                        editable &&
                        m_catalog->GetBookmarkIndex(Bookmark(i)) != -1);
    }
}



bool PoeditFrame::WriteCatalog(const wxString& catalog)
{
    wxBusyCursor bcur;

    Catalog::HeaderData& dt = m_catalog->Header();
    dt.Translator = wxConfig::Get()->Read(_T("translator_name"), dt.Translator);
    dt.TranslatorEmail = wxConfig::Get()->Read(_T("translator_email"), dt.TranslatorEmail);

    if ( !m_catalog->Save(catalog) )
        return false;

    m_fileName = catalog;
    m_modified = false;

#ifdef USE_TRANSMEM
    if (GetTransMem())
    {
        TranslationMemory *tm = GetTransMem();
        size_t cnt = m_catalog->GetCount();
        for (size_t i = 0; i < cnt; i++)
        {
            const CatalogItem& dt = (*m_catalog)[i];

            // ignore translations with errors in them
            if (dt.GetValidity() == CatalogItem::Val_Invalid)
                continue;

            // ignore untranslated or unfinished translations
            if (dt.IsFuzzy() || dt.GetTranslation().empty())
                continue;

            // Note that dt.IsModified() is intentionally not checked - we
            // want to save old entries in the TM too, so that we harvest as
            // much useful translations as we can.

            if ( !tm->Store(dt.GetString(), dt.GetTranslation()) )
            {
                // If the TM failed, it would almost certainly fail to store
                // the next item as well. Don't bother, just stop updating
                // it here, so that the user is given only one error message
                // instead of an insane amount of them.
                break;
            }
        }
    }
#endif

    m_history.AddFileToHistory(m_fileName);
    UpdateTitle();

    RefreshControls();

    if (ManagerFrame::Get())
        ManagerFrame::Get()->NotifyFileChanged(m_fileName);

    return true;
}


void PoeditFrame::OnEditComment(wxCommandEvent&)
{
    CatalogItem *entry = GetCurrentItem();
    wxCHECK_RET( entry, _T("no entry selected") );

    wxString comment = entry->GetComment();
    CommentDialog dlg(this, comment);
    if (dlg.ShowModal() == wxID_OK)
    {
        m_modified = true;
        UpdateTitle();
        comment = dlg.GetComment();
        entry->SetComment(comment);

        RefreshSelectedItem();

        // update comment window
        m_textComment->SetValue(CommentDialog::RemoveStartHash(comment));
    }
}

void PoeditFrame::OnPurgeDeleted(wxCommandEvent& WXUNUSED(event))
{
    const wxString title =
        _("Purge deleted translations");
    const wxString main =
        _("Do you want to remove all translations that are no longer used?");
    const wxString details =
        _("If you continue with purging, all translations marked as deleted will be permanently removed. You will have to translate them again if they are added back in the future.");

#if wxCHECK_VERSION(2,9,0)
    wxMessageDialog dlg(this, main, title, wxYES_NO | wxICON_QUESTION);
    dlg.SetExtendedMessage(details);
    dlg.SetYesNoLabels(_("Purge"), _("Keep"));
#else // wx < 2.9.0
    const wxString all = main + _T("\n\n") + details;
    wxMessageDialog dlg(this, all, title, wxYES_NO | wxICON_QUESTION);
#endif

    if (dlg.ShowModal() == wxID_YES)
    {
        m_catalog->RemoveDeletedItems();
        UpdateMenu();
    }
}


#ifdef USE_TRANSMEM
void PoeditFrame::OnAutoTranslate(wxCommandEvent& event)
{
    CatalogItem *entry = GetCurrentItem();
    wxCHECK_RET( entry, _T("no entry selected") );

    int ind = event.GetId() - ID_POPUP_TRANS;

    entry->SetTranslation(m_autoTranslations[ind]);
    entry->SetFuzzy(false);
    entry->SetModified(true);

    // FIXME: instead of this mess, use notifications of catalog change
    m_modified = true;
    UpdateTitle();

    UpdateToTextCtrl();
    RefreshSelectedItem();
}

void PoeditFrame::OnAutoTranslateAll(wxCommandEvent&)
{
    AutoTranslateCatalog();
}

bool PoeditFrame::AutoTranslateCatalog()
{
    wxBusyCursor bcur;

    TranslationMemory *tm = GetTransMem();

    if (tm == NULL)
        return false;

    size_t cnt = m_catalog->GetCount();
    size_t matches = 0;
    wxString msg;

    ProgressInfo pi(this, _("Automatically translating..."));
    pi.SetGaugeMax(cnt);
    for (size_t i = 0; i < cnt; i++)
    {
        CatalogItem& dt = (*m_catalog)[i];
        if (dt.IsFuzzy() || !dt.IsTranslated())
        {
            wxArrayString results;
            int score = tm->Lookup(dt.GetString(), results);
            if (score > 0)
            {
                dt.SetTranslation(results[0]);
                dt.SetAutomatic(true);
                dt.SetFuzzy(true);
                matches++;
                msg.Printf(_("Automatically translated %u strings"), matches);
                pi.UpdateMessage(msg);

                if (m_modified == false)
                {
                    m_modified = true;
                    UpdateTitle();
                }
            }
        }
        pi.UpdateGauge();
    }

    RefreshControls();

    return true;
}
#endif

wxMenu *PoeditFrame::GetPopupMenu(size_t item)
{
    if (!m_catalog) return NULL;
    if (item >= (size_t)m_list->GetItemCount()) return NULL;

    const wxArrayString& refs = (*m_catalog)[item].GetReferences();
    wxMenu *menu = new wxMenu;

    menu->Append(XRCID("menu_copy_from_src"),
                 #ifdef __WXMSW__
                 wxString(_("Copy from source text"))
                 #else
                 wxString(_("Copy from Source Text"))
                 #endif
                   + _T("\tCtrl+B"));
    menu->Append(XRCID("menu_clear"),
                 #ifdef __WXMSW__
                 wxString(_("Clear translation"))
                 #else
                 wxString(_("Clear Translation"))
                 #endif
                   + _T("\tCtrl+K"));
   menu->Append(XRCID("menu_comment"),
                 #ifdef __WXMSW__
                 wxString(_("Edit comment"))
                 #else
                 wxString(_("Edit Comment"))
                 #endif
                   + _T("\tCtrl+M"));

#ifdef USE_TRANSMEM
    if (GetTransMem())
    {
        wxBusyCursor bcur;
        CatalogItem& dt = (*m_catalog)[item];
        m_autoTranslations.Clear();
        if (GetTransMem()->Lookup(dt.GetString(), m_autoTranslations) > 0)
        {
            menu->AppendSeparator();
#ifdef CAN_MODIFY_DEFAULT_FONT
            wxMenuItem *it2 = new wxMenuItem
                                  (
                                      menu,
                                      ID_POPUP_DUMMY+1,
                                      #ifdef __WXMSW__
                                      _("Automatic translations:")
                                      #else
                                      _("Automatic Translations:")
                                      #endif
                                  );
            it2->SetFont(m_boldGuiFont);
            menu->Append(it2);
#else
            menu->Append
                  (
                      ID_POPUP_DUMMY+1,
                      #ifdef __WXMSW__
                      _("Automatic translations:")
                      #else
                      _("Automatic Translations:")
                      #endif
                  );
#endif

            for (size_t i = 0; i < m_autoTranslations.GetCount(); i++)
            {
                // Convert from UTF-8 to environment's default charset:
                wxString s(m_autoTranslations[i].wc_str(wxConvUTF8), wxConvLocal);
                if (!s)
                    s = m_autoTranslations[i];
                menu->Append(ID_POPUP_TRANS + i, _T("    ") + s);
            }
        }
    }
#endif

    if ( !refs.empty() )
    {
        menu->AppendSeparator();

#ifdef CAN_MODIFY_DEFAULT_FONT
        wxMenuItem *it1 = new wxMenuItem(menu, ID_POPUP_DUMMY+0, _("References:"));
        it1->SetFont(m_boldGuiFont);
        menu->Append(it1);
#else
        menu->Append(ID_POPUP_DUMMY+0, _("References:"));
#endif

        for (size_t i = 0; i < refs.GetCount(); i++)
            menu->Append(ID_POPUP_REFS + i, _T("    ") + refs[i]);
    }

    return menu;
}


void PoeditFrame::OnAbout(wxCommandEvent&)
{
#if 0
    // Forces translation of several strings that are used for about
    // dialog internally by wx, but are frequently not translate due to
    // state of wx's translations:

    // TRANSLATORS: This is titlebar of about dialog, "%s" is application name
    //              ("Poedit" here, but please use "%s")
    _("About %s");
    // TRANSLATORS: This is version information in about dialog, "%s" will be
    //              version number when used
    _("Version %s");
    // TRANSLATORS: This is version information in about dialog, it is followed
    //              by version number when used (wxWidgets 2.8)
    _(" Version ");
    // TRANSLATORS: This is titlebar of about dialog, the string ends with space
    //              and is followed by application name when used ("Poedit",
    //              but don't add it to this translation yourself) (wxWidgets 2.8)
    _("About ");
#endif

    wxAboutDialogInfo about;

    about.SetName(_T("Poedit"));
    about.SetVersion(wxGetApp().GetAppVersion());
#ifndef __WXMAC__
    about.SetDescription(_("Poedit is an easy to use translations editor."));
#endif
    about.SetCopyright(_T("Copyright \u00a9 1999-2012 Vaclav Slavik"));
#ifdef __WXGTK__ // other ports would show non-native about dlg
    about.SetWebSite(_T("http://www.poedit.net"));
#endif

    wxAboutBox(about);
}


void PoeditFrame::OnHelp(wxCommandEvent&)
{
    wxLaunchDefaultBrowser(_T("http://www.poedit.net/trac/wiki/Doc"));
}


void PoeditFrame::OnManager(wxCommandEvent&)
{
    wxFrame *f = ManagerFrame::Create();
    f->Raise();
}

void PoeditFrame::SetCustomFonts()
{
    wxConfigBase *cfg = wxConfig::Get();

    static bool prevUseFontText = false;

    bool useFontList = (bool)cfg->Read(_T("custom_font_list_use"), (long)false);
    bool useFontText = (bool)cfg->Read(_T("custom_font_text_use"), (long)false);

    if (useFontList)
    {
        wxString name = cfg->Read(_T("custom_font_list_name"), wxEmptyString);
        if (!name.empty())
        {
            wxNativeFontInfo fi;
            fi.FromString(name);
            wxFont font;
            font.SetNativeFontInfo(fi);
            m_list->SetCustomFont(font);
        }
    }
    else
    {
        m_list->SetCustomFont(wxNullFont);
    }

    if (useFontText)
    {
        wxString name = cfg->Read(_T("custom_font_text_name"), wxEmptyString);
        if (!name.empty())
        {
            wxNativeFontInfo fi;
            fi.FromString(name);
            wxFont font;
            font.SetNativeFontInfo(fi);
            m_textComment->SetFont(font);
            m_textAutoComments->SetFont(font);
            m_textOrig->SetFont(font);
            m_textOrigPlural->SetFont(font);
            m_textTrans->SetFont(font);
            for (size_t i = 0; i < m_textTransPlural.size(); i++)
                m_textTransPlural[i]->SetFont(font);
            prevUseFontText = true;
        }
    }
    else if (prevUseFontText)
    {
        wxFont font(wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT));
        m_textComment->SetFont(font);
        m_textAutoComments->SetFont(font);
        m_textOrig->SetFont(font);
        m_textOrigPlural->SetFont(font);
        m_textTrans->SetFont(font);
        for (size_t i = 0; i < m_textTransPlural.size(); i++)
            m_textTransPlural[i]->SetFont(font);
        prevUseFontText = false;
    }
}

void PoeditFrame::UpdateCommentWindowEditable()
{
    wxConfigBase *cfg = wxConfig::Get();
    bool commentWindowEditable =
        (bool)cfg->Read(_T("comment_window_editable"), (long)false);
    if (m_textComment == NULL ||
        commentWindowEditable != m_commentWindowEditable)
    {
        m_commentWindowEditable = commentWindowEditable;
        m_bottomSplitter->Unsplit();
        delete m_textComment;
        if (m_commentWindowEditable)
        {
            m_textComment = new wxTextCtrl(m_bottomRightPanel,
                                        ID_TEXTCOMMENT, wxEmptyString,
                                        wxDefaultPosition, wxDefaultSize,
                                        wxTE_MULTILINE | wxTE_RICH2);
        }
        else
        {
            m_textComment = new UnfocusableTextCtrl(m_bottomRightPanel,
                                        ID_TEXTCOMMENT, wxEmptyString,
                                        wxDefaultPosition, wxDefaultSize,
                                        wxTE_MULTILINE | wxTE_RICH2 | wxTE_READONLY);
        }
        UpdateDisplayCommentWin();
    }
}

void PoeditFrame::UpdateDisplayCommentWin()
{
    m_displayCommentWin =
        GetMenuBar()->IsChecked(XRCID("menu_comment_win"));
    m_displayAutoCommentsWin =
        GetMenuBar()->IsChecked(XRCID("menu_auto_comments_win"));

    if (m_displayCommentWin || m_displayAutoCommentsWin)
    {
        m_bottomSplitter->SplitVertically(
                m_bottomLeftPanel, m_bottomRightPanel,
                wxConfig::Get()->Read(_T("bottom_splitter"), -200L));
        m_bottomRightPanel->Show(true);

        // force recalculation of layout of panel so that text boxes take up
        // all the space they can
        // (sizer may be NULL on first call)
        if (m_bottomRightPanel->GetSizer() != NULL)
        {
            m_bottomRightPanel->GetSizer()->Show(m_labelComment,
                                                 m_displayCommentWin);
            m_bottomRightPanel->GetSizer()->Show(m_textComment,
                                                 m_displayCommentWin);

            m_bottomRightPanel->GetSizer()->Show(m_labelAutoComments,
                                                 m_displayAutoCommentsWin);
            m_bottomRightPanel->GetSizer()->Show(m_textAutoComments,
                                                 m_displayAutoCommentsWin);

            m_bottomRightPanel->GetSizer()->Layout();
            m_bottomRightPanel->Layout();
        }
    }
    else
    {
        if ( m_bottomSplitter->IsSplit() )
        {
            wxConfig::Get()->Write(_T("bottom_splitter"),
                                   (long)m_bottomSplitter->GetSashPosition());
        }
        m_bottomRightPanel->Show(false);
        m_bottomSplitter->Unsplit();
    }
    m_list->SetDisplayLines(m_displayLines);
    RefreshControls();
}

void PoeditFrame::OnCommentWindowText(wxCommandEvent&)
{
    if (!m_commentWindowEditable)
        return;

    CatalogItem *entry = GetCurrentItem();
    wxCHECK_RET( entry, _T("no entry selected") );

    wxString comment;
    comment = CommentDialog::AddStartHash(m_textComment->GetValue());

    if (comment == entry->GetComment())
        return;

    entry->SetComment(comment);
    RefreshSelectedItem();

    if (m_modified == false)
    {
        m_modified = true;
        UpdateTitle();
    }
}


void PoeditFrame::RefreshSelectedItem()
{
    m_itemsRefreshQueue.insert(m_list->GetSelection());
}

void PoeditFrame::OnIdle(wxIdleEvent& event)
{
    event.Skip();

#ifdef USE_GETTEXT_VALIDATION
    if (!m_itemsToValidate.empty() && m_itemBeingValidated == -1)
        BeginItemValidation();
#endif

    for ( std::set<int>::const_iterator i = m_itemsRefreshQueue.begin();
          i != m_itemsRefreshQueue.end(); ++i )
    {
        m_list->RefreshItem(*i);
    }
    m_itemsRefreshQueue.clear();
}

void PoeditFrame::OnSize(wxSizeEvent& event)
{
    event.Skip();

    // see the comment in PoeditFrame ctor
    if ( m_setSashPositionsWhenMaximized && IsMaximized() )
    {
        m_setSashPositionsWhenMaximized = false;

        // update sizes of child windows first:
        Layout();

        // then set sash positions
        m_splitter->SetSashPosition(wxConfig::Get()->Read(_T("splitter"), 240L));
        if ( m_bottomSplitter->IsSplit() )
        {
            m_bottomSplitter->SetSashPosition(
                wxConfig::Get()->Read(_T("bottom_splitter"), -200L));
        }
    }
}

void PoeditFrame::OnEndProcess(wxProcessEvent&)
{
#ifdef USE_GETTEXT_VALIDATION
    m_gettextProcess = NULL;
    event.Skip(); // deletes wxProcess object
    EndItemValidation();
    wxWakeUpIdle();
#endif
}

void PoeditFrame::CancelItemsValidation()
{
#ifdef USE_GETTEXT_VALIDATION
    // stop checking entries in the background:
    m_itemsToValidate.clear();
    m_itemBeingValidated = -1;
#endif
}

void PoeditFrame::RestartItemsValidation()
{
#ifdef USE_GETTEXT_VALIDATION
    // start checking catalog's entries in the background:
    int cnt = m_list->GetItemCount();
    for (int i = 0; i < cnt; i++)
        m_itemsToValidate.push_back(i);
#endif
}

void PoeditFrame::BeginItemValidation()
{
#ifdef USE_GETTEXT_VALIDATION
    int item = m_itemsToValidate.front();
    int index = m_list->ListIndexToCatalog(item);
    CatalogItem& dt = (*m_catalog)[index];

    if (!dt.IsTranslated())
        return;

    if (dt.GetValidity() != CatalogItem::Val_Unknown)
        return;

    // run this entry through msgfmt (in a single-entry catalog) to check if
    // it is correct:
    Catalog cat;
    cat.AddItem(new CatalogItem(dt));

    if (m_catalog->Header().HasHeader(_T("Plural-Forms")))
    {
        cat.Header().SetHeader(
                _T("Plural-Forms"),
                m_catalog->Header().GetHeader(_T("Plural-Forms")));
    }

    wxString tmp1 = wxFileName::CreateTempFileName(_T("poedit"));
    wxString tmp2 = wxFileName::CreateTempFileName(_T("poedit"));
    cat.Save(tmp1, false);
    wxString cmdline = _T("msgfmt -c -f -o \"") + tmp2 +
                       _T("\" \"") + tmp1 + _T("\"");

    m_validationProcess.tmp1 = tmp1;
    m_validationProcess.tmp2 = tmp2;

    wxProcess *process =
        ExecuteGettextNonblocking(cmdline, &m_validationProcess, this);
    if (process)
    {
        m_gettextProcess = process;
        m_itemBeingValidated = item;
        m_itemsToValidate.pop_front();
    }
    else
    {
        EndItemValidation();
    }
#endif
}

void PoeditFrame::EndItemValidation()
{
#ifdef USE_GETTEXT_VALIDATION
    wxASSERT( m_catalog );

    wxRemoveFile(m_validationProcess.tmp1);
    wxRemoveFile(m_validationProcess.tmp2);

    if (m_itemBeingValidated != -1)
    {
        int item = m_itemBeingValidated;
        int index = m_list->ListIndexToCatalog(item);
        CatalogItem& dt = (*m_catalog)[index];

        bool ok = (m_validationProcess.ExitCode == 0);
        dt.SetValidity(ok);

        if (!ok)
        {
            wxString err;
            for (size_t i = 0; i < m_validationProcess.Stderr.GetCount(); i++)
            {
                wxString line(m_validationProcess.Stderr[i]);
                if (!line.empty())
                {
                    err += line;
                    err += _T('\n');
                }
            }
            err.RemoveLast();
            err.Replace(m_validationProcess.tmp1, _T(""));
            if (err[0u] == _T(':'))
                err = err.Mid(1);
            err = err.AfterFirst(_T(':'));
            dt.SetErrorString(err);
        }

        m_itemBeingValidated = -1;

        if ((m_itemsToValidate.size() % 10) == 0)
        {
            UpdateStatusBar();
        }

        if (!ok)
            m_list->Refresh(item);  // Force refresh

        if (m_itemsToValidate.empty())
        {
            wxLogTrace(_T("poedit"),
                       _T("finished checking validity in background"));
        }
    }
#endif
}

void PoeditFrame::ShowPluralFormUI(bool show)
{
    if (show && (!m_catalog || m_catalog->GetPluralFormsCount() == 0))
        show = false;

    wxSizer *origSizer = m_textOrig->GetContainingSizer();
    origSizer->Show(m_labelSingular, show);
    origSizer->Show(m_labelPlural, show);
    origSizer->Show(m_textOrigPlural, show);
    origSizer->Layout();

    wxSizer *textSizer = m_textTrans->GetContainingSizer();
    textSizer->Show(m_textTrans, !show);
    textSizer->Show(m_pluralNotebook, show);
    textSizer->Layout();
}


void PoeditFrame::RecreatePluralTextCtrls()
{
    for (size_t i = 0; i < m_textTransPlural.size(); i++)
       m_textTransPlural[i]->PopEventHandler(true/*delete*/);
    m_textTransPlural.clear();
    m_pluralNotebook->DeleteAllPages();
    m_textTransSingularForm = NULL;

    if (!m_catalog)
        return;

    PluralFormsCalculator *calc = PluralFormsCalculator::make(
                m_catalog->Header().GetHeader(_T("Plural-Forms")).ToAscii());

    int cnt = m_catalog->GetPluralFormsCount();
    for (int i = 0; i < cnt; i++)
    {
        // find example number that would use this plural form:
        unsigned example = 0;
        if (calc)
        {
            for (example = 1; example < 1000; example++)
            {
                if (calc->evaluate(example) == i)
                    break;
            }
            // we prefer non-zero values, but if this form is for zero only,
            // use zero:
            if (example == 1000 && calc->evaluate(0) == i)
                example = 0;
        }
        else
            example = 1000;

        wxString desc;
        if (example == 1000)
            desc.Printf(_("Form %i"), i);
        else
            desc.Printf(_("Form %i (e.g. \"%u\")"), i, example);

        // create text control and notebook page for it:
        wxTextCtrl *txt = new wxTextCtrl(m_pluralNotebook, -1,
                                         wxEmptyString,
                                         wxDefaultPosition, wxDefaultSize,
                                         wxTE_MULTILINE | wxTE_RICH2);
        txt->PushEventHandler(new TransTextctrlHandler(this));
        m_textTransPlural.push_back(txt);
        m_pluralNotebook->AddPage(txt, desc);

        if (example == 1)
            m_textTransSingularForm = txt;
    }

    // as a fallback, assume 1st form for plural entries is the singular
    // (like in English and most real-life uses):
    if (!m_textTransSingularForm && !m_textTransPlural.empty())
        m_textTransSingularForm = m_textTransPlural[0];

    delete calc;

    SetCustomFonts();
    InitSpellchecker();
    UpdateToTextCtrl();
}

void PoeditFrame::OnListRightClick(wxMouseEvent& event)
{
    long item;
    int flags = wxLIST_HITTEST_ONITEM;
    wxListCtrl *list = (wxListCtrl*)event.GetEventObject();

    item = list->HitTest(event.GetPosition(), flags);
    if (item != -1 && (flags & wxLIST_HITTEST_ONITEM))
        list->SetItemState(item, wxLIST_STATE_SELECTED,
                                 wxLIST_STATE_SELECTED);

    wxMenu *menu = GetPopupMenu(m_list->GetSelectedCatalogItem());
    if (menu)
    {
        list->PopupMenu(menu, event.GetPosition());
        delete menu;
    }
    else event.Skip();
}

void PoeditFrame::OnListFocus(wxFocusEvent& event)
{
    if (g_focusToText)
    {
        if (m_textTrans->IsShown())
            m_textTrans->SetFocus();
        else if (!m_textTransPlural.empty())
            (m_textTransPlural)[0]->SetFocus();
    }
    else
        event.Skip();
}

void PoeditFrame::AddBookmarksMenu(wxMenu *parent)
{
    wxMenu *menu = new wxMenu();

    parent->AppendSeparator();
    parent->AppendSubMenu(menu, _("&Bookmarks"));

#if defined(__WXMAC__)
    // on Mac, Alt+something is used during normal typing, so we shouldn't
    // use it as shortcuts:
    #define LABEL_BOOKMARK_SET   _("Set Bookmark %i\tCtrl+%i")
    #define LABEL_BOOKMARK_GO    _("Go to Bookmark %i\tCtrl+Alt+%i")
#elif defined(__WXMSW__)
    #define LABEL_BOOKMARK_SET   _("Set bookmark %i\tAlt+%i")
    #define LABEL_BOOKMARK_GO    _("Go to bookmark %i\tCtrl+%i")
#elif defined(__WXGTK__)
    #define LABEL_BOOKMARK_SET   _("Set Bookmark %i\tAlt+%i")
    #define LABEL_BOOKMARK_GO    _("Go to Bookmark %i\tCtrl+%i")
#else
    #error "what is correct capitalization for this toolkit?"
#endif

    for (int i = 0; i < 10; i++)
    {
        menu->Append(ID_BOOKMARK_SET + i,
                     wxString::Format(LABEL_BOOKMARK_SET, i, i));
    }
    menu->AppendSeparator();
    for (int i = 0; i < 10; i++)
    {
        menu->Append(ID_BOOKMARK_GO + i,
                     wxString::Format(LABEL_BOOKMARK_GO, i, i));
    }
}

void PoeditFrame::OnGoToBookmark(wxCommandEvent& event)
{
    // Go to bookmark, if there is an item for it
    Bookmark bk = static_cast<Bookmark>(event.GetId() - ID_BOOKMARK_GO);
    int bkIndex = m_catalog->GetBookmarkIndex(bk);
    if (bkIndex != -1)
    {
        int listIndex = m_list->CatalogIndexToList(bkIndex);
        if (listIndex >= 0 && listIndex < m_list->GetItemCount())
        {
            m_list->EnsureVisible(listIndex);
            m_list->SetItemState(listIndex,
                                 wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
        }
    }
}

void PoeditFrame::OnSetBookmark(wxCommandEvent& event)
{
    // Set bookmark if different from the current value for the item,
    // else unset it
    int bkIndex = -1;
    int selItemIndex = m_list->GetSelectedCatalogItem();

    Bookmark bk = static_cast<Bookmark>(event.GetId() - ID_BOOKMARK_SET);
    if (m_catalog->GetBookmarkIndex(bk) == selItemIndex)
    {
        m_catalog->SetBookmark(selItemIndex, NO_BOOKMARK);
    }
    else
    {
        bkIndex = m_catalog->SetBookmark(selItemIndex, bk);
    }

    // Refresh items
    RefreshSelectedItem();
    if (bkIndex != -1)
        m_list->RefreshItem(m_list->CatalogIndexToList(bkIndex));

    // Catalog has been modified
    m_modified = true;
    UpdateTitle();
    UpdateMenu();
}


void PoeditFrame::OnSortByFileOrder(wxCommandEvent&)
{
    m_list->sortOrder.by = SortOrder::By_FileOrder;
    m_list->Sort();
}


void PoeditFrame::OnSortBySource(wxCommandEvent&)
{
    m_list->sortOrder.by = SortOrder::By_Source;
    m_list->Sort();
}


void PoeditFrame::OnSortByTranslation(wxCommandEvent&)
{
    m_list->sortOrder.by = SortOrder::By_Translation;
    m_list->Sort();
}


void PoeditFrame::OnSortUntranslatedFirst(wxCommandEvent& event)
{
    m_list->sortOrder.untransFirst = event.IsChecked();
    m_list->Sort();
}

// ------------------------------------------------------------------
//  catalog navigation
// ------------------------------------------------------------------

namespace
{

bool Pred_AnyItem(const CatalogItem&)
{
    return true;
}

bool Pred_UnfinishedItem(const CatalogItem& item)
{
    return !item.IsTranslated() || item.IsFuzzy();
}

} // anonymous namespace


void PoeditFrame::Navigate(int step, NavigatePredicate predicate, bool wrap)
{
    const int count = m_list->GetItemCount();
    if ( !count )
        return;

    const int start = m_list->GetSelection();

    int i = start;

    for ( ;; )
    {
        i += step;

        if ( i < 0 )
        {
            if ( wrap )
                i += count;
            else
                return; // nowhere to go
        }
        else if ( i >= count )
        {
            if ( wrap )
                i -= count;
            else
                return; // nowhere to go
        }

        if ( i == start )
            return; // nowhere to go

        const CatalogItem& item = m_list->ListIndexToCatalogItem(i);
        if ( predicate(item) )
        {
            m_list->Select(i);
            return;
        }
    }
}

void PoeditFrame::OnPrev(wxCommandEvent&)
{
    Navigate(-1, Pred_AnyItem, /*wrap=*/false);
}

void PoeditFrame::OnNext(wxCommandEvent&)
{
    Navigate(+1, Pred_AnyItem, /*wrap=*/false);
}

void PoeditFrame::OnPrevUnfinished(wxCommandEvent&)
{
    Navigate(-1, Pred_UnfinishedItem, /*wrap=*/false);
}

void PoeditFrame::OnNextUnfinished(wxCommandEvent&)
{
    Navigate(+1, Pred_UnfinishedItem, /*wrap=*/false);
}

void PoeditFrame::OnDoneAndNext(wxCommandEvent&)
{
    // like "next unfinished", but wraps
    Navigate(+1, Pred_UnfinishedItem, /*wrap=*/true);
}

void PoeditFrame::OnPrevPage(wxCommandEvent&)
{
    int pos = std::max(m_list->GetSelection()-10, 0);
    m_list->Select(pos);
}

void PoeditFrame::OnNextPage(wxCommandEvent&)
{
    int pos = std::min(m_list->GetSelection()+10, m_list->GetItemCount()-1);
    m_list->Select(pos);
}
