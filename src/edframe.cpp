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

#include "edframe.h"

#include <wx/wx.h>
#include <wx/checkbox.h>
#include <wx/config.h>
#include <wx/html/htmlwin.h>
#include <wx/statline.h>
#include <wx/sizer.h>
#include <wx/filedlg.h>
#include <wx/datetime.h>
#include <wx/tokenzr.h>
#include <wx/xrc/xmlres.h>
#include <wx/settings.h>
#include <wx/button.h>
#include <wx/statusbr.h>
#include <wx/stdpaths.h>
#include <wx/splitter.h>
#include <wx/fontutil.h>
#include <wx/textfile.h>
#include <wx/wupdlock.h>
#include <wx/iconbndl.h>
#include <wx/dnd.h>
#include <wx/windowptr.h>

#ifdef __WXOSX__
#import <AppKit/NSDocumentController.h>
#include "macos_helpers.h"
#endif

#include <algorithm>
#include <map>
#include <fstream>

#include "catalog.h"
#include "cat_update.h"
#include "cloud_sync.h"
#include "colorscheme.h"
#include "concurrency.h"
#include "configuration.h"
#include "crowdin_gui.h"
#include "customcontrols.h"
#include "edapp.h"
#include "editing_area.h"
#include "hidpi.h"
#include "propertiesdlg.h"
#include "prefsdlg.h"
#include "fileviewer.h"
#include "findframe.h"
#include "tm/transmem.h"
#include "language.h"
#include "progressinfo.h"
#include "commentdlg.h"
#include "main_toolbar.h"
#include "manager.h"
#include "pluralforms/pl_evaluate.h"
#include "pretranslate.h"
#include "attentionbar.h"
#include "utility.h"
#include "languagectrl.h"
#include "welcomescreen.h"
#include "errors.h"
#include "sidebar.h"
#include "spellchecking.h"
#include "str_helpers.h"


namespace
{

// Splitters with customized appearance to blend with EditingArea:

class ThinSplitter : public wxSplitterWindow
{
public:
    ThinSplitter(wxWindow *parent, const wxColour& color)
        : wxSplitterWindow(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_NOBORDER | wxSP_LIVE_UPDATE)
    {
#ifdef __WXOSX__
        SetBackgroundColour(color);
#else
        m_color = color;
#endif
    }

#ifndef __WXOSX__
    void DrawSash(wxDC& dc) override
    {
        if (m_sashPosition == 0 || !m_windowTwo || IsSashInvisible())
            return;

        auto mode = GetSplitMode();
        auto rect = (mode == wxSPLIT_HORIZONTAL)
                    ? wxRect(0, m_sashPosition, GetClientSize().x, GetSashSize())
                    : wxRect(m_sashPosition, 0, GetSashSize(), GetClientSize().y);

        dc.SetPen(m_color);
        dc.SetBrush(m_color);
        if (mode == wxSPLIT_HORIZONTAL)
            dc.DrawRectangle(rect.x, rect.y, rect.width, PX(1));
        else
            dc.DrawRectangle(rect.x, rect.y, PX(1), rect.height);

        if (GetSashSize() > PX(1))
        {
            if (mode == wxSPLIT_HORIZONTAL)
            {
                rect.y += PX(1);
                rect.height -= PX(1);
            }
            else
            {
                rect.x += PX(1);
                rect.width -= PX(1);
            }

            auto bg = GetWindow2()->GetBackgroundColour();
            dc.SetPen(bg);
            dc.SetBrush(bg);
            dc.DrawRectangle(rect);
        }
    }

private:
    wxColour m_color;
#endif // !__WXOSX__
};

} // anonymous namespace


// this should be high enough to not conflict with any wxNewId-allocated value,
PoeditFrame::PoeditFramesList PoeditFrame::ms_instances;

// but there's a check for this in the PoeditFrame ctor, too
const wxWindowID ID_POEDIT_FIRST = wxID_HIGHEST + 10000;
const unsigned   ID_POEDIT_STEP  = 1000;

const wxWindowID ID_POPUP_REFS   = ID_POEDIT_FIRST + 1*ID_POEDIT_STEP;
const wxWindowID ID_POPUP_DUMMY  = ID_POEDIT_FIRST + 3*ID_POEDIT_STEP;
const wxWindowID ID_BOOKMARK_GO  = ID_POEDIT_FIRST + 4*ID_POEDIT_STEP;
const wxWindowID ID_BOOKMARK_SET = ID_POEDIT_FIRST + 5*ID_POEDIT_STEP;

const wxWindowID ID_POEDIT_LAST  = ID_POEDIT_FIRST + 6*ID_POEDIT_STEP;


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
    wxFileName fn(filename);

    for (auto n: ms_instances)
    {
        if (wxFileName(n->GetFileName()) == fn)
            return n;
    }
    return NULL;
}

/*static*/ PoeditFrame *PoeditFrame::UnusedWindow(bool active)
{
    for (auto win: ms_instances)
    {
        if ((!active || win->IsActive()) && win->m_catalog == nullptr)
            return win;
    }
    return nullptr;
}

/*static*/ bool PoeditFrame::AnyWindowIsModified()
{
    for (PoeditFramesList::const_iterator n = ms_instances.begin();
         n != ms_instances.end(); ++n)
    {
        if ((*n)->IsModified())
            return true;
    }
    return false;
}

/*static*/ PoeditFrame *PoeditFrame::Create(const wxString& filename, int lineno)
{
    PoeditFrame *f = PoeditFrame::Find(filename);
    if (f)
    {
        f->Raise();
    }
    else
    {
        // NB: duplicated in ReadCatalog()
        CatalogPtr cat = std::make_shared<Catalog>(filename);
        if (!cat->IsOk())
        {
            wxMessageDialog dlg
            (
                nullptr,
                _("The file cannot be opened."),
                _("Invalid file"),
                wxOK | wxICON_ERROR
            );
            dlg.SetExtendedMessage(
                _("The file may be either corrupted or in a format not recognized by Poedit.")
            );
            dlg.ShowModal();
            return nullptr;
        }

        f = new PoeditFrame();
        f->Show(true);
        f->ReadCatalog(cat);
    }

    f->Show(true);

    // HACK: make sure this is called *after* the delayed call in PoeditListCtrl::CatalogChanged
    if (f->m_list)
        f->m_list->CallAfter([=]{ f->PlaceInitialFocus(lineno); });

    return f;
}

/*static*/ PoeditFrame *PoeditFrame::CreateEmpty()
{
    PoeditFrame *f = new PoeditFrame;
    f->Show(true);

    return f;
}

/*static*/ PoeditFrame *PoeditFrame::CreateWelcome()
{
    PoeditFrame *f = new PoeditFrame;
    f->EnsureContentView(Content::Welcome);
    f->Show(true);

    return f;
}


BEGIN_EVENT_TABLE(PoeditFrame, wxFrame)
// macOS and GNOME apps should open new documents in a new window. On Windows,
// however, the usual thing to do is to open the new document in the already
// open window and replace the current document.
   EVT_BUTTON         (XRCID("button_new_from_this_pot"),PoeditFrame::OnNew)
#ifdef __WXMSW__
   EVT_MENU           (wxID_NEW,                  PoeditFrame::OnNew)
   EVT_MENU           (XRCID("menu_new_from_pot"),PoeditFrame::OnNew)
   EVT_MENU           (wxID_OPEN,                 PoeditFrame::OnOpen)
  #ifdef HAVE_HTTP_CLIENT
   EVT_MENU           (XRCID("menu_open_crowdin"),PoeditFrame::OnOpenFromCrowdin)
  #endif
#endif // __WXMSW__
#ifndef __WXOSX__
   EVT_MENU_RANGE     (wxID_FILE1, wxID_FILE9,    PoeditFrame::OnOpenHist)
   EVT_MENU           (wxID_CLOSE,                PoeditFrame::OnCloseCmd)
#endif
   EVT_MENU           (wxID_SAVE,                 PoeditFrame::OnSave)
   EVT_MENU           (wxID_SAVEAS,               PoeditFrame::OnSaveAs)
   EVT_MENU           (XRCID("menu_compile_mo"),  PoeditFrame::OnCompileMO)
   EVT_MENU           (XRCID("menu_export"),      PoeditFrame::OnExport)
   EVT_MENU           (XRCID("menu_catproperties"), PoeditFrame::OnProperties)
   EVT_MENU           (XRCID("menu_update_from_src"), PoeditFrame::OnUpdateFromSources)
   EVT_MENU           (XRCID("menu_update_from_pot"),PoeditFrame::OnUpdateFromPOT)
  #ifdef HAVE_HTTP_CLIENT
   EVT_MENU           (XRCID("menu_update_from_crowdin"),PoeditFrame::OnUpdateFromCrowdin)
  #endif
   EVT_MENU           (XRCID("toolbar_update"),PoeditFrame::OnUpdateSmart)
   EVT_MENU           (XRCID("menu_validate"),    PoeditFrame::OnValidate)
   EVT_MENU           (XRCID("menu_purge_deleted"), PoeditFrame::OnPurgeDeleted)
   EVT_MENU           (XRCID("menu_fuzzy"),       PoeditFrame::OnFuzzyFlag)
   EVT_MENU           (XRCID("menu_ids"),         PoeditFrame::OnIDsFlag)
   EVT_MENU           (XRCID("sort_by_order"),    PoeditFrame::OnSortByFileOrder)
   EVT_MENU           (XRCID("sort_by_source"),    PoeditFrame::OnSortBySource)
   EVT_MENU           (XRCID("sort_by_translation"), PoeditFrame::OnSortByTranslation)
   EVT_MENU           (XRCID("sort_group_by_context"), PoeditFrame::OnSortGroupByContext)
   EVT_MENU           (XRCID("sort_untrans_first"), PoeditFrame::OnSortUntranslatedFirst)
   EVT_MENU           (XRCID("sort_errors_first"), PoeditFrame::OnSortErrorsFirst)
   EVT_MENU           (XRCID("show_sidebar"),      PoeditFrame::OnShowHideSidebar)
   EVT_UPDATE_UI      (XRCID("show_sidebar"),      PoeditFrame::OnUpdateShowHideSidebar)
   EVT_MENU           (XRCID("show_statusbar"),    PoeditFrame::OnShowHideStatusbar)
   EVT_UPDATE_UI      (XRCID("show_statusbar"),    PoeditFrame::OnUpdateShowHideStatusbar)
   EVT_MENU           (XRCID("menu_copy_from_src"), PoeditFrame::OnCopyFromSource)
   EVT_MENU           (XRCID("menu_copy_from_singular"), PoeditFrame::OnCopyFromSingular)
   EVT_MENU           (XRCID("menu_clear"),       PoeditFrame::OnClearTranslation)
   EVT_MENU           (XRCID("menu_references"),  PoeditFrame::OnReferencesMenu)
   EVT_MENU           (wxID_FIND,                 PoeditFrame::OnFind)
   EVT_MENU           (wxID_REPLACE,              PoeditFrame::OnFindAndReplace)
   EVT_MENU           (XRCID("menu_find_next"),   PoeditFrame::OnFindNext)
   EVT_MENU           (XRCID("menu_find_prev"),   PoeditFrame::OnFindPrev)
   EVT_MENU           (XRCID("menu_comment"),     PoeditFrame::OnEditComment)
   EVT_BUTTON         (XRCID("menu_comment"),     PoeditFrame::OnEditComment)
   EVT_MENU           (XRCID("go_done_and_next"),   PoeditFrame::OnDoneAndNext)
   EVT_MENU           (XRCID("go_prev"),            PoeditFrame::OnPrev)
   EVT_MENU           (XRCID("go_next"),            PoeditFrame::OnNext)
   EVT_MENU           (XRCID("go_prev_page"),       PoeditFrame::OnPrevPage)
   EVT_MENU           (XRCID("go_next_page"),       PoeditFrame::OnNextPage)
   EVT_MENU           (XRCID("go_prev_unfinished"), PoeditFrame::OnPrevUnfinished)
   EVT_MENU           (XRCID("go_next_unfinished"), PoeditFrame::OnNextUnfinished)
   EVT_MENU           (XRCID("go_prev_pluralform"), PoeditFrame::OnPrevPluralForm)
   EVT_MENU           (XRCID("go_next_pluralform"), PoeditFrame::OnNextPluralForm)
   EVT_MENU_RANGE     (ID_POPUP_REFS, ID_POPUP_REFS + 999, PoeditFrame::OnReference)
   EVT_COMMAND        (wxID_ANY, EVT_SUGGESTION_SELECTED, PoeditFrame::OnSuggestion)
   EVT_MENU           (XRCID("menu_pretranslate"), PoeditFrame::OnPreTranslateAll)
   EVT_MENU_RANGE     (ID_BOOKMARK_GO, ID_BOOKMARK_GO + 9,
                       PoeditFrame::OnGoToBookmark)
   EVT_MENU_RANGE     (ID_BOOKMARK_SET, ID_BOOKMARK_SET + 9,
                       PoeditFrame::OnSetBookmark)
   EVT_CLOSE          (                PoeditFrame::OnCloseWindow)
   EVT_SIZE           (PoeditFrame::OnSize)

   // handling of selection:
   EVT_UPDATE_UI(XRCID("menu_references"), PoeditFrame::OnReferencesMenuUpdate)
   EVT_UPDATE_UI_RANGE(ID_BOOKMARK_SET, ID_BOOKMARK_SET + 9, PoeditFrame::OnSingleSelectionUpdate)

   EVT_UPDATE_UI(XRCID("go_done_and_next"),   PoeditFrame::OnSingleSelectionUpdate)
   EVT_UPDATE_UI(XRCID("go_prev"),            PoeditFrame::OnSingleSelectionUpdate)
   EVT_UPDATE_UI(XRCID("go_next"),            PoeditFrame::OnSingleSelectionUpdate)
   EVT_UPDATE_UI(XRCID("go_prev_page"),       PoeditFrame::OnSingleSelectionUpdate)
   EVT_UPDATE_UI(XRCID("go_next_page"),       PoeditFrame::OnSingleSelectionUpdate)
   EVT_UPDATE_UI(XRCID("go_prev_unfinished"), PoeditFrame::OnSingleSelectionUpdate)
   EVT_UPDATE_UI(XRCID("go_next_unfinished"), PoeditFrame::OnSingleSelectionUpdate)
   EVT_UPDATE_UI(XRCID("go_prev_pluralform"), PoeditFrame::OnSingleSelectionWithPluralsUpdate)
   EVT_UPDATE_UI(XRCID("go_next_pluralform"), PoeditFrame::OnSingleSelectionWithPluralsUpdate)

   EVT_UPDATE_UI(XRCID("menu_fuzzy"),         PoeditFrame::OnSelectionUpdateEditable)
   EVT_UPDATE_UI(XRCID("menu_clear"),         PoeditFrame::OnSelectionUpdateEditable)
   EVT_UPDATE_UI(XRCID("menu_copy_from_src"), PoeditFrame::OnSelectionUpdateEditable)
   EVT_UPDATE_UI(XRCID("menu_copy_from_singular"), PoeditFrame::OnSingleSelectionWithPluralsUpdate)
   EVT_UPDATE_UI(XRCID("menu_comment"),       PoeditFrame::OnEditCommentUpdate)

   // handling of open files:
   EVT_UPDATE_UI(wxID_SAVE,                   PoeditFrame::OnHasCatalogUpdate)
   EVT_UPDATE_UI(wxID_SAVEAS,                 PoeditFrame::OnHasCatalogUpdate)
   EVT_UPDATE_UI(XRCID("menu_statistics"),    PoeditFrame::OnHasCatalogUpdate)
   EVT_UPDATE_UI(XRCID("menu_pretranslate"),  PoeditFrame::OnIsEditableUpdate)
   EVT_UPDATE_UI(XRCID("menu_validate"),      PoeditFrame::OnIsEditableUpdate)
   EVT_UPDATE_UI(XRCID("menu_update_from_src"), PoeditFrame::OnUpdateFromSourcesUpdate)
 #ifdef HAVE_HTTP_CLIENT
   EVT_UPDATE_UI(XRCID("menu_update_from_crowdin"), PoeditFrame::OnUpdateFromCrowdinUpdate)
 #endif
   EVT_UPDATE_UI(XRCID("menu_update_from_pot"), PoeditFrame::OnUpdateFromPOTUpdate)
   EVT_UPDATE_UI(XRCID("toolbar_update"), PoeditFrame::OnUpdateSmartUpdate)

   // handling of find/replace:
   EVT_UPDATE_UI(XRCID("menu_find_next"),   PoeditFrame::OnUpdateFind)
   EVT_UPDATE_UI(XRCID("menu_find_prev"),   PoeditFrame::OnUpdateFind)

#if defined(__WXMSW__) || defined(__WXGTK__)
   EVT_MENU(wxID_UNDO,      PoeditFrame::OnTextEditingCommand)
   EVT_MENU(wxID_REDO,      PoeditFrame::OnTextEditingCommand)
   EVT_MENU(wxID_CUT,       PoeditFrame::OnTextEditingCommand)
   EVT_MENU(wxID_COPY,      PoeditFrame::OnTextEditingCommand)
   EVT_MENU(wxID_PASTE,     PoeditFrame::OnTextEditingCommand)
   EVT_MENU(wxID_DELETE,    PoeditFrame::OnTextEditingCommand)
   EVT_MENU(wxID_SELECTALL, PoeditFrame::OnTextEditingCommand)
   EVT_UPDATE_UI(wxID_UNDO,      PoeditFrame::OnTextEditingCommandUpdate)
   EVT_UPDATE_UI(wxID_REDO,      PoeditFrame::OnTextEditingCommandUpdate)
   EVT_UPDATE_UI(wxID_CUT,       PoeditFrame::OnTextEditingCommandUpdate)
   EVT_UPDATE_UI(wxID_COPY,      PoeditFrame::OnTextEditingCommandUpdate)
   EVT_UPDATE_UI(wxID_PASTE,     PoeditFrame::OnTextEditingCommandUpdate)
   EVT_UPDATE_UI(wxID_DELETE,    PoeditFrame::OnTextEditingCommandUpdate)
   EVT_UPDATE_UI(wxID_SELECTALL, PoeditFrame::OnTextEditingCommandUpdate)
#endif
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

        auto ext = f.GetExt().Lower();
        if ( ext != "po" && ext != "pot" )
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
    PoeditFrameBase(NULL, -1, _("Poedit"),
                    wxDefaultPosition, wxDefaultSize,
                    wxDEFAULT_FRAME_STYLE | wxNO_FULL_REPAINT_ON_RESIZE,
                    "mainwin"),
    m_contentType(Content::Invalid),
    m_contentView(nullptr),
    m_catalog(nullptr),
    m_fileExistsOnDisk(false),
    m_list(nullptr),
    m_modified(false),
    m_hasObsoleteItems(false),
    m_setSashPositionsWhenMaximized(false)
{
    m_list = nullptr;
    m_editingArea = nullptr;
    m_splitter = nullptr;
    m_sidebarSplitter = nullptr;
    m_sidebar = nullptr;

    // make sure that the [ID_POEDIT_FIRST,ID_POEDIT_LAST] range of IDs is not
    // used for anything else:
    wxASSERT_MSG( wxGetCurrentId() < ID_POEDIT_FIRST ||
                  wxGetCurrentId() > ID_POEDIT_LAST,
                  "detected ID values conflict!" );
    wxRegisterId(ID_POEDIT_LAST);

    wxConfigBase *cfg = wxConfig::Get();

    m_displayIDs = (bool)cfg->Read("display_lines", (long)false);
    g_focusToText = (bool)cfg->Read("focus_to_text", (long)false);

#if defined(__WXGTK__)
    wxIconBundle appicons;
    appicons.AddIcon(wxArtProvider::GetIcon("poedit", wxART_FRAME_ICON, wxSize(16,16)));
    appicons.AddIcon(wxArtProvider::GetIcon("poedit", wxART_FRAME_ICON, wxSize(32,32)));
    appicons.AddIcon(wxArtProvider::GetIcon("poedit", wxART_FRAME_ICON, wxSize(48,48)));
    SetIcons(appicons);
#elif defined(__WXMSW__)
    SetIcons(wxIconBundle(wxStandardPaths::Get().GetResourcesDir() + "\\Resources\\Poedit.ico"));
#endif

    wxMenuBar *MenuBar = wxXmlResource::Get()->LoadMenuBar("mainmenu");
    if (MenuBar)
    {
#ifndef __WXOSX__
        m_menuForHistory = MenuBar->GetMenu(MenuBar->FindMenu(_("&File")));
        FileHistory().UseMenu(m_menuForHistory);
        FileHistory().AddFilesToMenu(m_menuForHistory);
#endif
        AddBookmarksMenu(MenuBar->GetMenu(MenuBar->FindMenu(_("&Go"))));
#ifdef __WXOSX__
        wxGetApp().TweakOSXMenuBar(MenuBar);
#endif
#ifndef HAVE_HTTP_CLIENT
        wxMenu *menu;
        wxMenuItem *item;
        item = MenuBar->FindItem(XRCID("menu_update_from_crowdin"), &menu);
        menu->Destroy(item);
        item = MenuBar->FindItem(XRCID("menu_open_crowdin"), &menu);
        menu->Destroy(item);
#endif
        SetMenuBar(MenuBar);
    }
    else
    {
        wxLogError("Cannot load main menu from resource, something must have went terribly wrong.");
        wxLog::FlushActive();
        return;
    }

    m_toolbar = MainToolbar::Create(this);

    GetMenuBar()->Check(XRCID("menu_ids"), m_displayIDs);

    if (wxConfigBase::Get()->ReadBool("/statusbar_shown", true))
        CreateStatusBar(1, wxST_SIZEGRIP);

    m_contentWrappingSizer = new wxBoxSizer(wxVERTICAL);
    SetSizer(m_contentWrappingSizer);

#ifndef __WXOSX__
    // render a separator between the toolbar and content
    SetBackgroundColour(ColorScheme::Get(Color::ToolbarSeparator));
    m_contentWrappingSizer->AddSpacer(PX(1));
#endif // !__WXOSX__

    m_attentionBar = new AttentionBar(this);
    m_contentWrappingSizer->Add(m_attentionBar, wxSizerFlags().Expand());

    SetAccelerators();

    auto defaultSize = wxGetDisplaySize();
    defaultSize.x -= PX(400);
    defaultSize.y -= PX(400);
    if (defaultSize.x > PX(1400))
        defaultSize.x = PX(1400);
    if (double(defaultSize.x) / double(defaultSize.y) > 1.6)
        defaultSize.x = defaultSize.y * 1.6;
    RestoreWindowState(this, defaultSize, WinState_Size | WinState_Pos);

    UpdateMenu();

    ms_instances.insert(this);

    SetDropTarget(new PoeditDropTarget(this));

#ifdef __WXOSX__
    NSWindow *wnd = (NSWindow*)GetWXWindow();
    [wnd setCollectionBehavior:NSWindowCollectionBehaviorFullScreenPrimary];
#endif
}


void PoeditFrame::EnsureContentView(Content type)
{
    if (m_contentType == type)
        return;

#ifdef __WXMSW__
    wxWindowUpdateLocker no_updates(this);
#endif

    if (m_contentView)
        DestroyContentView();

    switch (type)
    {
        case Content::Invalid:
            m_contentType = Content::Invalid;
            return; // nothing to do

        case Content::Welcome:
            m_contentView = CreateContentViewWelcome();
            break;

        case Content::Empty_PO:
            m_contentView = CreateContentViewEmptyPO();
            break;

        case Content::PO:
        case Content::POT:
            m_contentView = CreateContentViewPO(type);
            break;
    }

    m_contentType = type;
    m_contentWrappingSizer->Add(m_contentView, wxSizerFlags(1).Expand());
    Layout();
#ifdef __WXMSW__
    m_contentView->Show();
    Layout();
#endif
}

void PoeditFrame::EnsureAppropriateContentView()
{
    wxCHECK_RET( m_catalog, "must have catalog here" );

    if (m_catalog->empty())
    {
        EnsureContentView(Content::Empty_PO);
    }
    else
    {
        switch (m_catalog->GetFileType())
        {
            case Catalog::Type::PO:
                EnsureContentView(Content::PO);
                break;
            case Catalog::Type::POT:
                EnsureContentView(Content::POT);
                break;
        }
    }
}


wxWindow* PoeditFrame::CreateContentViewPO(Content type)
{
    auto main = new wxPanel(this, wxID_ANY);
    auto mainSizer = new wxBoxSizer(wxHORIZONTAL);
    main->SetSizer(mainSizer);

#ifdef __WXMSW__
    // don't create the window as shown, avoid flicker
    main->Hide();
#endif

    m_sidebarSplitter = new ThinSplitter(main, ColorScheme::Get(Color::SidebarSeparator));
    m_sidebarSplitter->Bind(wxEVT_SPLITTER_SASH_POS_CHANGING, &PoeditFrame::OnSidebarSplitterSashMoving, this);

    mainSizer->Add(m_sidebarSplitter, wxSizerFlags(1).Expand());

    m_splitter = new ThinSplitter(m_sidebarSplitter, ColorScheme::Get(Color::EditingSeparator));
    m_splitter->Bind(wxEVT_SPLITTER_SASH_POS_CHANGING, &PoeditFrame::OnSplitterSashMoving, this);

    // make only the upper part grow when resizing
    m_splitter->SetSashGravity(1.0);

    m_list = new PoeditListCtrl(m_splitter, wxID_ANY, m_displayIDs);

    m_editingArea = new EditingArea
                        (
                            m_splitter,
                            m_list,
                            type == Content::POT ? EditingArea::POT : EditingArea::Editing
                        );

    m_editingArea->OnUpdatedFromTextCtrl = [=](CatalogItemPtr item, bool statsChanged){
        OnUpdatedFromTextCtrl(item, statsChanged);
    };

    SetCustomFonts();

    m_splitter->SetMinimumPaneSize(PX(200));
    m_sidebarSplitter->SetMinimumPaneSize(PX(200));

    m_list->Bind(wxEVT_DATAVIEW_SELECTION_CHANGED, &PoeditFrame::OnListSel, this);
    m_list->Bind(wxEVT_DATAVIEW_ITEM_CONTEXT_MENU, &PoeditFrame::OnListRightClick, this);
#ifdef wxHAS_GENERIC_DATAVIEWCTRL
    m_list->GetChildren()[0]->Bind(wxEVT_SET_FOCUS, &PoeditFrame::OnListFocus, this);
#else
    m_list->Bind(wxEVT_SET_FOCUS, &PoeditFrame::OnListFocus, this);
#endif

    auto suggestionsMenu = GetMenuBar()->FindItem(XRCID("menu_suggestions"))->GetSubMenu();
    m_sidebar = new Sidebar(m_sidebarSplitter, suggestionsMenu);
    m_sidebar->Bind(wxEVT_UPDATE_UI, &PoeditFrame::OnSingleSelectionUpdate, this);

    UpdateMenu();

    switch ( m_list->sortOrder().by )
    {
        case SortOrder::By_FileOrder:
            GetMenuBar()->Check(XRCID("sort_by_order"), true);
            break;
        case SortOrder::By_Source:
            GetMenuBar()->Check(XRCID("sort_by_source"), true);
            break;
        case SortOrder::By_Translation:
            GetMenuBar()->Check(XRCID("sort_by_translation"), true);
            break;
    }
    GetMenuBar()->Check(XRCID("sort_group_by_context"), m_list->sortOrder().groupByContext);
    GetMenuBar()->Check(XRCID("sort_untrans_first"), m_list->sortOrder().untransFirst);
    GetMenuBar()->Check(XRCID("sort_errors_first"), m_list->sortOrder().errorsFirst);

    // Call splitter splitting later, when the window is laid out, otherwise
    // the sizes would get truncated immediately:
    CallAfter([=]{
        // This is a hack -- windows are not maximized immediately and so we can't
        // set correct sash position in ctor (unmaximized window may be too small
        // for sash position when maximized -- see bug #2120600)
        if ( wxConfigBase::Get()->Read(WindowStatePath(this) + "maximized", long(0)) )
            m_setSashPositionsWhenMaximized = true;

        if (wxConfigBase::Get()->ReadBool("/sidebar_shown", true))
        {
            auto split = GetSize().x * wxConfigBase::Get()->ReadDouble("/sidebar_splitter", 0.75);
            m_sidebarSplitter->SplitVertically(m_splitter, m_sidebar, split);
        }
        else
        {
            m_sidebar->Hide();
            m_sidebarSplitter->Initialize(m_splitter);
            Layout();
        }

        m_splitter->SplitHorizontally(m_list, m_editingArea, (int)wxConfigBase::Get()->ReadLong("/splitter", -PX(320)));

        if (m_sidebar)
            m_sidebar->SetUpperHeight(m_splitter->GetSashPosition());
    });

    return main;
}


wxWindow* PoeditFrame::CreateContentViewWelcome()
{
    return new WelcomeScreenPanel(this);
}


wxWindow* PoeditFrame::CreateContentViewEmptyPO()
{
    return new EmptyPOScreenPanel(this);
}


void PoeditFrame::DestroyContentView()
{
    if (!m_contentView)
        return;

    NotifyCatalogChanged(nullptr);

    if (m_splitter)
        wxConfigBase::Get()->Write("/splitter", (long)m_splitter->GetSashPosition());

    m_contentWrappingSizer->Detach(m_contentView);
    m_contentView->Destroy();
    m_contentView = nullptr;

    m_list = nullptr;
    m_splitter = nullptr;
    m_sidebarSplitter = nullptr;
    m_sidebar = nullptr;
    m_editingArea = nullptr;

    if (m_findWindow)
    {
        m_findWindow->Destroy();
        m_findWindow.Release();
    }
}


PoeditFrame::~PoeditFrame()
{
    ms_instances.erase(this);

    DestroyContentView();

    wxConfigBase *cfg = wxConfig::Get();
    cfg->SetPath("/");

    cfg->Write("display_lines", m_displayIDs);

    SaveWindowState(this);

#ifndef __WXOSX__
    FileHistory().RemoveMenu(m_menuForHistory);
    FileHistory().Save(*cfg);
#endif

    // write all changes:
    cfg->Flush();

    m_catalog.reset();
    m_pendingHumanEditedItem.reset();

    // shutdown the spellchecker:
    InitSpellchecker();
}


void PoeditFrame::PlaceInitialFocus(int lineno)
{
    if (g_focusToText && m_editingArea)
        m_editingArea->SetTextFocus();
    else if (m_list)
        m_list->SetFocus();

    if (m_catalog && m_list && m_list->GetItemCount() > 0)
    {
        int item = 0;
        if (lineno > 0)
        {
            item = m_catalog->FindItemIndexByLine(lineno);
            item = (item == -1) ? 0 : m_list->CatalogIndexToList(item);
        }
        m_list->SelectAndFocus(item);
    }
}


void PoeditFrame::SetAccelerators()
{
    wxAcceleratorEntry entries[] = {
#ifdef __WXMSW__
        { wxACCEL_CTRL, WXK_F3,                 XRCID("menu_find_next") },
        { wxACCEL_CTRL | wxACCEL_SHIFT, WXK_F3, XRCID("menu_find_prev") },
#endif

        { wxACCEL_CTRL, WXK_PAGEUP,             XRCID("go_prev_page") },
        { wxACCEL_CTRL, WXK_NUMPAD_PAGEUP,      XRCID("go_prev_page") },
        { wxACCEL_CTRL, WXK_PAGEDOWN,           XRCID("go_next_page") },
        { wxACCEL_CTRL, WXK_NUMPAD_PAGEDOWN,    XRCID("go_next_page") },

        { wxACCEL_CTRL | wxACCEL_SHIFT, WXK_UP,             XRCID("go_prev_unfinished") },
        { wxACCEL_CTRL | wxACCEL_SHIFT, WXK_NUMPAD_UP,      XRCID("go_prev_unfinished") },
        { wxACCEL_CTRL | wxACCEL_SHIFT, WXK_DOWN,           XRCID("go_next_unfinished") },
        { wxACCEL_CTRL | wxACCEL_SHIFT, WXK_NUMPAD_DOWN,    XRCID("go_next_unfinished") },

        { wxACCEL_CTRL, WXK_UP,                 XRCID("go_prev") },
        { wxACCEL_CTRL, WXK_NUMPAD_UP,          XRCID("go_prev") },
        { wxACCEL_CTRL, WXK_DOWN,               XRCID("go_next") },
        { wxACCEL_CTRL, WXK_NUMPAD_DOWN,        XRCID("go_next") },

        { wxACCEL_CTRL, WXK_RETURN,             XRCID("go_done_and_next") },
        { wxACCEL_CTRL, WXK_NUMPAD_ENTER,       XRCID("go_done_and_next") }
    };

    wxAcceleratorTable accel(WXSIZEOF(entries), entries);
    SetAcceleratorTable(accel);
}


void PoeditFrame::InitSpellchecker()
{
    if (!IsSpellcheckingAvailable())
        return;

    if (!m_catalog || !m_catalog->HasCapability(Catalog::Cap::Translations))
        return;

    Language lang = m_catalog->GetLanguage();

    bool report_problem = false;
    bool enabled = m_catalog &&
                #ifndef __WXMSW__ // language choice is automatic, per-keyboard on Windows
                   lang.IsValid() &&
                #endif
                   wxConfig::Get()->Read("enable_spellchecking",
                                         (long)true);
    const bool enabledInitially = enabled;

#ifdef __WXOSX__
    if (enabled)
    {
        if ( !SetSpellcheckerLang(lang.LangAndCountry()) )
        {
            enabled = false;
            report_problem = true;
        }
    }
#endif

    if (m_editingArea && !m_editingArea->InitSpellchecker(enabled, lang))
        report_problem = true;

#ifndef __WXMSW__ // language choice is automatic, per-keyboard on Windows, can't fail
    if ( enabledInitially && report_problem )
    {
        // Some languages don't have a reasonable spellchecking method or hunspell support:
        const bool notCapable = lang.Lang() == "zh" || lang.Lang() == "ja";
        if (!notCapable)
        {
            AttentionMessage msg
            (
                "missing-spell-dict",
                AttentionMessage::Warning,
                wxString::Format
                (
                    // TRANSLATORS: %s is language name in its basic form (as you
                    // would see e.g. in a list of supported languages). You may need
                    // to rephrase it, e.g. to an equivalent of "for language %s".
                    _(L"Spellchecking is disabled, because the dictionary for %s isn’t installed."),
                    lang.LanguageDisplayName()
                )
            );
            msg.AddAction(_("Install"), []{ ShowSpellcheckerHelp(); });
            msg.AddDontShowAgain();
            m_attentionBar->ShowMessage(msg);
        }
    }
#endif // !__WXMSW__
}


void PoeditFrame::UpdateTextLanguage()
{
    if (!m_catalog)
        return;

    if (m_editingArea)
    {
        InitSpellchecker();
        m_editingArea->SetLanguage(m_catalog->GetLanguage());
    }

    if (m_sidebar)
        m_sidebar->RefreshContent();
}


#ifndef __WXOSX__
void PoeditFrame::OnCloseCmd(wxCommandEvent&)
{
    Close();
}
#endif


void PoeditFrame::OpenFile(const wxString& filename, int lineno)
{
    DoIfCanDiscardCurrentDoc([=]{
        DoOpenFile(filename, lineno);
    });
}


void PoeditFrame::DoOpenFile(const wxString& filename, int lineno)
{
    ReadCatalog(filename);

    // HACK: make sure this is called *after* the delayed call in PoeditListCtrl::CatalogChanged
    if (m_list)
        m_list->CallAfter([=]{ PlaceInitialFocus(lineno); });
}


bool PoeditFrame::NeedsToAskIfCanDiscardCurrentDoc() const
{
    return m_catalog && m_modified;
}

template<typename TFunctor>
void PoeditFrame::DoIfCanDiscardCurrentDoc(TFunctor completionHandler)
{
    if ( !NeedsToAskIfCanDiscardCurrentDoc() )
    {
        completionHandler();
        return;
    }

    wxWindowPtr<wxMessageDialog> dlg = CreateAskAboutSavingDialog();

    dlg->ShowWindowModalThenDo([this,dlg,completionHandler](int retval) {
        // hide the dialog asap, WriteCatalog() may show another modal sheet
        dlg->Hide();
#ifdef __WXOSX__
        // Hide() alone is not sufficient on macOS, we need to destroy dlg
        // shared_ptr and only then continue. Because this code is called
        // from event loop (and not this functions' caller) at an unspecified
        // time anyway, we can just as well defer it into the next idle time
        // iteration.
        CallAfter([this,retval,completionHandler]() {
#endif

        if (retval == wxID_YES)
        {
            auto doSaveFile = [=](const wxString& fn){
                WriteCatalog(fn, [=](bool saved){
                    if (saved)
                        completionHandler();
                });
            };
            if (!m_fileExistsOnDisk || GetFileName().empty())
                GetSaveAsFilenameThenDo(m_catalog, doSaveFile);
            else
                doSaveFile(GetFileName());
        }
        else if (retval == wxID_NO)
        {
            // call completion without saving the document
            completionHandler();
        }
        else if (retval == wxID_CANCEL)
        {
            // do not call -- not OK
        }

#ifdef __WXOSX__
        });
#endif
    });
}

wxWindowPtr<wxMessageDialog> PoeditFrame::CreateAskAboutSavingDialog()
{
    wxWindowPtr<wxMessageDialog> dlg(new wxMessageDialog
                    (
                        this,
                        _("Catalog modified. Do you want to save changes?"),
                        _("Save changes"),
                        wxYES_NO | wxCANCEL | wxICON_QUESTION
                    ));
    dlg->SetExtendedMessage(_("Your changes will be lost if you don't save them."));
    dlg->SetYesNoLabels
         (
            _("Save"),
        #ifdef __WXMSW__
            _("Don't save")
        #else
            _("Don't Save")
        #endif
         );
    return dlg;
}



void PoeditFrame::OnCloseWindow(wxCloseEvent& event)
{
    if (event.CanVeto() && NeedsToAskIfCanDiscardCurrentDoc())
    {
#ifdef __WXOSX__
        // Veto the event by default, then window-modally ask for permission.
        // If it turns out that the window can be closed, the completion handler
        // will do it:
        event.Veto();
        DoIfCanDiscardCurrentDoc([=]{
            Destroy();
        });
#else // !__WXOSX__
        // DoIfCanDiscardCurentDoc() doesn't have on-failure callback and
        // so we instead veto preemtively and then un-veto it. Note that this
        // only works because on non-OSX platforms the question dialog is
        // modal and the code below called immediately.
        event.Veto();
        DoIfCanDiscardCurrentDoc([=, &event]{
            event.Veto(false);
            Destroy();
        });
#endif
    }
    else // can't veto
    {
        Destroy();
    }
}


void PoeditFrame::OnOpen(wxCommandEvent&)
{
    DoIfCanDiscardCurrentDoc([=]{

        wxString path = wxPathOnly(GetFileName());
        if (path.empty())
            path = wxConfig::Get()->Read("last_file_path", wxEmptyString);

        wxString name = wxFileSelector(MACOS_OR_OTHER("", _("Open catalog")),
                        path, wxEmptyString, wxEmptyString,
                        Catalog::GetAllTypesFileMask(),
                        wxFD_OPEN | wxFD_FILE_MUST_EXIST, this);

        if (!name.empty())
        {
            wxConfig::Get()->Write("last_file_path", wxPathOnly(name));

            DoOpenFile(name);
        }
    });
}


#ifdef HAVE_HTTP_CLIENT
void PoeditFrame::OnOpenFromCrowdin(wxCommandEvent&)
{
    DoIfCanDiscardCurrentDoc([=]{
        CrowdinOpenFile(this, [=](wxString name){
            DoOpenFile(name);
            if (m_catalog)
                m_catalog->AttachCloudSync(std::make_shared<CrowdinSyncDestination>());
        });
    });
}
#endif


#ifndef __WXOSX__
void PoeditFrame::OnOpenHist(wxCommandEvent& event)
{
    wxString f(FileHistory().GetHistoryFile(event.GetId() - wxID_FILE1));
    if ( !wxFileExists(f) )
    {
        wxLogError(_("File '%s' doesn't exist."), f.c_str());
        return;
    }

    OpenFile(f);
}
#endif // !__WXOSX__


void PoeditFrame::OnSave(wxCommandEvent& event)
{
    try
    {
        if (!m_fileExistsOnDisk || GetFileName().empty())
            OnSaveAs(event);
        else
            WriteCatalog(GetFileName());
    }
    catch (Exception& e)
    {
        wxLogError("%s", e.What());
    }
}


static wxString SuggestFileName(const CatalogPtr& catalog)
{
    wxString name;
    if (catalog)
        name = catalog->GetLanguage().Code();

    if (name.empty())
        return "default";
    else
        return name;
}

template<typename F>
void PoeditFrame::GetSaveAsFilenameThenDo(const CatalogPtr& cat, F then)
{
    auto current = cat->GetFileName();
    wxString name(wxFileNameFromPath(current));
    wxString path(wxPathOnly(current));

    if (name.empty())
    {
        path = wxConfig::Get()->Read("last_file_path", wxEmptyString);
        name = SuggestFileName(cat) + ".po";
    }

    wxWindowPtr<wxFileDialog> dlg(
        new wxFileDialog(this,
                         MACOS_OR_OTHER("", _("Save as...")),
                         path,
                         name,
                         m_catalog->GetFileMask(),
                         wxFD_SAVE | wxFD_OVERWRITE_PROMPT));

    dlg->ShowWindowModalThenDo([=](int retcode){
        if (retcode != wxID_OK)
            return;
        auto fn = dlg->GetPath();
        wxConfig::Get()->Write("last_file_path", wxPathOnly(name));
        then(fn);
    });
}

void PoeditFrame::DoSaveAs(const wxString& filename)
{
    if (filename.empty())
        return;

    WriteCatalog(filename);
}

void PoeditFrame::OnSaveAs(wxCommandEvent&)
{
    GetSaveAsFilenameThenDo(m_catalog, [=](const wxString& fn){
        DoSaveAs(fn);
    });
}

void PoeditFrame::OnCompileMO(wxCommandEvent&)
{
    auto fileName = GetFileName();
    wxString name;
    wxFileName::SplitPath(fileName, nullptr, &name, nullptr);

    if (name.empty())
    {
        name = SuggestFileName(m_catalog) + ".mo";
    }
    else
        name += ".mo";

    wxWindowPtr<wxFileDialog> dlg(
        new wxFileDialog(this,
                         MACOS_OR_OTHER("", _("Compile to...")),
                         wxPathOnly(fileName),
                         name,
                         wxString::Format("%s (*.mo)|*.mo", _("Compiled Translation Files")),
                         wxFD_SAVE | wxFD_OVERWRITE_PROMPT));

    dlg->ShowWindowModalThenDo([=](int retcode){
        if (retcode != wxID_OK)
            return;

        wxBusyCursor bcur;
        auto fn = dlg->GetPath();
        wxConfig::Get()->Write("last_file_path", wxPathOnly(fn));
        int validation_errors = 0;
        Catalog::CompilationStatus compilation_status = Catalog::CompilationStatus::NotDone;
        m_catalog->CompileToMO(fn, validation_errors, compilation_status);

        if (validation_errors)
        {
            // Note: this may show window-modal window and because we may
            //       be called from such window too, run this in the next
            //       event loop iteration.
            CallAfter([=]{
                ReportValidationErrors(validation_errors, compilation_status, /*from_save=*/true, /*other_file_saved=*/false, []{});
            });
        }
    });
}

void PoeditFrame::OnExport(wxCommandEvent&)
{
    auto fileName = GetFileName();
    wxString name;
    wxFileName::SplitPath(fileName, nullptr, &name, nullptr);

    if (name.empty())
    {
        name = SuggestFileName(m_catalog) + ".html";
    }
    else
        name += ".html";

    wxWindowPtr<wxFileDialog> dlg(
        new wxFileDialog(this,
                         MACOS_OR_OTHER("", _("Export as...")),
                         wxPathOnly(fileName),
                         name,
                         wxString::Format("%s (*.html)|*.html", _("HTML Files")),
                         wxFD_SAVE | wxFD_OVERWRITE_PROMPT));

    dlg->ShowWindowModalThenDo([=](int retcode){
        if (retcode != wxID_OK)
            return;
        auto fn = dlg->GetPath();
        wxConfig::Get()->Write("last_file_path", wxPathOnly(fn));
        ExportCatalog(fn);
    });
}

bool PoeditFrame::ExportCatalog(const wxString& filename)
{
    wxBusyCursor bcur;

    TempOutputFileFor tempfile(filename);
    std::ofstream f;
    f.open(tempfile.FileName().fn_str());
    m_catalog->ExportToHTML(f);
    f.close();
    if (!tempfile.Commit())
    {
        wxLogError(_("Couldn't save file %s."), filename);
        return false;
    }
    return true;
}



void PoeditFrame::OnNew(wxCommandEvent& event)
{
    DoIfCanDiscardCurrentDoc([=]{
        if (event.GetId() == XRCID("menu_new_from_pot"))
        {
            NewFromPOT();
        }
        else if (event.GetId() == XRCID("button_new_from_this_pot"))
        {
            wxWindowPtr<LanguageDialog> dlg(new LanguageDialog(this));
            dlg->ShowWindowModalThenDo([=](int retcode){
                if (retcode == wxID_OK)
                    NewFromPOT(m_catalog->GetFileName(), dlg->GetLang());
            });
        }
        else
        {
            NewFromScratch();
        }
    });
}


void PoeditFrame::NewFromPOT()
{
    wxString path = wxPathOnly(GetFileName());
    if (path.empty())
        path = wxConfig::Get()->Read("last_file_path", wxEmptyString);
    wxString pot_file =
        wxFileSelector(_("Open catalog template"),
             path, wxEmptyString, wxEmptyString,
             Catalog::GetTypesFileMask({Catalog::Type::POT, Catalog::Type::PO}),
             wxFD_OPEN | wxFD_FILE_MUST_EXIST, this);
    if (!pot_file.empty())
    {
        wxConfig::Get()->Write("last_file_path", wxPathOnly(pot_file));
        NewFromPOT(pot_file);
    }
}

void PoeditFrame::NewFromPOT(const wxString& pot_file, Language language)
{
    auto catalog = Catalog::CreateFromPOT(pot_file);
    if (!catalog)
    {
        return;
    }

    m_catalog = catalog;
    m_pendingHumanEditedItem.reset();

    m_fileExistsOnDisk = false;
    m_modified = true;

    EnsureAppropriateContentView();
    NotifyCatalogChanged(m_catalog);

    UpdateTitle();
    UpdateMenu();
    UpdateStatusBar();
    UpdateTextLanguage();

    auto setLanguageFunc = [=](Language lang)
    {
        if (lang.IsValid())
        {
            catalog->SetLanguage(lang);

            // Derive save location for the file from the location of the POT
            // file (same directory, language-based name). This doesn't always
            // work, e.g. WordPress plugins use different naming, so don't actually
            // save the file just yet and let the user confirm the location when saving.
            wxFileName pot_fn(pot_file);
            pot_fn.SetFullName(lang.Code() + ".po");
            m_catalog->SetFileName(pot_fn.GetFullPath());
        }
        else
        {
            // default to English style plural
            if (catalog->HasPluralItems())
                catalog->Header().SetHeaderNotEmpty("Plural-Forms", "nplurals=2; plural=(n != 1);");
        }

        RecreatePluralTextCtrls();
        UpdateTitle();
        UpdateMenu();
        UpdateStatusBar();
        UpdateTextLanguage();
        NotifyCatalogChanged(m_catalog); // refresh language column
    };

    if (language.IsValid())
    {
        setLanguageFunc(language);
        PlaceInitialFocus();
    }
    else
    {
        // Choose the language:
        wxWindowPtr<LanguageDialog> dlg(new LanguageDialog(this));

        dlg->ShowWindowModalThenDo([=](int retcode){
            if (retcode == wxID_OK)
                setLanguageFunc(dlg->GetLang());
            else
                setLanguageFunc(Language());
            PlaceInitialFocus();
        });
    }
}


void PoeditFrame::NewFromScratch()
{
    CatalogPtr catalog = std::make_shared<Catalog>();
    catalog->CreateNewHeader();

    m_catalog = catalog;
    m_pendingHumanEditedItem.reset();

    m_fileExistsOnDisk = false;
    m_modified = true;

    EnsureContentView(Content::Empty_PO);

    UpdateTitle();
    UpdateMenu();
    UpdateStatusBar();

    // Choose the language:
    wxWindowPtr<LanguageDialog> dlg(new LanguageDialog(this));

    dlg->ShowWindowModalThenDo([=](int retcode){
        if (retcode == wxID_OK)
        {
            catalog->SetLanguage(dlg->GetLang());
        }
    });
}


void PoeditFrame::OnProperties(wxCommandEvent&)
{
    EditCatalogProperties();
}

void PoeditFrame::EditCatalogProperties()
{
    wxWindowPtr<PropertiesDialog> dlg(new PropertiesDialog(this, m_catalog, m_fileExistsOnDisk));

    const Language prevLang = m_catalog->GetLanguage();
    dlg->TransferTo(m_catalog);
    dlg->ShowWindowModalThenDo([=](int retcode){
        if (retcode == wxID_OK)
        {
            dlg->TransferFrom(m_catalog);
            m_modified = true;
            RecreatePluralTextCtrls();
            UpdateTitle();
            UpdateMenu();
            if (prevLang != m_catalog->GetLanguage())
            {
                UpdateTextLanguage();
                // trigger resorting and language header update:
                NotifyCatalogChanged(m_catalog);
            }
        }
    });
}

void PoeditFrame::EditCatalogPropertiesAndUpdateFromSources()
{
    // TODO: share code with EditCatalogProperties()

    wxWindowPtr<PropertiesDialog> dlg(new PropertiesDialog(this, m_catalog, m_fileExistsOnDisk, 1));

    const Language prevLang = m_catalog->GetLanguage();
    dlg->TransferTo(m_catalog);
    dlg->ShowWindowModalThenDo([=](int retcode){
        if (retcode == wxID_OK)
        {
            dlg->TransferFrom(m_catalog);
            m_modified = true;
            if (m_list)
                RecreatePluralTextCtrls();
            UpdateTitle();
            UpdateMenu();
            if (prevLang != m_catalog->GetLanguage())
            {
                UpdateTextLanguage();
                // trigger resorting and language header update:
                NotifyCatalogChanged(m_catalog);
            }

            if (!m_catalog->Header().SearchPaths.empty())
            {
                EnsureAppropriateContentView();
                UpdateCatalog();
            }
        }
    });
}


void PoeditFrame::UpdateAfterPreferencesChange()
{
    g_focusToText = (bool)wxConfig::Get()->Read("focus_to_text",
                                                 (long)false);

    if (m_list)
    {
        SetCustomFonts();
        m_list->Refresh(); // if font changed
        UpdateTextLanguage();
    }
}

/*static*/ void PoeditFrame::UpdateAllAfterPreferencesChange()
{
    for (PoeditFramesList::const_iterator n = ms_instances.begin();
         n != ms_instances.end(); ++n)
    {
        (*n)->UpdateAfterPreferencesChange();
    }
}


bool PoeditFrame::UpdateCatalog(const wxString& pot_file)
{
    // This ensures that the list control won't be redrawn during Update()
    // call when a dialog box is hidden; another alternative would be to call
    // m_list->CatalogChanged(NULL) here
    std::unique_ptr<wxWindowUpdateLocker> locker;
    if (m_list)
        locker.reset(new wxWindowUpdateLocker(m_list));


    UpdateResultReason reason = UpdateResultReason::Unspecified;
    bool succ;

    if (pot_file.empty())
    {
        if (m_catalog->HasSourcesAvailable())
        {
            succ = PerformUpdateFromSources(this, m_catalog, reason);

            locker.reset();
            EnsureAppropriateContentView();
            NotifyCatalogChanged(m_catalog);
        }
        else
        {
            reason = UpdateResultReason::NoSourcesFound;
            succ = false;
        }
    }
    else
    {
        succ = PerformUpdateFromPOT(this, m_catalog, pot_file, reason);

        locker.reset();
        EnsureAppropriateContentView();
        NotifyCatalogChanged(m_catalog);
    }

    m_modified = succ || m_modified;
    UpdateStatusBar();

    if (!succ)
    {
        switch (reason)
        {
            case UpdateResultReason::NoSourcesFound:
            {
                wxWindowPtr<wxMessageDialog> dlg(new wxMessageDialog
                    (
                        this,
                        _("Source code not available."),
                        MSW_OR_OTHER(_("Updating failed"), ""),
                        wxOK | wxICON_ERROR
                    ));
                dlg->SetExtendedMessage(_(L"Translations couldn’t be updated from the source code, because no code was found in the location specified in the catalog’s Properties."));
                dlg->ShowWindowModalThenDo([dlg](int){});
                break;
            }
            case UpdateResultReason::Unspecified:
            {
                wxLogWarning(_("Entries in the catalog are probably incorrect."));
                wxLogError(
                   _("Updating the catalog failed. Click on 'Details >>' for details."));
                break;
            }
            case UpdateResultReason::CancelledByUser:
                break;
        }
    }

    return succ;
}

void PoeditFrame::OnUpdateFromSources(wxCommandEvent&)
{
    DoIfCanDiscardCurrentDoc([=]{
        try
        {
            if (UpdateCatalog())
            {
                if (Config::UseTM() && Config::MergeBehavior() == Merge_UseTM)
                {
                    if (PreTranslateCatalog(this, m_catalog, PreTranslate_OnlyGoodQuality, nullptr))
                    {
                        if (!m_modified)
                        {
                            m_modified = true;
                            UpdateTitle();
                        }
                        RefreshControls();
                    }
                }
            }
        }
        catch (...)
        {
            wxLogError("%s", DescribeCurrentException());
        }

        RefreshControls();
    });
}

void PoeditFrame::OnUpdateFromSourcesUpdate(wxUpdateUIEvent& event)
{
    event.Enable(m_catalog &&
                 !m_catalog->IsFromCrowdin() &&
                 m_catalog->HasSourcesConfigured());
}

void PoeditFrame::OnUpdateFromPOT(wxCommandEvent&)
{
    DoIfCanDiscardCurrentDoc([=]{
        wxString path = wxPathOnly(GetFileName());
        if (path.empty())
            path = wxConfig::Get()->Read("last_file_path", wxEmptyString);

        wxWindowPtr<wxFileDialog> dlg(
            new wxFileDialog(this,
                             _("Open catalog template"),
                             path,
                             wxEmptyString,
                             Catalog::GetTypesFileMask({Catalog::Type::POT, Catalog::Type::PO}),
                             wxFD_OPEN | wxFD_FILE_MUST_EXIST));

        dlg->ShowWindowModalThenDo([=](int retcode){
            if (retcode != wxID_OK)
                return;
            auto pot_file = dlg->GetPath();
            wxConfig::Get()->Write("last_file_path", wxPathOnly(pot_file));
            try
            {
                if (UpdateCatalog(pot_file))
                {
                    if (Config::UseTM() && Config::MergeBehavior() == Merge_UseTM)
                    {
                        if (PreTranslateCatalog(this, m_catalog, PreTranslate_OnlyGoodQuality, nullptr))
                        {
                            if (!m_modified)
                            {
                                m_modified = true;
                                UpdateTitle();
                            }
                            RefreshControls();
                        }
                    }
                }
            }
            catch (...)
            {
                wxLogError("%s", DescribeCurrentException());
            }
        });

        RefreshControls();
    });
}

void PoeditFrame::OnUpdateFromPOTUpdate(wxUpdateUIEvent& event)
{
    if (!m_catalog || m_catalog->GetFileType() != Catalog::Type::PO)
        event.Enable(false);
    else
        OnHasCatalogUpdate(event);
}

#ifdef HAVE_HTTP_CLIENT
void PoeditFrame::OnUpdateFromCrowdin(wxCommandEvent&)
{
    DoIfCanDiscardCurrentDoc([=]{
        CrowdinSyncFile(this, m_catalog, [=](std::shared_ptr<Catalog> cat){
            m_catalog = cat;
            EnsureAppropriateContentView();
            NotifyCatalogChanged(m_catalog);
            RefreshControls();
        });
    });
}

void PoeditFrame::OnUpdateFromCrowdinUpdate(wxUpdateUIEvent& event)
{
    event.Enable(m_catalog && m_catalog->IsFromCrowdin() &&
                 m_catalog->HasCapability(Catalog::Cap::Translations));
}
#endif

void PoeditFrame::OnUpdateSmart(wxCommandEvent& event)
{
    if (!m_catalog)
        return;
#ifdef HAVE_HTTP_CLIENT
    if (m_catalog->IsFromCrowdin())
        OnUpdateFromCrowdin(event);
    else
#endif
        OnUpdateFromSources(event);
}

void PoeditFrame::OnUpdateSmartUpdate(wxUpdateUIEvent& event)
{
    event.Enable(false);
    if (m_catalog)
    {
#ifdef HAVE_HTTP_CLIENT
       if (m_catalog->IsFromCrowdin())
            OnUpdateFromCrowdinUpdate(event);
        else
#endif
            OnUpdateFromSourcesUpdate(event);
    }
}


void PoeditFrame::OnValidate(wxCommandEvent&)
{
    try
    {
        wxBusyCursor bcur;
        ReportValidationErrors(m_catalog->Validate(),
                               /*mo_compilation_failed=*/Catalog::CompilationStatus::NotDone,
                               /*from_save=*/false, /*other_file_saved=*/false, []{});
    }
    catch (Exception& e)
    {
        wxLogError("%s", e.What());
    }
}


template<typename TFunctor>
void PoeditFrame::ReportValidationErrors(int errors,
                                         Catalog::CompilationStatus mo_compilation_status,
                                         bool from_save, bool other_file_saved,
                                         TFunctor completionHandler)
{
    wxWindowPtr<wxMessageDialog> dlg;

    // Refresh the list even without errors, because there may be new warnings
    if (m_list && m_catalog->GetCount())
        m_list->RefreshAllItems();

    if ( errors )
    {
        RefreshControls();

        dlg.reset(new wxMessageDialog
        (
            this,
            wxString::Format
            (
                wxPLURAL("%d issue with the translation found.",
                         "%d issues with the translation found.",
                         errors),
                errors
            ),
            _("Validation results"),
            wxOK | wxICON_ERROR
        ));
        wxString details = _("Entries with errors were marked in red in the list. Details of the error will be shown when you select such an entry.");
        if ( from_save )
        {
            details += "\n\n";
            if (other_file_saved)
            {
                switch ( mo_compilation_status )
                {
                    case Catalog::CompilationStatus::NotDone:
                        details += _("The file was saved safely.");
                        break;
                    case Catalog::CompilationStatus::Success:
                        details += _("The file was saved safely and compiled into the MO format, but it will probably not work correctly.");
                        break;
                    case Catalog::CompilationStatus::Error:
                        details += _("The file was saved safely, but it cannot be compiled into the MO format and used.");
                        break;
                }
            }
            else // saving only the MO file
            {
                switch ( mo_compilation_status )
                {
                    case Catalog::CompilationStatus::Success:
                        details += _("The file was compiled into the MO format, but it will probably not work correctly.");
                        break;
                    case Catalog::CompilationStatus::NotDone:
                    case Catalog::CompilationStatus::Error:
                        details += _("The file cannot be compiled into the MO format and used.");
                        break;
                }
            }
        }
        dlg->SetExtendedMessage(details);
    }
    else
    {
        wxASSERT( !from_save );

        dlg.reset(new wxMessageDialog
        (
            this,
            _("No problems with the translation found."),
            _("Validation results"),
            wxOK | wxICON_INFORMATION
        ));

        wxString details;
        int unfinished = 0;
        m_catalog->GetStatistics(nullptr, nullptr, nullptr, nullptr, &unfinished);
        if (unfinished)
        {
            details = wxString::Format(wxPLURAL("The translation is ready for use, but %d entry is not translated yet.",
                                                "The translation is ready for use, but %d entries are not translated yet.", unfinished), unfinished);
        }
        else
        {
            details = _("The translation is ready for use.");
        }
        dlg->SetExtendedMessage(details);
    }

    dlg->ShowWindowModalThenDo([dlg,completionHandler](int){
        completionHandler();
    });
}


void PoeditFrame::OnListSel(wxDataViewEvent& event)
{
    bool multipleSel = m_list && m_list->HasMultipleSelection();
    bool hasTextFocus = m_editingArea->HasTextFocus();

    event.Skip();

    if (m_pendingHumanEditedItem)
    {
        OnNewTranslationEntered(m_pendingHumanEditedItem);
        m_pendingHumanEditedItem.reset();
    }

    if (multipleSel)
    {
        m_editingArea->SetMultipleSelectionMode();
    }
    else
    {
        m_editingArea->SetSingleSelectionMode();
        UpdateToTextCtrl(EditingArea::ItemChanged);
    }

    if (m_sidebar)
    {
        if (multipleSel)
            m_sidebar->SetMultipleSelection();
        else
            m_sidebar->SetSelectedItem(m_catalog, GetCurrentItem()); // may be nullptr
    }

    if (hasTextFocus)
        m_editingArea->SetTextFocus();

    auto references = FileViewer::GetIfExists();
    if (references)
        references->ShowReferences(m_catalog, GetCurrentItem(), 0);
}



void PoeditFrame::OnReferencesMenu(wxCommandEvent&)
{
    auto entry = GetCurrentItem();
    if ( !entry )
        return;
    ShowReference(0);
}

void PoeditFrame::OnReferencesMenuUpdate(wxUpdateUIEvent& event)
{
    OnSingleSelectionUpdate(event);
    if (event.GetEnabled())
    {
        auto item = GetCurrentItem();
        event.Enable(item && !item->GetReferences().empty());
    }
}

void PoeditFrame::OnReference(wxCommandEvent& event)
{
    ShowReference(event.GetId() - ID_POPUP_REFS);
}



void PoeditFrame::ShowReference(int num)
{
    auto entry = GetCurrentItem();
    if (!entry)
        return;
    FileViewer::GetAndActivate()->ShowReferences(m_catalog, entry, num);
}



void PoeditFrame::OnFuzzyFlag(wxCommandEvent&)
{
    bool setFuzzy = GetMenuBar()->IsChecked(XRCID("menu_fuzzy"));

    bool modified = false;

    m_list->ForSelectedCatalogItemsDo([=,&modified](CatalogItem& item){
        if (item.IsFuzzy() != setFuzzy)
        {
            item.SetFuzzy(setFuzzy);
            item.SetModified(true);
            modified = true;
        }
    });

    if (modified && !IsModified())
    {
        m_modified = true;
        UpdateTitle();
    }
    UpdateStatusBar();

    UpdateToTextCtrl(EditingArea::UndoableEdit);

    if (m_list->HasSingleSelection())
    {
        // The user explicitly changed fuzzy status (e.g. to on). Normally, if the
        // user edits an entry, it's fuzzy flag is cleared, but if the user sets
        // fuzzy on to indicate the translation is problematic and then continues
        // editing the entry, we do not want to annoy him by changing fuzzy back on
        // every keystroke.
        m_editingArea->DontAutoclearFuzzyStatus();
    }
}



void PoeditFrame::OnIDsFlag(wxCommandEvent&)
{
    m_displayIDs = GetMenuBar()->IsChecked(XRCID("menu_ids"));
    m_list->SetDisplayLines(m_displayIDs);
}

void PoeditFrame::OnCopyFromSingular(wxCommandEvent&)
{
    m_editingArea->CopyFromSingular();
}

void PoeditFrame::OnCopyFromSource(wxCommandEvent&)
{
    bool modified = false;

    m_list->ForSelectedCatalogItemsDo([&modified](CatalogItem& item){
        item.SetTranslationFromSource();
        if (item.IsModified())
            modified = true;
    });

    if (modified && !IsModified())
    {
        m_modified = true;
        UpdateTitle();
    }
    UpdateStatusBar();

    UpdateToTextCtrl(EditingArea::UndoableEdit);
}

void PoeditFrame::OnClearTranslation(wxCommandEvent&)
{
    bool modified = false;

    m_list->ForSelectedCatalogItemsDo([&modified](CatalogItem& item){
        item.ClearTranslation();
        if (item.IsModified())
            modified = true;
    });

    if (modified && !IsModified())
    {
        m_modified = true;
        UpdateTitle();
    }
    UpdateStatusBar();

    UpdateToTextCtrl(EditingArea::UndoableEdit);
}


void PoeditFrame::OnFind(wxCommandEvent&)
{
    if (!m_findWindow)
        m_findWindow = new FindFrame(this, m_list, m_editingArea, m_catalog);

    m_findWindow->ShowForFind();
}

void PoeditFrame::OnFindAndReplace(wxCommandEvent&)
{
    if (!m_findWindow)
        m_findWindow = new FindFrame(this, m_list, m_editingArea, m_catalog);

    m_findWindow->ShowForReplace();
}

void PoeditFrame::OnFindNext(wxCommandEvent&)
{
    if (m_findWindow)
        m_findWindow->FindNext();
}

void PoeditFrame::OnFindPrev(wxCommandEvent&)
{
    if (m_findWindow)
        m_findWindow->FindPrev();
}

void PoeditFrame::OnUpdateFind(wxUpdateUIEvent& e)
{
    e.Enable(m_catalog && !m_catalog->empty() &&
             m_findWindow && m_findWindow->HasText());
}

CatalogItemPtr PoeditFrame::GetCurrentItem() const
{
    if ( !m_catalog || !m_list )
        return nullptr;
    return m_list->GetCurrentCatalogItem();
}


void PoeditFrame::OnUpdatedFromTextCtrl(CatalogItemPtr item, bool statsChanged)
{
    GetMenuBar()->Check(XRCID("menu_fuzzy"), item->IsFuzzy());

    m_pendingHumanEditedItem = item;

    if (statsChanged)
    {
        UpdateStatusBar();
    }
    // else: no point in recomputing stats

    if (!IsModified())
    {
        m_modified = true;
        UpdateTitle();
    }
}


void PoeditFrame::OnNewTranslationEntered(const CatalogItemPtr& item)
{
    if (item->IsFuzzy() || !item->IsTranslated())
        return;

    if (Config::UseTM())
    {
        auto srclang = m_catalog->GetSourceLanguage();
        auto lang = m_catalog->GetLanguage();
        dispatch::async([=](){
            try
            {
                auto tm = TranslationMemory::Get().GetWriter();
                tm->Insert(srclang, lang, item);
                // Note: do *not* call tm->Commit() here, because Lucene commit is
                // expensive. Instead, wait until the file is saved with committing
                // the changes. This way TM updates are available immediately for use
                // in further translations within the file, but per-item updates
                // remain inexpensive.
            }
            catch (const Exception&)
            {
                // ignore failures here, they'll become apparent when saving the file
            }
        });
    }
}


void PoeditFrame::UpdateToTextCtrl(int flags)
{
    auto item = GetCurrentItem();
    if (!item)
        return;

    m_pendingHumanEditedItem.reset();

    m_editingArea->UpdateToTextCtrl(item, flags);

    GetMenuBar()->Check(XRCID("menu_fuzzy"), item->IsFuzzy());
}



void PoeditFrame::ReadCatalog(const wxString& catalog)
{
    wxBusyCursor bcur;

    // NB: duplicated in PoeditFrame::Create()
    CatalogPtr cat = std::make_shared<Catalog>(catalog);
    if (cat->IsOk())
    {
        ReadCatalog(cat);
    }
    else
    {
        wxMessageDialog dlg
        (
            this,
            _("The file cannot be opened."),
            _("Invalid file"),
            wxOK | wxICON_ERROR
        );
        dlg.SetExtendedMessage(
            _("The file may be either corrupted or in a format not recognized by Poedit.")
        );
        dlg.ShowModal();
    }
}


void PoeditFrame::ReadCatalog(const CatalogPtr& cat)
{
    wxASSERT( cat && cat->IsOk() );

    {
#ifdef __WXMSW__
        wxWindowUpdateLocker no_updates(this);
#endif

        m_catalog = cat;
        m_pendingHumanEditedItem.reset();

        if (m_catalog->empty())
        {
            EnsureContentView(Content::Empty_PO);
        }
        else
        {
            EnsureAppropriateContentView();
            // This must be done as soon as possible, otherwise the list would be
            // confused. GetCurrentItem() could return nullptr or something invalid,
            // causing crash in UpdateToTextCtrl() called from
            // RecreatePluralTextCtrls() just few lines below.
            NotifyCatalogChanged(m_catalog);
        }

        m_fileExistsOnDisk = true;
        m_modified = false;

        RecreatePluralTextCtrls();
        RefreshControls(Refresh_NoCatalogChanged /*done right above*/);
        UpdateTitle();
        UpdateTextLanguage();

        NoteAsRecentFile();

        if (cat->HasCapability(Catalog::Cap::Translations))
            WarnAboutLanguageIssues();
    }

    // Can't do this with the window being frozen, because positioning the toolbar
    // in presence of mCtrl menubar would not size & repaint properly:
#ifdef HAVE_HTTP_CLIENT
    m_toolbar->EnableSyncWithCrowdin(m_catalog->IsFromCrowdin());
#endif

    FixDuplicatesIfPresent();
}

void PoeditFrame::FixDuplicatesIfPresent()
{
    // Poedit always produces good files, so don't bother with it. Older
    // versions would preserve bad files, though.
    wxString generator = m_catalog->Header().GetHeader("X-Generator");
    wxString gversion;
    if (generator.StartsWith("Poedit ", &gversion) &&
            !gversion.StartsWith("1.7") && !gversion.StartsWith("1.6") && !gversion.StartsWith("1.5"))
        return;

    if (!m_catalog->HasDuplicateItems())
        return; // good

    // Fix duplicates and explain the changes to the user:
    m_catalog->FixDuplicateItems();
    NotifyCatalogChanged(m_catalog);

    wxWindowPtr<wxMessageDialog> dlg(
        new wxMessageDialog
            (
                this,
                wxString::Format(_(L"Poedit automatically fixed invalid content in the file “%s”."), wxFileName(GetFileName()).GetFullName()),
                _("Invalid file"),
                wxOK | wxICON_INFORMATION
            )
    );
    dlg->SetExtendedMessage(_("The file contained duplicate items, which is not allowed in PO files and would prevent the file from being used. Poedit fixed the issue, but you should review translations of any items marked as needing work and correct them if necessary."));
    dlg->ShowWindowModalThenDo([dlg](int){});

}

void PoeditFrame::WarnAboutLanguageIssues()
{
    Language srclang = m_catalog->GetSourceLanguage();
    Language lang = m_catalog->GetLanguage();

    if (!lang.IsValid())
    {
        AttentionMessage msg
            (
                "missing-language",
                AttentionMessage::Error,
                _("Language of the translation isn't set.")
            );
        msg.AddAction(MSW_OR_OTHER(_("Set language"), _("Set Language")),
                      [=]{ EditCatalogProperties(); });
        // TRANSLATORS: This is shown underneath "Language of the translation isn't set (or ...is the same as source language)."
        msg.SetExplanation(_("Suggestions are not available if the translation language is not set correctly. Other features, such as plural forms, may be affected as well."));
        m_attentionBar->ShowMessage(msg);
    }

    // Check if the language is set wrongly. This is typically done in such a way that
    // both languages are English, so check explicitly for the common case of "translating"
    // from en to en_US too:
    if (lang.IsValid() && srclang.IsValid() &&
        (lang == srclang || (srclang == Language::English() && lang.Code() == "en_US")))
    {
        AttentionMessage msg
            (
                "same-language-as-source",
                AttentionMessage::Warning,
                _("Language of the translation is the same as source language.")
            );
        msg.SetExplanation(_("Suggestions are not available if the translation language is not set correctly. Other features, such as plural forms, may be affected as well."));
        msg.AddAction(MSW_OR_OTHER(_("Fix language"), _("Fix Language")),
                      [=]{ EditCatalogProperties(); });
        if (lang != srclang)
            msg.AddDontShowAgain(); // possible that Poedit misjudged the intent

        m_attentionBar->ShowMessage(msg);
    }

    // check if plural forms header is correct (only if the language is set,
    // otherwise setting the language will fix this issue too):
    if ( lang.IsValid() && m_catalog->HasPluralItems() )
    {
        wxString err;

        if ( m_catalog->Header().GetHeader("Plural-Forms").empty() )
        {
            err = _("This catalog has entries with plural forms, but doesn't have Plural-Forms header configured.");
        }
        else if ( m_catalog->HasWrongPluralFormsCount() )
        {
            err = _("Entries in this catalog have different plural forms count from what catalog's Plural-Forms header says");
        }

        // FIXME: make this part of global error checking
        wxString plForms = m_catalog->Header().GetHeader("Plural-Forms");
        auto plCalc = PluralFormsCalculator::make(plForms.ToAscii());
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

        if ( !err.empty() )
        {
            AttentionMessage msg
                (
                    "malformed-plural-forms",
                    AttentionMessage::Error,
                    err
                );
            msg.AddAction(MSW_OR_OTHER(_("Fix the header"), _("Fix the Header")),
                          [=]{ EditCatalogProperties(); });

            m_attentionBar->ShowMessage(msg);
        }
        else // no error, check for warning-worthy stuff
        {
            if ( lang.IsValid() )
            {
                // Check for unusual plural forms. Do some normalization to avoid unnecessary
                // complains when the only differences are in whitespace for example.
                wxString pl1 = plForms;
                wxString pl2 = lang.DefaultPluralFormsExpr();
                if (!pl2.empty())
                {
                    pl1.Replace(" ", "");
                    pl2.Replace(" ", "");
                    if ( pl1 != pl2 )
                    {
                        if (pl1.Find(";plural=(") == wxNOT_FOUND && pl1.Last() == ';')
                        {
                            pl1.Replace(";plural=", ";plural=(");
                            pl1.RemoveLast();
                            pl1 += ");";
                        }
                    }

                    if ( pl1 != pl2 )
                    {
                        AttentionMessage msg
                            (
                                "unusual-plural-forms",
                                AttentionMessage::Warning,
                                wxString::Format
                                (
                                    // TRANSLATORS: %s is language name in its basic form (as you
                                    // would see e.g. in a list of supported languages). You may need
                                    // to rephrase it, e.g. to an equivalent of "for language %s".
                                    _("Plural forms expression used by the catalog is unusual for %s."),
                                    lang.DisplayName()
                                )
                            );
                        // TRANSLATORS: A verb, shown as action button with ""Plural forms expression used by the catalog is unusual for %s.")"
                        msg.AddAction(_("Review"), [=]{ EditCatalogProperties(); });
                        msg.AddDontShowAgain();

                        m_attentionBar->ShowMessage(msg);
                    }
                }
            }
        }
    }
}


void PoeditFrame::NoteAsRecentFile()
{
    wxFileName fn(GetFileName());
    fn.Normalize(wxPATH_NORM_DOTS | wxPATH_NORM_ABSOLUTE);
#ifdef __WXOSX__
    [[NSDocumentController sharedDocumentController] noteNewRecentDocumentURL:[NSURL fileURLWithPath:str::to_NS(fn.GetFullPath())]];
#else
    FileHistory().AddFileToHistory(fn.GetFullPath());
#endif
}


void PoeditFrame::MarkAsModified()
{
    m_modified = true;
    UpdateTitle();
}


void PoeditFrame::RefreshControls(int flags)
{
    if (!m_catalog)
        return;

    m_hasObsoleteItems = false;
    if (!m_catalog->IsOk())
    {
        wxLogError(_("Error loading message catalog file '%s'."), m_catalog->GetFileName());
        m_fileExistsOnDisk = false;
        UpdateMenu();
        UpdateTitle();
        m_catalog.reset();
        m_pendingHumanEditedItem.reset();
        NotifyCatalogChanged(nullptr);
        return;
    }

    wxBusyCursor bcur;
    UpdateMenu();

    if (m_list)
    {
        // update catalog view, this may involve reordering the items...
        if (!(flags & Refresh_NoCatalogChanged))
            m_list->CatalogChanged(m_catalog);

        if (m_findWindow)
            m_findWindow->Reset(m_catalog);
    }

    UpdateTitle();
    UpdateStatusBar();
    Refresh();
}


void PoeditFrame::NotifyCatalogChanged(const CatalogPtr& cat)
{
    if (m_sidebar)
        m_sidebar->ResetCatalog();
    if (m_list)
        m_list->CatalogChanged(cat);
}


void PoeditFrame::UpdateStatusBar()
{
    auto bar = GetStatusBar();
    if (m_catalog && bar)
    {
        int all, fuzzy, untranslated, errors, unfinished;
        m_catalog->GetStatistics(&all, &fuzzy, &errors, &untranslated, &unfinished);

        wxString text;
        if (m_catalog->HasCapability(Catalog::Cap::Translations))
        {
            int percent = (all == 0) ? 0 : (100 * (all - unfinished) / all);

            text.Printf(_("Translated: %d of %d (%d %%)"), all - unfinished, all, percent);
            if (unfinished > 0)
            {
                text += L"  •  ";
                text += wxString::Format(_("Remaining: %d"), unfinished);
            }
            if (errors > 0)
            {
                text += L"  •  ";
                text += wxString::Format(wxPLURAL("%d error", "%d errors", errors), errors);
            }
        }
        else
        {
            text.Printf(wxPLURAL("%d entry", "%d entries", all), all);
        }

        bar->SetStatusText(text);
    }
}


void PoeditFrame::UpdateTitle()
{
#ifdef __WXOSX__
    OSXSetModified(IsModified());
#endif

    m_fileNamePartOfTitle.clear();

    wxString title;
    auto fileName = GetFileName();
    if ( !fileName.empty() )
    {
        wxFileName fn(GetFileName());
        wxString fpath = fn.GetFullName();

        if (m_fileExistsOnDisk)
            SetRepresentedFilename(fileName);
        else
            fpath += _(" (unsaved)");

        if ( !m_catalog->Header().Project.empty() )
        {
            title.Printf(
#ifdef __WXOSX__
                L"%s — %s",
#else
                L"%s • %s",
#endif
                fpath, m_catalog->Header().Project);
        }
        else
        {
            title = fn.GetFullName();
        }

        m_fileNamePartOfTitle = title;

#ifndef __WXOSX__
        if ( IsModified() )
            title += _(" (modified)");
        title += " - Poedit";
#endif
    }
    else
    {
        title = "Poedit";
    }

    SetTitle(title);
}



void PoeditFrame::UpdateMenu()
{
    wxMenuBar *menubar = GetMenuBar();

    const bool hasCatalog = m_catalog != nullptr;
    const bool nonEmpty = hasCatalog && !m_catalog->empty();
    const bool editable = nonEmpty && m_catalog->HasCapability(Catalog::Cap::Translations);

    menubar->Enable(XRCID("menu_compile_mo"), hasCatalog && m_catalog->GetFileType() == Catalog::Type::PO);
    menubar->Enable(XRCID("menu_export"), hasCatalog);

    menubar->Enable(XRCID("menu_references"), nonEmpty);
    menubar->Enable(wxID_FIND, nonEmpty);
    menubar->Enable(wxID_REPLACE, nonEmpty);

    menubar->Enable(XRCID("menu_purge_deleted"), editable);
    menubar->Enable(XRCID("menu_validate"), editable);
    menubar->Enable(XRCID("menu_catproperties"), hasCatalog);

    menubar->Enable(XRCID("menu_ids"), nonEmpty);

    menubar->Enable(XRCID("sort_by_order"), nonEmpty);
    menubar->Enable(XRCID("sort_by_source"), nonEmpty);
    menubar->Enable(XRCID("sort_by_translation"), editable);
    menubar->Enable(XRCID("sort_group_by_context"), nonEmpty);
    menubar->Enable(XRCID("sort_untrans_first"), editable);
    menubar->Enable(XRCID("sort_errors_first"), editable);

    if (m_list)
        m_list->Enable(nonEmpty);

    menubar->Enable(XRCID("menu_purge_deleted"),
                    editable && m_catalog->HasDeletedItems());

    for (int i = 0; i < 10; i++)
    {
        menubar->Enable(ID_BOOKMARK_SET + i, editable);
        menubar->Enable(ID_BOOKMARK_GO + i,
                        editable &&
                        m_catalog->GetBookmarkIndex(Bookmark(i)) != -1);
    }
}


void PoeditFrame::WriteCatalog(const wxString& catalog)
{
    WriteCatalog(catalog, [](bool){});
}

template<typename TFunctor>
void PoeditFrame::WriteCatalog(const wxString& catalog, TFunctor completionHandler)
{
    wxBusyCursor bcur;

    dispatch::future<void> tmUpdateThread;
    if (Config::UseTM() && m_catalog->HasCapability(Catalog::Cap::Translations))
    {
        tmUpdateThread = dispatch::async([=]{
            try
            {
                auto tm = TranslationMemory::Get().GetWriter();
                tm->Insert(m_catalog);
                tm->Commit();
            }
            catch ( const Exception& e )
            {
                wxLogWarning(_("Failed to update translation memory: %s"), e.What());
            }
            catch ( ... )
            {
                wxLogWarning(_("Failed to update translation memory: %s"), "unknown error");
            }
        });
    }

    if (m_catalog->GetFileType() == Catalog::Type::PO)
    {
        Catalog::HeaderData& dt = m_catalog->Header();
        dt.Translator = wxConfig::Get()->Read("translator_name", dt.Translator);
        dt.TranslatorEmail = wxConfig::Get()->Read("translator_email", dt.TranslatorEmail);
    }

    int validation_errors = 0;
    Catalog::CompilationStatus mo_compilation_status = Catalog::CompilationStatus::NotDone;
    if ( !m_catalog->Save(catalog, true, validation_errors, mo_compilation_status) )
    {
        if (tmUpdateThread.valid())
            tmUpdateThread.wait();
        completionHandler(false);
        return;
    }

    m_catalog->SetFileName(catalog);
    m_modified = false;
    m_fileExistsOnDisk = true;

#ifndef __WXOSX__
    FileHistory().AddFileToHistory(GetFileName());
#endif

    UpdateTitle();

    RefreshControls();

    NoteAsRecentFile();

    if (ManagerFrame::Get())
        ManagerFrame::Get()->NotifyFileChanged(GetFileName());

    if (tmUpdateThread.valid())
        tmUpdateThread.wait();

    if (validation_errors)
    {
        // Note: this may show window-modal window and because we may
        //       be called from such window too, run this in the next
        //       event loop iteration.
        CallAfter([=]{
            ReportValidationErrors(validation_errors, mo_compilation_status, /*from_save=*/true, /*other_file_saved=*/true, [=]{
                completionHandler(true);
            });
        });
    }
    else
    {
        if (m_catalog->GetCloudSync())
        {
            CloudSyncProgressWindow::RunSync(this, m_catalog->GetCloudSync(), m_catalog);
        }

        completionHandler(true);
    }
}


void PoeditFrame::OnEditComment(wxCommandEvent& event)
{
    auto firstItem = GetCurrentItem();
    wxCHECK_RET( firstItem, "no entry selected" );

    (void)event;
    wxWindow *parent = this;
#ifndef __WXOSX__
    // Find suitable parent window for the comment dialog (e.g. the button):
    parent = dynamic_cast<wxWindow*>(event.GetEventObject());
    if (parent && dynamic_cast<wxToolBar*>(parent) != nullptr)
        parent = nullptr;
    if (!parent)
        parent = this;
#endif

    wxWindowPtr<CommentDialog> dlg(new CommentDialog(parent, firstItem->GetComment()));

    dlg->ShowWindowModalThenDo([=](int retcode){
        if (retcode == wxID_OK)
        {
            m_modified = true;
            UpdateTitle();
            wxString comment = dlg->GetComment();

            bool modified = false;
            m_list->ForSelectedCatalogItemsDo([&modified,comment](CatalogItem& item){
                if (item.GetComment() != comment)
                {
                    item.SetComment(comment);
                    item.SetModified(true);
                    modified = true;
                }
            });
            if (modified && !IsModified())
            {
                m_modified = true;
                UpdateTitle();
            }

            // update comment window
            if (m_sidebar)
                m_sidebar->RefreshContent();
        }
    });
}

void PoeditFrame::OnPurgeDeleted(wxCommandEvent& WXUNUSED(event))
{
    const wxString title =
        _("Purge deleted translations");
    const wxString main =
        _("Do you want to remove all translations that are no longer used?");
    const wxString details =
        _("If you continue with purging, all translations marked as deleted will be permanently removed. You will have to translate them again if they are added back in the future.");

    wxWindowPtr<wxMessageDialog> dlg(new wxMessageDialog(this, main, title, wxYES_NO | wxICON_QUESTION));
    dlg->SetExtendedMessage(details);
    dlg->SetYesNoLabels(_("Purge"), _("Keep"));

    dlg->ShowWindowModalThenDo([this,dlg](int retcode){
        if (retcode == wxID_YES) {
            m_catalog->RemoveDeletedItems();
            m_modified = true;
            UpdateTitle();
            UpdateMenu();
        }
    });
}


void PoeditFrame::OnSuggestion(wxCommandEvent& event)
{
    auto entry = GetCurrentItem();
    if (!entry)
        return;

    entry->SetTranslation(event.GetString());
    entry->SetFuzzy(false);
    entry->SetModified(true);

    // FIXME: instead of this mess, use notifications of catalog change
    m_modified = true;
    UpdateTitle();
    UpdateStatusBar();

    UpdateToTextCtrl(EditingArea::UndoableEdit);
    m_list->RefreshSelectedItems();
}


void PoeditFrame::OnPreTranslateAll(wxCommandEvent&)
{
    PreTranslateWithUI(this, m_list, m_catalog,[=]{
        if (!m_modified)
        {
            m_modified = true;
            UpdateTitle();
        }
        RefreshControls();
    });
}


wxMenu *PoeditFrame::GetPopupMenu(int item)
{
    if (!m_catalog) return NULL;
    if (item < 0 || item >= (int)m_list->GetItemCount()) return NULL;

    const wxArrayString& refs = (*m_catalog)[item]->GetReferences();
    wxMenu *menu = new wxMenu;

    menu->Append(XRCID("menu_copy_from_src"),
                 #ifdef __WXMSW__
                 wxString(_("Copy from source text"))
                 #else
                 wxString(_("Copy from Source Text"))
                 #endif
                   + "\t" + _("Ctrl+") + "B");
    menu->Append(XRCID("menu_clear"),
                 #ifdef __WXMSW__
                 wxString(_("Clear translation"))
                 #else
                 wxString(_("Clear Translation"))
                 #endif
                   + "\t" + _("Ctrl+") + "K");
   menu->Append(XRCID("menu_comment"),
                 #ifdef __WXMSW__
                 wxString(_("Edit comment"))
                 #else
                 wxString(_("Edit Comment"))
                 #endif
                 #ifndef __WXOSX__
                   + "\t" + _("Ctrl+") + "M"
                 #endif
                 );

    if ( !refs.empty() )
    {
        menu->AppendSeparator();

        wxMenuItem *it1 = new wxMenuItem(menu, ID_POPUP_DUMMY+0, _("References:"));
#ifdef __WXMSW__
        it1->SetFont(it1->GetFont().Bold());
        menu->Append(it1);
#else
        menu->Append(it1);
        it1->Enable(false);
#endif

        for (int i = 0; i < (int)refs.GetCount(); i++)
            menu->Append(ID_POPUP_REFS + i, "    " + refs[i]);
    }

    return menu;
}


void PoeditFrame::SetCustomFonts()
{
    if (!m_list)
        return;
    wxConfigBase *cfg = wxConfig::Get();

    static bool prevUseFontText = false;

    bool useFontList = (bool)cfg->Read("custom_font_list_use", (long)false);
    bool useFontText = (bool)cfg->Read("custom_font_text_use", (long)false);

    if (useFontList)
    {
        wxString name = cfg->Read("custom_font_list_name", wxEmptyString);
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
        wxString name = cfg->Read("custom_font_text_name", wxEmptyString);
        if (!name.empty())
        {
            wxNativeFontInfo fi;
            fi.FromString(name);
            wxFont font;
            font.SetNativeFontInfo(fi);
            m_editingArea->SetCustomFont(font);
            prevUseFontText = true;
        }
    }
    else if (prevUseFontText)
    {
        wxFont font(wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT));
        m_editingArea->SetCustomFont(font);
        prevUseFontText = false;
    }
}

void PoeditFrame::OnSize(wxSizeEvent& event)
{
    wxWindowUpdateLocker lock(this);

    event.Skip();

    // see the comment in PoeditFrame ctor
    if ( m_setSashPositionsWhenMaximized && IsMaximized() )
    {
        m_setSashPositionsWhenMaximized = false;

        // update sizes of child windows first:
        Layout();

        // then set sash positions
        if (m_splitter)
            m_splitter->SetSashPosition((int)wxConfig::Get()->ReadLong("/splitter", PX(250)));
    }

    if (m_sidebarSplitter)
    {
        auto split = wxConfigBase::Get()->ReadDouble("/sidebar_splitter", 0.75);
        m_sidebarSplitter->SetSashPosition(split * event.GetSize().x);
    }

    if (m_sidebar && m_splitter)
        m_sidebar->SetUpperHeight(m_splitter->GetSashPosition());
}


void PoeditFrame::RecreatePluralTextCtrls()
{
    if (!m_catalog || !m_editingArea)
        return;

    m_editingArea->RecreatePluralTextCtrls(m_catalog);

    SetCustomFonts();
    UpdateTextLanguage();
    UpdateToTextCtrl(EditingArea::ItemChanged);
}

void PoeditFrame::OnListRightClick(wxDataViewEvent& event)
{
    auto item = event.GetItem();
    if (!item.IsOk())
    {
        event.Skip();
        return;
    }

    m_list->SelectAndFocus(item);

    wxMenu *menu = GetPopupMenu(m_list->ListItemToCatalogIndex(item));
    if (menu)
    {
        m_list->PopupMenu(menu, event.GetPosition());
        delete menu;
    }
    else
    {
        event.Skip();
    }
}

void PoeditFrame::OnListFocus(wxFocusEvent& event)
{
    if (g_focusToText && m_editingArea)
        m_editingArea->SetTextFocus();
    else
        event.Skip();
}

void PoeditFrame::OnSplitterSashMoving(wxSplitterEvent& event)
{
    auto pos = event.GetSashPosition();
    wxConfigBase::Get()->Write("/splitter", (long)pos);
    if (m_sidebar)
        m_sidebar->SetUpperHeight(pos);
}

void PoeditFrame::OnSidebarSplitterSashMoving(wxSplitterEvent& event)
{
    auto split = (double)event.GetSashPosition() / (double)GetSize().x;
    wxConfigBase::Get()->Write("/sidebar_splitter", split);
}

void PoeditFrame::AddBookmarksMenu(wxMenu *parent)
{
    wxMenu *menu = new wxMenu();

    parent->AppendSeparator();
    parent->AppendSubMenu(menu, _("&Bookmarks"));

#ifdef __WXOSX__
    // on Mac, Alt+something is used during normal typing, so we shouldn't
    // use it as shortcuts:
    #define BK_ACCEL_SET  "Ctrl+rawctrl+%i"
    #define BK_ACCEL_GO   "Ctrl+Alt+%i"
#else
    // TRANSLATORS: This is the key shortcut used in menus on Windows, some languages call them differently
    #define BK_ACCEL_SET  _("Alt+") + "%i"
    // TRANSLATORS: This is the key shortcut used in menus on Windows, some languages call them differently
    #define BK_ACCEL_GO   _("Ctrl+") + _("Alt+") + "%i"
#endif

#ifdef __WXMSW__
    #define BK_LABEL_SET  _("Set bookmark %i")
    #define BK_LABEL_GO   _("Go to bookmark %i")
#else
    #define BK_LABEL_SET  _("Set Bookmark %i")
    #define BK_LABEL_GO   _("Go to Bookmark %i")
#endif

    for (int i = 0; i < 10; i++)
    {
        auto label = BK_LABEL_SET + "\t" + BK_ACCEL_SET;
        menu->Append(ID_BOOKMARK_SET + i, wxString::Format(label, i, i));
    }
    menu->AppendSeparator();
    for (int i = 0; i < 10; i++)
    {
        auto label = BK_LABEL_GO + "\t" + BK_ACCEL_GO;
        menu->Append(ID_BOOKMARK_GO + i, wxString::Format(label, i, i));
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
            m_list->SelectAndFocus(listIndex);
        }
    }
}

void PoeditFrame::OnSetBookmark(wxCommandEvent& event)
{
    // Set bookmark if different from the current value for the item,
    // else unset it
    int bkIndex = -1;
    int selItemIndex = m_list->ListItemToCatalogIndex(m_list->GetCurrentItem());
    if (selItemIndex == -1)
        return;

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
    m_list->RefreshSelectedItems();
    if (bkIndex != -1)
        m_list->RefreshItem(m_list->CatalogIndexToListItem(bkIndex));

    // Catalog has been modified
    m_modified = true;
    UpdateTitle();
    UpdateMenu();
}


void PoeditFrame::OnSortByFileOrder(wxCommandEvent&)
{
    m_list->sortOrder().by = SortOrder::By_FileOrder;
    m_list->Sort();
}


void PoeditFrame::OnSortBySource(wxCommandEvent&)
{
    m_list->sortOrder().by = SortOrder::By_Source;
    m_list->Sort();
}


void PoeditFrame::OnSortByTranslation(wxCommandEvent&)
{
    m_list->sortOrder().by = SortOrder::By_Translation;
    m_list->Sort();
}


void PoeditFrame::OnSortGroupByContext(wxCommandEvent& event)
{
    m_list->sortOrder().groupByContext = event.IsChecked();
    m_list->Sort();
}


void PoeditFrame::OnSortUntranslatedFirst(wxCommandEvent& event)
{
    m_list->sortOrder().untransFirst = event.IsChecked();
    m_list->Sort();
}

void PoeditFrame::OnSortErrorsFirst(wxCommandEvent& event)
{
    m_list->sortOrder().errorsFirst = event.IsChecked();
    m_list->Sort();
}


void PoeditFrame::OnShowHideSidebar(wxCommandEvent&)
{
    bool toShow = !m_sidebarSplitter->IsSplit();

    if (toShow)
    {
        auto split = GetSize().x * wxConfigBase::Get()->ReadDouble("/sidebar_splitter", 0.75);
        m_sidebarSplitter->SplitVertically(m_splitter, m_sidebar, split);
        m_sidebar->RefreshContent();
    }
    else
    {
        m_sidebarSplitter->Unsplit(m_sidebar);
    }

    wxConfigBase::Get()->Write("/sidebar_shown", toShow);

}

void PoeditFrame::OnUpdateShowHideSidebar(wxUpdateUIEvent& event)
{
    event.Enable(m_sidebar != nullptr);
    if (!m_sidebar)
        return;

    bool shown = m_sidebarSplitter->IsSplit();
#ifdef __WXOSX__
    auto shortcut = "\tCtrl+Alt+S";
    if (shown)
        event.SetText(_("Hide Sidebar") + shortcut);
    else
        event.SetText(_("Show Sidebar") + shortcut);
#else
    event.Check(shown);
#endif
}

void PoeditFrame::OnShowHideStatusbar(wxCommandEvent&)
{
    auto bar = GetStatusBar();
    bool toShow = (bar == nullptr);

    if (toShow)
    {
        CreateStatusBar(1, wxST_SIZEGRIP);
        UpdateStatusBar();
    }
    else
    {
        SetStatusBar(nullptr);
        bar->Destroy();
    }

    wxConfigBase::Get()->Write("/statusbar_shown", toShow);
}

void PoeditFrame::OnUpdateShowHideStatusbar(wxUpdateUIEvent& event)
{
    bool shown = GetStatusBar() != nullptr;
#ifdef __WXOSX__
    auto shortcut = "\tCtrl+/";
    if (shown)
        event.SetText(_("Hide Status Bar") + shortcut);
    else
        event.SetText(_("Show Status Bar") + shortcut);
#else
    event.Check(shown);
#endif
}


void PoeditFrame::OnSelectionUpdate(wxUpdateUIEvent& event)
{
    event.Enable(m_catalog && m_list && m_list->HasSelection());
}

void PoeditFrame::OnSelectionUpdateEditable(wxUpdateUIEvent& event)
{
    event.Enable(m_catalog && m_list && m_list->HasSelection() &&
                 m_catalog->HasCapability(Catalog::Cap::Translations));
}

void PoeditFrame::OnSingleSelectionUpdate(wxUpdateUIEvent& event)
{
    event.Enable(m_catalog && m_list && m_list->HasSingleSelection());
}

void PoeditFrame::OnSingleSelectionWithPluralsUpdate(wxUpdateUIEvent& event)
{
    // Enable only if a single item with plural forms is selected
    event.Enable(m_catalog &&
                 m_list && m_list->HasSingleSelection() &&
                 m_editingArea->IsShowingPlurals());
}

void PoeditFrame::OnHasCatalogUpdate(wxUpdateUIEvent& event)
{
    event.Enable(m_catalog != nullptr);
}

void PoeditFrame::OnIsEditableUpdate(wxUpdateUIEvent& event)
{
    event.Enable(m_catalog && !m_catalog->empty() &&
                 m_catalog->HasCapability(Catalog::Cap::Translations));
}

void PoeditFrame::OnEditCommentUpdate(wxUpdateUIEvent& event)
{
    event.Enable(m_catalog && m_list && m_list->HasSelection() &&
                 m_catalog->HasCapability(Catalog::Cap::UserComments));
}

#if defined(__WXMSW__) || defined(__WXGTK__)
// Emulate something like macOS's first responder: pass text editing commands to
// the focused text control.
void PoeditFrame::OnTextEditingCommand(wxCommandEvent& event)
{
#ifdef __WXGTK__
    wxEventBlocker block(this, wxEVT_MENU);
#endif
    wxWindow *w = wxWindow::FindFocus();
    if (!w || w == this || !w->ProcessWindowEventLocally(event))
        event.Skip();
}

void PoeditFrame::OnTextEditingCommandUpdate(wxUpdateUIEvent& event)
{
#ifdef __WXGTK__
    wxEventBlocker block(this, wxEVT_UPDATE_UI);
#endif
    wxWindow *w = wxWindow::FindFocus();
    if (!w || w == this || !w->ProcessWindowEventLocally(event))
        event.Enable(false);
}
#endif // __WXMSW__ || __WXGTK__

// ------------------------------------------------------------------
//  catalog navigation
// ------------------------------------------------------------------

namespace
{

bool Pred_AnyItem(const CatalogItemPtr&)
{
    return true;
}

bool Pred_UnfinishedItem(const CatalogItemPtr& item)
{
    return !item->IsTranslated() ||
           item->IsFuzzy() ||
           item->HasIssue();
}

} // anonymous namespace

int PoeditFrame::NavigateGetNextItem(const int start,
                                      int step, PoeditFrame::NavigatePredicate predicate, bool wrap,
                                      CatalogItemPtr *out_item)
{
    const int count = m_list ? m_list->GetItemCount() : 0;
    if ( !count )
        return -1;

    int i = start;

    for ( ;; )
    {
        i += step;

        if ( i < 0 )
        {
            if ( wrap )
                i += count;
            else
                return -1; // nowhere to go
        }
        else if ( i >= count )
        {
            if ( wrap )
                i -= count;
            else
                return -1; // nowhere to go
        }

        if ( i == start )
            return -1; // nowhere to go

        auto item = m_list->ListIndexToCatalogItem(i);
        if ( predicate(item) )
        {
            if (out_item)
                *out_item = item;
            return i;
        }
    }
}

void PoeditFrame::Navigate(int step, NavigatePredicate predicate, bool wrap)
{
    if (!m_list)
        return;
    auto i = NavigateGetNextItem(m_list->GetCurrentItemListIndex(), step, predicate, wrap, nullptr);
    if (i == -1)
        return;

    m_list->SelectAndFocus(i);
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
    auto item = GetCurrentItem();
    if (!item)
        return;

    // If the user is "done" with an item, it should be in its final approved state
    // (unless they _just_ marked it as fuzzy now):
    if (item->IsFuzzy() && !m_editingArea->ShouldNotAutoclearFuzzyStatus())
    {
        item->SetFuzzy(false);
        item->SetPreTranslated(false);
        item->SetModified(true);
        if (!IsModified())
        {
            m_modified = true;
            UpdateTitle();
            UpdateStatusBar();
        }

        // do additional processing of finished translations, such as adding it to the TM:
        m_pendingHumanEditedItem = item;
    }

    // like "next unfinished", but wraps
    Navigate(+1, Pred_UnfinishedItem, /*wrap=*/true);
}

void PoeditFrame::OnPrevPage(wxCommandEvent&)
{
    if (!m_list)
        return;
    auto pos = std::max(m_list->GetCurrentItemListIndex()-10, 0);
    m_list->SelectAndFocus(pos);
}

void PoeditFrame::OnNextPage(wxCommandEvent&)
{
    if (!m_list)
        return;
    auto pos = std::min(m_list->GetCurrentItemListIndex()+10, (int)m_list->GetItemCount()-1);
    m_list->SelectAndFocus(pos);
}

void PoeditFrame::OnPrevPluralForm(wxCommandEvent&)
{
    m_editingArea->ChangeFocusedPluralTab(-1);
}

void PoeditFrame::OnNextPluralForm(wxCommandEvent&)
{
    m_editingArea->ChangeFocusedPluralTab(+1);
}
