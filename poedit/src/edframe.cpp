
/*

    poedit, a wxWindows i18n catalogs editor

    ---------------
      edframe.cpp
    
      Editor frame
    
      (c) Vaclav Slavik, 2000-2005

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

#if USE_SPELLCHECKING
    #include <gtk/gtk.h>
    extern "C" {
    #include <gtkspell/gtkspell.h>
    }
#endif

#include <map>

#include "catalog.h"
#include "edapp.h"
#include "edframe.h"
#include "settingsdlg.h"
#include "prefsdlg.h"
#include "fileviewer.h"
#include "findframe.h"
#include "transmem.h"
#include "isocodes.h"
#include "progressinfo.h"
#include "commentdlg.h"
#include "manager.h"
#include "pluralforms/pl_evaluate.h"

#include <wx/listimpl.cpp>
WX_DEFINE_LIST(poEditFramesList);
poEditFramesList poEditFrame::ms_instances;



#ifdef __VISUALC__
// Disabling the useless and annoying MSVC++'s
// warning C4800: 'long' : forcing value to bool 'true' or 'false'
// (performance warning):
#pragma warning ( disable : 4800 )
#endif


/*static*/ poEditFrame *poEditFrame::Find(const wxString& filename)
{
    for (poEditFramesList::Node *n = ms_instances.GetFirst(); n; n = n->GetNext())
    {
        if (n->GetData()->m_fileName == filename)
            return n->GetData();
    }
    return NULL;
}

/*static*/ poEditFrame *poEditFrame::Create(const wxString& filename)
{
    poEditFrame *f;
    if (!filename)
    {
        f = new poEditFrame;
    }
    else
    {
        f = poEditFrame::Find(filename);
        if (!f)
        {
            f = new poEditFrame();
            f->Show(true);
            f->ReadCatalog(filename);
        }
    }
    f->Show(true);
    return f;
}


// Event & controls IDs:
enum 
{
    EDC_LIST = 1000,
    EDC_TEXTORIG,
    EDC_TEXTORIGPLURAL,
    EDC_TEXTTRANS,
    EDC_TEXTCOMMENT,
    
    ED_POPUP_REFS = 2000, 
    ED_POPUP_TRANS = 3000,
    ED_POPUP_DUMMY = 4000
};

class ListHandler;
class TextctrlHandler : public wxEvtHandler
{ 
    public:
        TextctrlHandler(poEditFrame* frame) :
                 wxEvtHandler(), m_frame(frame), m_list(frame->m_list), m_sel(&(frame->m_sel)) {}
                 
        void SetCatalog(Catalog* catalog) {m_catalog = catalog;}

    private:
        void OnKeyDown(wxKeyEvent& event)
        {
#if wxCHECK_VERSION(2,5,1)
            int keyCode = event.GetKeyCode();
#else
            int keyCode = event.KeyCode();
#endif

            switch (keyCode)
            {
                case WXK_UP:
                    if ((*m_sel > 0) && event.ControlDown())
                    {
                        m_list->EnsureVisible(*m_sel - 1);
                        m_list->SetItemState(*m_sel - 1, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
                    }
                    else
                        event.Skip();
                    break;
                case WXK_DOWN:
                    if ((*m_sel < m_list->GetItemCount() - 1) && event.ControlDown())
                    {
                        m_list->EnsureVisible(*m_sel + 1);
                        m_list->SetItemState(*m_sel + 1, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
                    }
                    else
                        event.Skip();
                    break;
                case WXK_PRIOR:
                    if (event.ControlDown())
                    {
                        int newy = *m_sel - 10;
                        if (newy < 0) newy = 0;
                        m_list->EnsureVisible(newy);
                        m_list->SetItemState(newy, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
                    }
                    else
                        event.Skip();
                    break;
                case WXK_NEXT:
                    if (event.ControlDown())
                    {
                        int newy = *m_sel + 10;
                        if (newy >= m_list->GetItemCount()) 
                            newy = m_list->GetItemCount() - 1;
                        m_list->EnsureVisible(newy);
                        m_list->SetItemState(newy, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
                    }
                    else
                        event.Skip();
                    break;
                case '0':
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                case '8':
                case '9':
                    if (m_catalog != NULL)
                    {
                        if (event.AltDown() && !event.ControlDown())
                        {
                            // Set bookmark if different from the current value for the item,
                            // else unset it
                            int bkIndex = -1;
                            int selItemIndex = m_list->GetItemData(*m_sel);
                            Bookmark bk = static_cast<Bookmark>(keyCode-'0');
                            if (m_catalog->GetBookmarkIndex(bk) == selItemIndex)
                                m_catalog->SetBookmark(selItemIndex, NO_BOOKMARK);
                            else
                                bkIndex = m_catalog->SetBookmark(selItemIndex, bk);
                            
                            // Refresh items
                            m_list->RefreshItem(*m_sel);
                            if (bkIndex > -1)
                                m_list->RefreshItem(m_list->GetItemIndex(bkIndex));
                                
                            // Catalog has been modified
                            m_frame->m_modified = true;
                            m_frame->UpdateTitle();
                        }
                        else if (event.ControlDown() && !event.AltDown())
                        {
                            // Go to bookmark, if there is an item for it
                            Bookmark bk = static_cast<Bookmark>(keyCode-'0');
                            int bkIndex = m_catalog->GetBookmarkIndex(bk);
                            if (bkIndex > -1)
                            {
                                int listIndex = m_list->GetItemIndex(bkIndex);
                                if (listIndex >= 0 && listIndex < m_list->GetItemCount())
                                {
                                    m_list->EnsureVisible(listIndex);
                                    m_list->SetItemState(listIndex, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
                                }
                            }    
                        }
                        else
                            event.Skip();
                    }           
                    else
                        event.Skip();
                    break;
                default:
                    event.Skip();
            }
        }

        DECLARE_EVENT_TABLE() 

        poEditFrame    *m_frame;
        poEditListCtrl *m_list;
        int            *m_sel;
        Catalog        *m_catalog;
        
        friend class ListHandler;
};

BEGIN_EVENT_TABLE(TextctrlHandler, wxEvtHandler)
   EVT_KEY_DOWN(TextctrlHandler::OnKeyDown)
END_EVENT_TABLE()

// I don't like this global flag, but all poEditFrame instances should share it :(
bool gs_focusToText = false;

// special handling of events in listctrl 
class ListHandler : public wxEvtHandler
{ 
    public:
        ListHandler(poEditFrame *frame) :
                 wxEvtHandler(), m_frame(frame) {}

    private:
        void OnSel(wxListEvent& event) { m_frame->OnListSel(event); }
        void OnDesel(wxListEvent& event) { m_frame->OnListDesel(event); }
        void OnActivated(wxListEvent& event) { m_frame->OnListActivated(event); }
        void OnRightClick(wxMouseEvent& event) { m_frame->OnListRightClick(event); }
        void OnFocus(wxFocusEvent& event) { m_frame->OnListFocus(event); }
        void OnKeyDown(wxKeyEvent& event) { dynamic_cast<TextctrlHandler*>(m_frame->m_textTrans->GetEventHandler())->OnKeyDown(event); }

        DECLARE_EVENT_TABLE() 

        poEditFrame *m_frame;
};

BEGIN_EVENT_TABLE(ListHandler, wxEvtHandler)
   EVT_LIST_ITEM_SELECTED  (EDC_LIST, ListHandler::OnSel)
   EVT_LIST_ITEM_DESELECTED(EDC_LIST, ListHandler::OnDesel)
   EVT_LIST_ITEM_ACTIVATED (EDC_LIST, ListHandler::OnActivated)
   EVT_RIGHT_DOWN          (          ListHandler::OnRightClick)
   EVT_SET_FOCUS           (          ListHandler::OnFocus)
   EVT_KEY_DOWN            (          ListHandler::OnKeyDown)
END_EVENT_TABLE()

class StatusbarHandler : public wxEvtHandler
{ 
    public:
        StatusbarHandler(wxStatusBar *bar, wxGauge *gauge) :
                 m_bar(bar), m_gauge(gauge) {}

    private:
        void OnSize(wxSizeEvent& event)
        {
            wxRect rect;
            m_bar->GetFieldRect(1, rect);
            m_gauge->SetSize(rect.x+2, rect.y+2, rect.width-4, rect.height-4);
            event.Skip();
        }

        DECLARE_EVENT_TABLE() 

        wxStatusBar *m_bar;
        wxGauge     *m_gauge;
};

BEGIN_EVENT_TABLE(StatusbarHandler, wxEvtHandler)
   EVT_SIZE(StatusbarHandler::OnSize)
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
        virtual bool AcceptsFocus() const { return FALSE; }
};


BEGIN_EVENT_TABLE(poEditFrame, wxFrame)
   EVT_MENU           (XRCID("menu_quit"),        poEditFrame::OnQuit)
   EVT_MENU           (XRCID("menu_help"),        poEditFrame::OnHelp)
   EVT_MENU           (XRCID("menu_help_gettext"),poEditFrame::OnHelpGettext)
   EVT_MENU           (XRCID("menu_about"),       poEditFrame::OnAbout)
   EVT_MENU           (XRCID("menu_new"),         poEditFrame::OnNew)
   EVT_MENU           (XRCID("menu_new_from_pot"),poEditFrame::OnNew)
   EVT_MENU           (XRCID("menu_open"),        poEditFrame::OnOpen)
   EVT_MENU           (XRCID("menu_save"),        poEditFrame::OnSave)
   EVT_MENU           (XRCID("menu_saveas"),      poEditFrame::OnSaveAs)
   EVT_MENU           (XRCID("menu_export"),      poEditFrame::OnExport)
   EVT_MENU_RANGE     (wxID_FILE1, wxID_FILE9,    poEditFrame::OnOpenHist)
   EVT_MENU           (XRCID("menu_catsettings"), poEditFrame::OnSettings)
   EVT_MENU           (XRCID("menu_preferences"), poEditFrame::OnPreferences)
   EVT_MENU           (XRCID("menu_update"),      poEditFrame::OnUpdate)
   EVT_MENU           (XRCID("menu_update_from_pot"),poEditFrame::OnUpdate)
   EVT_MENU           (XRCID("menu_purge_deleted"), poEditFrame::OnPurgeDeleted)
   EVT_MENU           (XRCID("menu_fuzzy"),       poEditFrame::OnFuzzyFlag)
   EVT_MENU           (XRCID("menu_quotes"),      poEditFrame::OnQuotesFlag)
   EVT_MENU           (XRCID("menu_lines"),       poEditFrame::OnLinesFlag)
   EVT_MENU           (XRCID("menu_comment_win"), poEditFrame::OnCommentWinFlag)
   EVT_MENU           (XRCID("menu_auto_comments_win"), poEditFrame::OnAutoCommentsWinFlag)
   EVT_MENU           (XRCID("menu_shaded"),      poEditFrame::OnShadedListFlag)
   EVT_MENU           (XRCID("menu_insert_orig"), poEditFrame::OnInsertOriginal)
   EVT_MENU           (XRCID("menu_references"),  poEditFrame::OnReferencesMenu)
   EVT_MENU           (XRCID("menu_fullscreen"),  poEditFrame::OnFullscreen) 
   EVT_MENU           (XRCID("menu_find"),        poEditFrame::OnFind)
   EVT_MENU           (XRCID("menu_comment"),     poEditFrame::OnEditComment)
   EVT_MENU           (XRCID("menu_manager"),     poEditFrame::OnManager)
   EVT_MENU_RANGE     (ED_POPUP_REFS, ED_POPUP_REFS + 999, poEditFrame::OnReference)
#ifdef USE_TRANSMEM
   EVT_MENU_RANGE     (ED_POPUP_TRANS, ED_POPUP_TRANS + 999,
                       poEditFrame::OnAutoTranslate)
   EVT_MENU           (XRCID("menu_auto_translate"), poEditFrame::OnAutoTranslateAll)
#endif
   EVT_CLOSE          (                poEditFrame::OnCloseWindow)
   EVT_TEXT           (EDC_TEXTCOMMENT,poEditFrame::OnCommentWindowText)
#ifdef __WXMSW__
   EVT_DROP_FILES     (poEditFrame::OnFileDrop)
#endif
   EVT_IDLE           (poEditFrame::OnIdle)
   EVT_END_PROCESS    (-1, poEditFrame::OnEndProcess)
END_EVENT_TABLE()


// Frame class:

poEditFrame::poEditFrame() :
    wxFrame(NULL, -1, _("poEdit"), wxDefaultPosition,
                             wxSize(
                                 wxConfig::Get()->Read(_T("frame_w"), 600),
                                 wxConfig::Get()->Read(_T("frame_h"), 400)),
                             wxDEFAULT_FRAME_STYLE | wxNO_FULL_REPAINT_ON_RESIZE),
    m_itemBeingValidated(-1),
    m_catalog(NULL), 
#ifdef USE_TRANSMEM
    m_transMem(NULL),
    m_transMemLoaded(false),
#endif    
    m_list(NULL),
    m_modified(false),
    m_hasObsoleteItems(false),
    m_sel(-1), //m_selItem(0),
    m_edittedTextFuzzyChanged(false)
{
#ifdef __WXMSW__
    const int SPLITTER_BORDER = wxSP_NOBORDER;
#else
    const int SPLITTER_BORDER = wxSP_3DBORDER;
#endif
    
    wxConfigBase *cfg = wxConfig::Get();
    
    // VS: a dirty hack of sort -- if this is the only poEdit frame opened,
    //     place it at remembered position, but don't do that if there already
    //     are other frames, because they would overlap and nobody could
    //     recognize that there are many of them
    if (ms_instances.GetCount() == 0)
        Move(cfg->Read(_T("frame_x"), -1), cfg->Read(_T("frame_y"), -1));
 
    m_displayQuotes = (bool)cfg->Read(_T("display_quotes"), (long)false);
    m_displayLines = (bool)cfg->Read(_T("display_lines"), (long)false);
    m_displayCommentWin = 
        (bool)cfg->Read(_T("display_comment_win"), (long)true);
    m_displayAutoCommentsWin = 
        (bool)cfg->Read(_T("display_auto_comments_win"), (long)true);
    m_commentWindowEditable = 
        (bool)cfg->Read(_T("comment_window_editable"), (long)false);
    gs_focusToText = (bool)cfg->Read(_T("focus_to_text"), (long)false);
    gs_shadedList = (bool)cfg->Read(_T("shaded_list"), (long)true);

#ifdef __UNIX__
    SetIcon(wxArtProvider::GetIcon(_T("poedit-appicon")));
#else
    SetIcon(wxICON(appicon));
#endif

#ifdef CAN_MODIFY_DEFAULT_FONT
    m_boldGuiFont = wxSystemSettings::GetSystemFont(wxSYS_DEFAULT_GUI_FONT);
    m_boldGuiFont.SetWeight(wxFONTWEIGHT_BOLD);
#endif   
    
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
    }
    else
    {
        wxLogError(_T("Cannot load main menu from resource, something must have went terribly wrong."));
        wxLog::FlushActive();
    }

    SetToolBar(wxXmlResource::Get()->LoadToolBar(this, _T("toolbar")));

    GetToolBar()->ToggleTool(XRCID("menu_quotes"), m_displayQuotes);
    GetMenuBar()->Check(XRCID("menu_quotes"), m_displayQuotes);
    GetMenuBar()->Check(XRCID("menu_lines"), m_displayLines);
    GetMenuBar()->Check(XRCID("menu_comment_win"), m_displayCommentWin);
    GetMenuBar()->Check(XRCID("menu_auto_comments_win"), m_displayAutoCommentsWin);
    GetMenuBar()->Check(XRCID("menu_shaded"), gs_shadedList);
    
	m_splitter = new wxSplitterWindow(this, -1,
                                      wxDefaultPosition, wxDefaultSize,
                                      SPLITTER_BORDER);

    m_list = new poEditListCtrl(m_splitter, 
                                EDC_LIST, 
                                wxDefaultPosition, wxDefaultSize,
                                wxLC_REPORT | wxLC_SINGLE_SEL,
                                m_displayLines);

    m_bottomSplitter = new wxSplitterWindow(m_splitter, -1,
                                            wxDefaultPosition, wxDefaultSize,
                                            SPLITTER_BORDER);    
    m_bottomLeftPanel = new wxPanel(m_bottomSplitter);
    m_bottomRightPanel = new wxPanel(m_bottomSplitter);

    m_textComment = NULL;
    m_textAutoComments = new UnfocusableTextCtrl(m_bottomRightPanel,
                                EDC_TEXTORIG, wxEmptyString, 
                                wxDefaultPosition, wxDefaultSize, 
                                wxTE_MULTILINE | wxTE_READONLY);
    // This call will force the creation of the right kind of control
    // for the m_textComment member
    UpdateCommentWindowEditable();
    
    m_labelSingular = new wxStaticText(m_bottomLeftPanel, -1, _("Singular:"));
    m_labelPlural = new wxStaticText(m_bottomLeftPanel, -1, _("Plural:"));
    m_textOrig = new UnfocusableTextCtrl(m_bottomLeftPanel,
                                EDC_TEXTORIG, wxEmptyString, 
                                wxDefaultPosition, wxDefaultSize, 
                                wxTE_MULTILINE | wxTE_READONLY);
    m_textOrigPlural = new UnfocusableTextCtrl(m_bottomLeftPanel,
                                EDC_TEXTORIGPLURAL, wxEmptyString, 
                                wxDefaultPosition, wxDefaultSize, 
                                wxTE_MULTILINE | wxTE_READONLY);
    
    m_textTrans = new wxTextCtrl(m_bottomLeftPanel,
                                EDC_TEXTTRANS, wxEmptyString, 
                                wxDefaultPosition, wxDefaultSize, 
                                wxTE_MULTILINE);

    m_pluralNotebook = new wxNotebook(m_bottomLeftPanel, -1);

    SetCustomFonts();
    
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
    leftSizer->Add(gridSizer, 1, wxEXPAND);
    leftSizer->Add(m_textTrans, 1, wxEXPAND);
    leftSizer->Add(m_pluralNotebook, 1, wxEXPAND);
    rightSizer->Add(m_textAutoComments, 1, wxEXPAND);
    rightSizer->Add(m_textComment, 1, wxEXPAND);

    m_bottomLeftPanel->SetAutoLayout(true);
    m_bottomLeftPanel->SetSizer(leftSizer);

    m_bottomRightPanel->SetAutoLayout(true);
    m_bottomRightPanel->SetSizer(rightSizer);
    
    m_bottomSplitter->SetMinimumPaneSize(40);
    if (m_displayCommentWin || m_displayAutoCommentsWin)
    {
        m_bottomSplitter->SplitVertically(m_bottomLeftPanel, m_bottomRightPanel,
                                          cfg->Read(_T("bottom_splitter"),
                                                    -200L));
        m_bottomLeftPanel->GetSizer()->Show(m_textComment, m_displayCommentWin);
        m_bottomLeftPanel->GetSizer()->Show(m_textAutoComments, m_displayAutoCommentsWin);
        m_bottomLeftPanel->GetSizer()->Layout();
    }
    else
    {
        m_bottomRightPanel->Show(false);
        m_bottomSplitter->Initialize(m_bottomLeftPanel);
    }

    m_splitter->SetMinimumPaneSize(40);
    m_splitter->SplitHorizontally(m_list, m_bottomSplitter, cfg->Read(_T("splitter"), 240L));

    m_list->PushEventHandler(new ListHandler(this));
    m_textTrans->PushEventHandler(new TextctrlHandler(this));
    m_textComment->PushEventHandler(new TextctrlHandler(this));

    m_list->SetFocus();

    int widths[] = {-1, 200};
    CreateStatusBar(2, wxST_SIZEGRIP);
    wxStatusBar *bar = GetStatusBar();
    m_statusGauge = new wxGauge(bar, -1, 100, wxDefaultPosition, wxDefaultSize, wxGA_SMOOTH);
    bar->SetStatusWidths(2, widths);
    bar->PushEventHandler(new StatusbarHandler(bar, m_statusGauge));
#ifdef __WXMSW__    
    bar->SetSize(-1,-1,-1,-1);
#endif

    ShowPluralFormUI(false);

    UpdateMenu();

    wxString cannon = wxGetApp().GetLocale().GetCanonicalName().Left(2);
    wxString datadir = wxGetApp().GetAppPath() + _T("/share/poedit");
    wxString book;
#if defined(__WXMSW__)
    book.Printf(_T("%s/poedit-%s.chm"), datadir.c_str(), cannon.c_str());
    if (!wxFileExists(book))
        book.Printf(_T("%s/poedit.chm"), datadir.c_str());
    m_help.Initialize(book);
    m_helpBook = book;
#elif defined(__UNIX__)
    book.Printf(_T("%s/help-%s.zip"), datadir.c_str(), cannon.c_str());
    if (!wxFileExists(book))
        book.Printf(_T("%s/help.zip"), datadir.c_str());
    m_help.Initialize(book);
    m_help.AddBook(datadir + _T("/help-gettext.zip"));    
#endif

    ms_instances.Append(this);

#ifdef __WXMSW__
    DragAcceptFiles(true);
#endif
}



poEditFrame::~poEditFrame()
{
    ms_instances.DeleteObject(this);

    GetStatusBar()->PopEventHandler(true/*delete*/);
    m_textTrans->PopEventHandler(true/*delete*/);
    m_list->PopEventHandler(true/*delete*/);

    wxSize sz = GetSize();
    wxPoint pos = GetPosition();
    wxConfigBase *cfg = wxConfig::Get();
    if (!IsIconized())
    {
        cfg->Write(_T("frame_w"), (long)sz.x);
        cfg->Write(_T("frame_h"), (long)sz.y);
        cfg->Write(_T("frame_x"), (long)pos.x);
        cfg->Write(_T("frame_y"), (long)pos.y);
    }
    if (m_displayCommentWin)
        cfg->Write(_T("bottom_splitter"),
                   (long)m_bottomSplitter->GetSashPosition());
    cfg->Write(_T("splitter"), (long)m_splitter->GetSashPosition());
    cfg->Write(_T("display_quotes"), m_displayQuotes);
    cfg->Write(_T("display_lines"), m_displayLines);
    cfg->Write(_T("display_comment_win"), m_displayCommentWin);
    cfg->Write(_T("display_auto_comments_win"), m_displayAutoCommentsWin);
    cfg->Write(_T("shaded_list"), gs_shadedList);

    m_history.Save(*cfg);

#ifdef USE_TRANSMEM    
    if (m_transMem)
        m_transMem->Release();
#endif
    delete m_catalog;

    m_catalog = NULL;

    // shutdown the spellchecker:
    InitSpellchecker();
}

#if USE_SPELLCHECKING
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
}

static void DoInitSpellchecker(wxTextCtrl *text,
                               bool enable, const wxString& lang)
{
    GtkTextView *textview = GetTextView(text);
    wxASSERT_MSG( textview, _T("wxTextCtrl is supposed to use GtkTextView") );
    GtkSpell *spell = gtkspell_get_from_text_view(textview);

    if (spell)        
        gtkspell_detach(spell);

    if (enable)
    {
        // map of languages we know don't work, so that we don't bother
        // the user with error messages every time the spell checker
        // is re-initialized:
        static std::map<wxString, bool> brokenLangs;
        
        if (brokenLangs.find(lang) == brokenLangs.end())
        {        
            GError *err = NULL;
            if (!gtkspell_new_attach(textview, lang.ToAscii(), &err))
            {
                wxLogError(_("Error initializing spell checking: %s"),
                           wxString(err->message, wxConvUTF8).c_str());
                g_error_free(err);
                
                // remember that this language is broken:
                brokenLangs[lang] = true;
            }
        }
        // else silently not use the spellchecker
    }
}
#endif // USE_SPELLCHECKING

void poEditFrame::InitSpellchecker()
{
#if USE_SPELLCHECKING
    wxString lang;
    if (m_catalog) lang = m_catalog->GetLocaleCode();
    bool enabled = m_catalog && !lang.empty() &&
                   wxConfig::Get()->Read(_T("enable_spellchecking"),
                                         (long)true);

    DoInitSpellchecker(m_textTrans, enabled, lang);
    for (size_t i = 0; i < m_textTransPlural.size(); i++)
        DoInitSpellchecker(m_textTransPlural[i], enabled, lang);
#endif
}
    


#ifdef USE_TRANSMEM
TranslationMemory *poEditFrame::GetTransMem()
{
    wxConfigBase *cfg = wxConfig::Get();

    if (m_transMemLoaded)
        return m_transMem;
    else
    {
        wxString lang;
        wxString dbPath = cfg->Read(_T("TM/database_path"), wxEmptyString);
       
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

        if (!lang.IsEmpty() && TranslationMemory::IsSupported(lang, dbPath))
        {
            m_transMem = TranslationMemory::Create(lang, dbPath);
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



void poEditFrame::OnQuit(wxCommandEvent&)
{
    Close(true);
}



void poEditFrame::OnCloseWindow(wxCloseEvent&)
{
    UpdateFromTextCtrl();
    if (m_catalog && m_modified)
    {
        int r =
            wxMessageBox(_("Catalog modified. Do you want to save changes?"),
                         _("Save changes"), 
                         wxYES_NO | wxCANCEL | wxCENTRE | wxICON_QUESTION);
        if (r == wxYES)
            WriteCatalog(m_fileName);
        else if (r == wxCANCEL)
            return;
    }
    Destroy();
}



void poEditFrame::OnOpen(wxCommandEvent&)
{
    UpdateFromTextCtrl();
    if (m_catalog && m_modified)
    {
        int r = 
            wxMessageBox(_("Catalog modified. Do you want to save changes?"),
                         _("Save changes"), 
                         wxYES_NO | wxCANCEL | wxCENTRE | wxICON_QUESTION);
        if (r == wxYES)
            WriteCatalog(m_fileName);
        else if (r == wxCANCEL)
            return;
    }

    wxString path = wxPathOnly(m_fileName);
    if (path.empty()) 
        path = wxConfig::Get()->Read(_T("last_file_path"), wxEmptyString);
    
    wxString name = wxFileSelector(_("Open catalog"), 
                    path, wxEmptyString, wxEmptyString, 
                    _("GNU GetText catalogs (*.po)|*.po|All files (*.*)|*.*"), 
                    wxOPEN | wxFILE_MUST_EXIST, this);
    if (!name.IsEmpty())
    {
        wxConfig::Get()->Write(_T("last_file_path"), wxPathOnly(name));
        ReadCatalog(name);
    }
}



void poEditFrame::OnOpenHist(wxCommandEvent& event)
{
    UpdateFromTextCtrl();
    if (m_catalog && m_modified)
    {
        int r = 
            wxMessageBox(_("Catalog modified. Do you want to save changes?"),
                         _("Save changes"), 
                         wxYES_NO | wxCANCEL | wxCENTRE | wxICON_QUESTION);
        if (r == wxYES)
            WriteCatalog(m_fileName);
        else if (r == wxCANCEL)
            return;
    }

    wxString f(m_history.GetHistoryFile(event.GetId() - wxID_FILE1));
    if (f != wxEmptyString && wxFileExists(f))
        ReadCatalog(f);
    else
        wxLogError(_("File '%s' doesn't exist."), f.c_str());
}
        


#ifdef __WXMSW__
void poEditFrame::OnFileDrop(wxDropFilesEvent& event)
{
    if (event.GetNumberOfFiles() != 1)
    {
        wxLogError(_("You can't drop more than one file on poEdit window."));
        return;
    }
    
    wxFileName f(event.GetFiles()[0]);
    if (f.GetExt().Lower() != _T("po"))
    {
        wxLogError(_("File '%s' is not message catalog."),
                   f.GetFullPath().c_str());
        return; 
    }
    
    if (f.FileExists())
    {
        UpdateFromTextCtrl();
        if (m_catalog && m_modified)
        {
            int r =
              wxMessageBox(_("Catalog modified. Do you want to save changes?"),
                           _("Save changes"), 
                           wxYES_NO | wxCANCEL | wxCENTRE | wxICON_QUESTION);
            if (r == wxYES)
                WriteCatalog(m_fileName);
            else if (r == wxCANCEL)
                return;
        }
        ReadCatalog(f.GetFullPath());
    }
    else
        wxLogError(_("File '%s' doesn't exist."), f.GetFullPath().c_str());
}
#endif



void poEditFrame::OnSave(wxCommandEvent& event)
{
    UpdateFromTextCtrl();
    if (m_fileName.IsEmpty())
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

void poEditFrame::OnSaveAs(wxCommandEvent&)
{
    UpdateFromTextCtrl();

    wxString name(wxFileNameFromPath(m_fileName));
    wxString path(wxPathOnly(m_fileName));

    if (name.empty())
    {
        path = wxConfig::Get()->Read(_T("last_file_path"), wxEmptyString);
        name = SuggestFileName(m_catalog) + _T(".po");
    }
    
    name = wxFileSelector(_("Save as..."), path, name, wxEmptyString, 
                          _("GNU GetText catalogs (*.po)|*.po|All files (*.*)|*.*"),
                          wxSAVE | wxOVERWRITE_PROMPT, this);
    if (!name.IsEmpty())
    {
        wxConfig::Get()->Write(_T("last_file_path"), wxPathOnly(name));
        WriteCatalog(name);
    }
}


void poEditFrame::OnExport(wxCommandEvent&)
{
    UpdateFromTextCtrl();

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
                          wxSAVE | wxOVERWRITE_PROMPT, this);
    if (!name.IsEmpty())
    {
        wxConfig::Get()->Write(_T("last_file_path"), wxPathOnly(name));
        ExportCatalog(name);
    }
}

bool poEditFrame::ExportCatalog(const wxString& filename)
{
    wxBusyCursor bcur;
    bool ok = m_catalog->ExportToHTML(filename);
	return ok;
}



void poEditFrame::OnNew(wxCommandEvent& event)
{
    bool isFromPOT = event.GetId() == XRCID("menu_new_from_pot");
    
    UpdateFromTextCtrl();
    if (m_catalog && m_modified)
    {
        int r =
            wxMessageBox(_("Catalog modified. Do you want to save changes?"),
                         _("Save changes"), 
                         wxYES_NO | wxCANCEL | wxCENTRE | wxICON_QUESTION);
        if (r == wxYES)
            WriteCatalog(m_fileName);
        else if (r == wxCANCEL)
            return;
    }

    SettingsDialog dlg(this);
    Catalog *catalog = new Catalog;
    
    if (isFromPOT)
    {
        wxString path = wxPathOnly(m_fileName);
        if (path.empty())
            path = wxConfig::Get()->Read(_T("last_file_path"), wxEmptyString);
        wxString pot_file = 
            wxFileSelector(_("Open catalog template"),
                 path, wxEmptyString, wxEmptyString,
                 _("GNU GetText templates (*.pot)|*.pot|All files (*.*)|*.*"), 
                 wxOPEN | wxFILE_MUST_EXIST, this);
        bool ok = false;
        if (!pot_file.empty())
        {
            wxConfig::Get()->Write(_T("last_file_path"), wxPathOnly(pot_file));
            ok = catalog->UpdateFromPOT(pot_file, false/*summary*/);
        }
        if (!ok)
        {
            delete m_catalog;
            return;
        }
    }
    
    dlg.TransferTo(catalog);
    if (dlg.ShowModal() == wxID_OK)
    {
        CancelItemsValidation();
        
        dlg.TransferFrom(catalog);
        delete m_catalog;
        m_catalog = catalog;
        m_fileName = wxEmptyString;
        m_modified = true;
        OnSave(event);
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



void poEditFrame::OnSettings(wxCommandEvent&)
{
    SettingsDialog dlg(this);
    
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



void poEditFrame::OnPreferences(wxCommandEvent&)
{
    PreferencesDialog dlg(this);
    
    dlg.TransferTo(wxConfig::Get());
    if (dlg.ShowModal() == wxID_OK)
    {
        dlg.TransferFrom(wxConfig::Get());
        gs_focusToText = (bool)wxConfig::Get()->Read(_T("focus_to_text"),
                                                     (long)false);
        SetCustomFonts();
        UpdateCommentWindowEditable();
        InitSpellchecker();
    }
}



void poEditFrame::UpdateCatalog(const wxString& pot_file)
{
    CancelItemsValidation();
    
    UpdateFromTextCtrl();
    bool succ;
    if (pot_file.empty())
        succ = m_catalog->Update();
    else
        succ = m_catalog->UpdateFromPOT(pot_file);

    RestartItemsValidation();
    
    m_modified = succ || m_modified;
    if (!succ)
    {
        wxLogWarning(_("Entries in the catalog are probably incorrect."));
        wxLogError(
           _("Updating the catalog failed. Click on 'More>>' for details."));
    }
}

void poEditFrame::OnUpdate(wxCommandEvent& event)
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
                 _("GNU GetText templates (*.pot)|*.pot|All files (*.*)|*.*"), 
                 wxOPEN | wxFILE_MUST_EXIST, this);
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



void poEditFrame::OnListSel(wxListEvent& event)
{
    if (m_sel != -1)
        UpdateFromTextCtrl(m_sel);
    
    wxWindow *focus = wxWindow::FindFocus();
    bool hasFocus = (focus == m_textTrans) ||
                    (focus && focus->GetParent() == m_pluralNotebook);

    m_sel = event.GetIndex();
    UpdateToTextCtrl(m_sel);
    event.Skip();
    
    if (hasFocus)
    {
        if (m_textTrans->IsShown())
            m_textTrans->SetFocus();
        else if (!m_textTransPlural.empty())
            m_textTransPlural[0]->SetFocus();
    }
}



void poEditFrame::OnListDesel(wxListEvent& event)
{
// OnListDesel does not seem to be called for virtual mode lists
/*    wxMessageBox(wxString(_T("OnListDesel: "))<<event.GetIndex());
    UpdateFromTextCtrl(event.GetIndex());*/
    event.Skip();
}

void poEditFrame::OnListActivated(wxListEvent& event)
{
    if (m_catalog)
    {
        int ind = m_list->GetItemData(event.GetIndex());
        if (ind >= (int)m_catalog->GetCount()) return;    
        CatalogData& entry = (*m_catalog)[ind];
        if (entry.GetValidity() == CatalogData::Val_Invalid)
        {
            wxMessageBox(entry.GetErrorString(),
                         _("Gettext syntax error"),
                         wxOK | wxICON_ERROR);
        }
    }
}



void poEditFrame::OnReferencesMenu(wxCommandEvent& event)
{
    int selItem = m_list->GetItemData(m_sel);
    if (selItem < 0 || selItem >= (int)m_catalog->GetCount()) return;
    
    const wxArrayString& refs = (*m_catalog)[selItem].GetReferences();

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


void poEditFrame::OnReference(wxCommandEvent& event)
{
    ShowReference(event.GetId() - ED_POPUP_REFS);
}



void poEditFrame::ShowReference(int num)
{
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
    
    if (wxConfig::Get()->Read(_T("open_editor_immediately"), (long)false))
    {
        FileViewer::OpenInEditor(basepath, 
                                 (*m_catalog)[m_list->GetItemData(m_sel)].GetReferences()[num]);
    }
    else
    {
        FileViewer *w = new FileViewer(this, basepath,
                                       (*m_catalog)[m_list->GetItemData(m_sel)].GetReferences(),
                                       num);
        if (w->FileOk())
            w->Show(true);
        else
            w->Close();
    }
}



void poEditFrame::OnFuzzyFlag(wxCommandEvent& event)
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
    m_edittedTextFuzzyChanged = true;
    UpdateFromTextCtrl();
}



void poEditFrame::OnQuotesFlag(wxCommandEvent& event)
{
    UpdateFromTextCtrl();
    if (event.GetEventObject() == GetToolBar())
        GetMenuBar()->Check(XRCID("menu_quotes"), 
                            GetToolBar()->GetToolState(XRCID("menu_quotes")));
    else
        GetToolBar()->ToggleTool(XRCID("menu_quotes"),  
                                 GetMenuBar()->IsChecked(XRCID("menu_quotes")));
    m_displayQuotes = GetToolBar()->GetToolState(XRCID("menu_quotes"));
    UpdateToTextCtrl();
}



void poEditFrame::OnLinesFlag(wxCommandEvent& event)
{
    m_displayLines = GetMenuBar()->IsChecked(XRCID("menu_lines"));
    m_list->SetDisplayLines(m_displayLines);
    RefreshControls();
}



void poEditFrame::OnCommentWinFlag(wxCommandEvent& event)
{
    UpdateDisplayCommentWin();
}

void poEditFrame::OnAutoCommentsWinFlag(wxCommandEvent& event)
{
    UpdateDisplayCommentWin();
}


void poEditFrame::OnShadedListFlag(wxCommandEvent& event)
{
    gs_shadedList = GetMenuBar()->IsChecked(XRCID("menu_shaded"));
    RefreshControls();
}



void poEditFrame::OnInsertOriginal(wxCommandEvent& event)
{
    if (!m_textTransPlural.empty())
    {
        wxString orig = m_textOrigPlural->GetValue();
        for (size_t i = 0; i < m_textTransPlural.size(); i++)
            m_textTransPlural[i]->SetValue(orig);
    }
    else
    {
        m_textTrans->SetValue(m_textOrig->GetValue());
    }
}



void poEditFrame::OnFullscreen(wxCommandEvent& event)
{
    bool fs = IsFullScreen();
    wxConfigBase *cfg = wxConfigBase::Get();

    GetMenuBar()->Check(XRCID("menu_fullscreen"), !fs);
    GetToolBar()->ToggleTool(XRCID("menu_fullscreen"), !fs);

    if (fs)
    {
        cfg->Write(_T("splitter_fullscreen"), (long)m_splitter->GetSashPosition());
        m_splitter->SetSashPosition(cfg->Read(_T("splitter"), 240L));
    }
    else
    {
        long oldSash = m_splitter->GetSashPosition();
        cfg->Write(_T("splitter"), oldSash);
        m_splitter->SetSashPosition(cfg->Read(_T("splitter_fullscreen"), oldSash));
    }

    ShowFullScreen(!fs, wxFULLSCREEN_NOBORDER | wxFULLSCREEN_NOCAPTION);
}



void poEditFrame::OnFind(wxCommandEvent& event)
{
    FindFrame *f = (FindFrame*)FindWindow(_T("find_frame"));

    if (!f)
        f = new FindFrame(this, m_list, m_catalog, m_textOrig, m_textTrans, m_textComment, m_textAutoComments);
    f->Show(true);
}


inline wxString convertToLocalCharset(const wxString& str)
{
#if !wxUSE_UNICODE
    wxString s2(str.wc_str(wxConvUTF8), wxConvLocal);
    if (s2.empty() /*conversion failed*/)
        return str;
    else
        return s2;
#else
    return str;
#endif
}

inline wxString convertFromLocalCharset(const wxString& str)
{
#if !wxUSE_UNICODE
    return wxString(str.wc_str(wxConvLocal), wxConvUTF8);
#else
    return str;
#endif
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

void poEditFrame::UpdateFromTextCtrl(int item)
{
    if (m_catalog == NULL) return;
    if (item == -1) item = m_sel;
    if (m_sel == -1 || m_sel >= m_list->GetItemCount()) return;
    int ind = m_list->GetItemData(item);
    if (ind >= (int)m_catalog->GetCount()) return;
    
    CatalogData& entry = (*m_catalog)[ind];
    
    wxString key = entry.GetString();
    wxString newval = m_textTrans->GetValue();
    bool newfuzzy = GetToolBar()->GetToolState(XRCID("menu_fuzzy"));
    
    // check if anything changed:
    if (entry.IsFuzzy() == newfuzzy)
    {
        if (entry.HasPlural())
        {
            bool changed = false;
#ifndef __VISUALC__
    // unbelievably, VC++ doesn't define std::min/max()!
    #define _cpp_min std::min
#endif
            size_t size = _cpp_min(m_textTransPlural.size(),
                                   m_edittedTextOrig.size());
            
            for (size_t i = 0; i < size; i++)
            {
                if (m_edittedTextOrig.empty() ||
                    m_textTransPlural[i]->GetValue() != m_edittedTextOrig[i])
                {
                    changed = true;
                    break;
                }
            }
            if (!changed)
                return;
        }
        else
        {
            if (!m_edittedTextOrig.empty() && newval == m_edittedTextOrig[0])
                return;
        }
    }

    newval = TransformNewval(newval, m_displayQuotes);

//    m_list->SetItem(item, 1, newval.substr(0, m_list->GetMaxColChars()));

#if !wxUSE_UNICODE
    // convert to UTF-8 using user's environment default charset (do it
    // after calling m_list->SetItem because SetItem expects string in
    // local charset, not UTF-8):
    newval = convertFromLocalCharset(newval);
#endif

    if (entry.HasPlural())
    {
        wxArrayString str;
        for (unsigned i = 0; i < m_textTransPlural.size(); i++)
        {
            wxString val = TransformNewval(m_textTransPlural[i]->GetValue(),
                                           m_displayQuotes);
#if !wxUSE_UNICODE
            // convert to UTF-8 using user's environment default charset (do it
            // after calling m_list->SetItem because SetItem expects string in
            // local charset, not UTF-8):
            val = convertFromLocalCharset(val);
#endif
            str.Add(val);
        }
        entry.SetTranslations(str);
    }
    else
    {
        entry.SetTranslation(newval);
    }

    if (newfuzzy == entry.IsFuzzy() && !m_edittedTextFuzzyChanged)
        newfuzzy = false;
    entry.SetFuzzy(newfuzzy);
    GetToolBar()->ToggleTool(XRCID("menu_fuzzy"), newfuzzy);
    GetMenuBar()->Check(XRCID("menu_fuzzy"), newfuzzy);


    entry.SetModified(true);
    entry.SetAutomatic(false);
    entry.SetTranslated(!newval.IsEmpty());
    
    m_list->RefreshItem(item);
    
    if (m_modified == false)
    {
        m_modified = true;
        UpdateTitle();
    }

    UpdateStatusBar();

    // check validity of this item:
    m_itemsToValidate.push_front(item);
}


void poEditFrame::UpdateToTextCtrl(int item)
{
    if (m_catalog == NULL) return;
    if (item == -1) item = m_sel;
    if (item == -1 || item >= m_list->GetItemCount()) return;
    int ind = m_list->GetItemData(item);
    if (ind >= (int)m_catalog->GetCount()) return;
    
    const CatalogData& entry = (*m_catalog)[ind];

    wxString quote;
    wxString t_o, t_t, t_c, t_ac;
    if (m_displayQuotes) quote = _T("\""); else quote = wxEmptyString;
    t_o = quote + entry.GetString() + quote;
    t_o.Replace(_T("\\n"), _T("\\n\n"));
    t_c = entry.GetComment();
    t_c.Replace(_T("\\n"), _T("\\n\n"));

    for (unsigned i=0; i < entry.GetAutoComments().GetCount(); i++)
      t_ac += entry.GetAutoComments()[i] + _T("\n");
    t_ac.Replace(_T("\\n"), _T("\\n\n"));

    // remove "# " in front of every comment line
    t_c = CommentDialog::RemoveStartHash(t_c);

#if !wxUSE_UNICODE
    // Convert from UTF-8 to environment's default charset:
    t_o = convertToLocalCharset(t_o);
    t_c = convertToLocalCharset(t_c);
#endif
    
    m_textOrig->SetValue(t_o);

    m_edittedTextOrig.clear();

    if (entry.HasPlural())
    {
        wxString t_op = quote + entry.GetPluralString() + quote;
        t_op.Replace(_T("\\n"), _T("\\n\n"));
#if !wxUSE_UNICODE
        t_op = convertToLocalCharset(t_op);
#endif
        m_textOrigPlural->SetValue(t_op);

        size_t formsCnt = m_textTransPlural.size();
        for (size_t i = 0; i < formsCnt; i++)
            m_textTransPlural[i]->SetValue(wxEmptyString);

        for (size_t i = 0; 
                i < wxMin(formsCnt, entry.GetNumberOfTranslations()); i++)
        {
            t_t = quote + entry.GetTranslation(i) + quote;
            t_t.Replace(_T("\\n"), _T("\\n\n"));
#if !wxUSE_UNICODE
            t_t = convertToLocalCharset(t_t);
#endif
            m_textTransPlural[i]->SetValue(t_t);
            if (m_displayQuotes) 
                m_textTransPlural[i]->SetInsertionPoint(1);
            m_edittedTextOrig.push_back(t_t);
        }
    }
    else
    {
        t_t = quote + entry.GetTranslation() + quote;
        t_t.Replace(_T("\\n"), _T("\\n\n"));
#if !wxUSE_UNICODE
        t_t = convertToLocalCharset(t_t);
#endif
        m_textTrans->SetValue(t_t);
        if (m_displayQuotes) 
            m_textTrans->SetInsertionPoint(1);
        m_edittedTextOrig.push_back(t_t);
    }
    
    if (m_displayCommentWin)
        m_textComment->SetValue(t_c);
        
    if (m_displayAutoCommentsWin)
        m_textAutoComments->SetValue(t_ac);

    m_edittedTextFuzzyChanged = false;
    GetToolBar()->ToggleTool(XRCID("menu_fuzzy"), entry.IsFuzzy());
    GetMenuBar()->Check(XRCID("menu_fuzzy"), entry.IsFuzzy());
    
    ShowPluralFormUI(entry.HasPlural());
}



void poEditFrame::ReadCatalog(const wxString& catalog)
{
    CancelItemsValidation();

    delete m_catalog;
    m_catalog = new Catalog(catalog);
    m_list->SetCatalog(m_catalog);
    dynamic_cast<TextctrlHandler*>(m_textTrans->GetEventHandler())->SetCatalog(m_catalog);

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
}



void poEditFrame::AddItemsToList(const Catalog& catalog,
                                 poEditListCtrl *list, size_t& pos,
                                 bool (*filter)(const CatalogData& d),
                                 const wxColour *clr)
{
/*    int clrPos;
    wxListItem listitem;
    size_t cnt = catalog.GetCount();
    size_t maxchars = list->GetMaxColChars();

    for (size_t i = 0; i < cnt; i++)
    {
        if (filter(catalog[i]))
        {
            wxString orig = convertToLocalCharset(catalog[i].GetString());
                
            list->InsertItem(pos,
                             orig.substr(0, maxchars),
                             GetItemIcon(catalog[i]));
            
            // Convert from UTF-8 to environment's default charset:
            wxString trans = convertToLocalCharset(catalog[i].GetTranslation());
            list->SetItem(pos, 1, trans.substr(0, maxchars));

            if (m_displayLines)
            {
                wxString linenum;
                linenum << catalog[i].GetLineNumber();
                list->SetItem(pos, 2, linenum);
            }

            list->SetItemData(pos, i);
            listitem.SetId(pos);
            clrPos = gs_shadedList ? (pos % 2) : 0;
            if (clr && clr[clrPos].Ok())
            {
                list->GetItem(listitem);
                listitem.SetBackgroundColour(clr[clrPos]);
                list->SetItem(listitem);
            }
            pos++;
        }
    }*/
}

static bool CatFilterUntranslated(const CatalogData& d)
{
    return !d.IsTranslated();
}

static bool CatFilterFuzzy(const CatalogData& d)
{
    return d.IsFuzzy() &&
           d.GetValidity() != CatalogData::Val_Invalid && d.IsTranslated();
}

static bool CatFilterInvalid(const CatalogData& d)
{
    return d.GetValidity() == CatalogData::Val_Invalid && d.IsTranslated();
}

static bool CatFilterRest(const CatalogData& d)
{
    return !d.IsFuzzy() &&
           d.GetValidity() != CatalogData::Val_Invalid && d.IsTranslated();
}

void poEditFrame::RefreshControls()
{
    if (!m_catalog) return;
    
    m_hasObsoleteItems = false;    
    if (!m_catalog->IsOk())
    {
        wxLogError(_("Error loading message catalog file '") + m_fileName + _("'."));
        m_fileName = wxEmptyString;
        UpdateMenu();
        UpdateTitle();
        delete m_catalog;
        m_catalog = NULL;
        return;
    }
    
    wxBusyCursor bcur;
    UpdateMenu();

    long selection_idx = m_list->GetFirstSelected();
    wxString selection;
    if (selection_idx != -1)
        selection = m_list->GetItemText(selection_idx);
    
    m_list->Freeze();
    m_list->CreateColumns(); // This forces to reread the catalog
    m_list->Refresh();

    wxString trans;
    size_t pos = 0;
   
    m_list->Thaw();
    
    if (m_catalog->GetCount() > 0)
    {
        if (selection.empty())
        {
            m_list->Select(0);
            m_list->Focus(0);
        }
        else
        {
            size_t cnt = m_catalog->GetCount();
            for (size_t i = 0; i < cnt; i++)
            {
                if (m_list->GetItemText(i) == selection)
                {
                    // This will force to not update the item that now has the 
                    // position the item that just got modified had before the 
                    // catalog was saved. If we don't force this to happen,
                    // that item would get the value of the one that just got
                    // modified (from the text controls), thus deleting its 
                    // legitimate value
                    m_sel = -1; 
                    
                    // Now, select the item in the list
                    m_list->Select(i);
                    m_list->Focus(i);
                    break;
                }
            }
        }
    }
	
    FindFrame *f = (FindFrame*)FindWindow(_T("find_frame"));
    if (f)
        f->Reset(m_catalog);
    
    UpdateTitle();
    UpdateStatusBar();
    Refresh();
}



void poEditFrame::UpdateStatusBar()
{
    int all, fuzzy, untranslated, badtokens;
    if (m_catalog)
    {
        wxString txt;
        
        m_catalog->GetStatistics(&all, &fuzzy, &badtokens, &untranslated);
        
        txt.Printf(_("%i strings (%i fuzzy, %i bad tokens, %i not translated)"), 
                   all, fuzzy, badtokens, untranslated);

        if (!m_itemsToValidate.empty())
        {
            wxString progress;
            progress.Printf(_("[checking translations: %i left]"),
                            m_itemsToValidate.size());
            txt += _T("    ");
            txt += progress;
        }
        
        GetStatusBar()->SetStatusText(txt);
        if (all > 0)
            m_statusGauge->SetValue(100 * (all-fuzzy-badtokens-untranslated) / all);
        else
            m_statusGauge->SetValue(0);
    }
}
        
void poEditFrame::DoGiveHelp(const wxString& text, bool show)
{
    if (show || !text.empty())
        wxFrame::DoGiveHelp(text, show);
    else
        UpdateStatusBar();        
}


void poEditFrame::UpdateTitle()
{
    if (m_modified)
        SetTitle(_T("poEdit : ") + m_fileName + _(" (modified)"));
    else
        SetTitle(_T("poEdit : ") + m_fileName);
}



void poEditFrame::UpdateMenu()
{
    if (m_catalog == NULL)
    {
        GetMenuBar()->Enable(XRCID("menu_save"), false);
        GetMenuBar()->Enable(XRCID("menu_saveas"), false);
        GetMenuBar()->Enable(XRCID("menu_export"), false);
        GetToolBar()->EnableTool(XRCID("menu_save"), false);
        GetToolBar()->EnableTool(XRCID("menu_update"), false);
        GetToolBar()->EnableTool(XRCID("menu_fuzzy"), false);
        GetToolBar()->EnableTool(XRCID("menu_comment"), false);
        GetMenuBar()->EnableTop(1, false);
        GetMenuBar()->EnableTop(2, false);
        m_textTrans->Enable(false);
        m_textOrig->Enable(false);
        m_textOrigPlural->Enable(false);
        m_textComment->Enable(false);
        m_textAutoComments->Enable(false);
        m_list->Enable(false);
    }
    else
    {
        GetMenuBar()->Enable(XRCID("menu_save"), true);
        GetMenuBar()->Enable(XRCID("menu_saveas"), true);
        GetMenuBar()->Enable(XRCID("menu_export"), true);
        GetToolBar()->EnableTool(XRCID("menu_save"), true);
        GetToolBar()->EnableTool(XRCID("menu_fuzzy"), true);
        GetToolBar()->EnableTool(XRCID("menu_comment"), true);
        GetMenuBar()->EnableTop(1, true);
        GetMenuBar()->EnableTop(2, true);
        m_textTrans->Enable(true);
        m_textOrig->Enable(true);
        m_textOrigPlural->Enable(true);
        m_textComment->Enable(true);
        m_textAutoComments->Enable(true);
        m_list->Enable(true);
        bool doupdate = m_catalog->Header().SearchPaths.GetCount() > 0;
        GetToolBar()->EnableTool(XRCID("menu_update"), doupdate);
        GetMenuBar()->Enable(XRCID("menu_update"), doupdate);
        GetMenuBar()->Enable(XRCID("menu_purge_deleted"),
                             m_catalog->HasDeletedItems());
    }
}



void poEditFrame::WriteCatalog(const wxString& catalog)
{
    wxBusyCursor bcur;

    Catalog::HeaderData& dt = m_catalog->Header();
    dt.Translator = wxConfig::Get()->Read(_T("translator_name"), dt.Translator);
    dt.TranslatorEmail = wxConfig::Get()->Read(_T("translator_email"), dt.TranslatorEmail);

    m_catalog->Save(catalog);
    m_fileName = catalog;
    m_modified = false;

#ifdef USE_TRANSMEM
    if (GetTransMem())
    {
        TranslationMemory *tm = GetTransMem();
        size_t cnt = m_catalog->GetCount();
        for (size_t i = 0; i < cnt; i++)
        {
            CatalogData& dt = (*m_catalog)[i];
            if (dt.IsModified() && !dt.IsFuzzy() &&
                dt.GetValidity() == CatalogData::Val_Valid &&
                !dt.GetTranslation().IsEmpty())
            {
                tm->Store(dt.GetString(), dt.GetTranslation());
            }
        }
    }
#endif

    m_history.AddFileToHistory(m_fileName);
    UpdateTitle();
    
    RefreshControls();
    
    if (ManagerFrame::Get())
        ManagerFrame::Get()->NotifyFileChanged(m_fileName);
}


void poEditFrame::OnEditComment(wxCommandEvent& event)
{
    int selItem = m_list->GetItemData(m_sel);
    if (selItem < 0 || selItem >= (int)m_catalog->GetCount()) return;
   
    wxString comment = convertToLocalCharset(
                            (*m_catalog)[selItem].GetComment());
    CommentDialog dlg(this, comment);
    if (dlg.ShowModal() == wxID_OK)
    {
        m_modified = true;
        UpdateTitle();
        comment = convertFromLocalCharset(dlg.GetComment());
        (*m_catalog)[selItem].SetComment(comment);

        m_list->RefreshItem(m_sel);
/*        wxListItem listitem;
        listitem.SetId(m_sel);
        m_list->GetItem(listitem);
        int icon = GetItemIcon((*m_catalog)[m_selItem]);
        m_list->SetItemImage(listitem, icon, icon);
        m_list->SetItem(listitem);*/

        // update comment window
        m_textComment->SetValue(CommentDialog::RemoveStartHash(comment));
    }
}
        
void poEditFrame::OnPurgeDeleted(wxCommandEvent& WXUNUSED(event))
{
    wxMessageDialog dlg(this,
                        _("Do you really want to remove all translations that are no longer used from the catalog?\nIf you continue with purging, you will have to translate them again if they are added back in the future."),
                        _("Purge delete translations"), 
                        wxYES_NO | wxICON_QUESTION);

    if (dlg.ShowModal() == wxID_YES)
    {
        m_catalog->RemoveDeletedItems();
        UpdateMenu();
    }
}


#ifdef USE_TRANSMEM
void poEditFrame::OnAutoTranslate(wxCommandEvent& event)
{
    int ind = event.GetId() - ED_POPUP_TRANS;
    (*m_catalog)[m_list->GetItemData(m_sel)].SetTranslation(m_autoTranslations[ind]);
    UpdateToTextCtrl();
    // VS: This dirty trick ensures proper refresh of everything: 
    m_edittedTextOrig.clear();
    m_edittedTextFuzzyChanged = false;
    UpdateFromTextCtrl();
}

void poEditFrame::OnAutoTranslateAll(wxCommandEvent& event)
{
    AutoTranslateCatalog();
}

bool poEditFrame::AutoTranslateCatalog()
{
    TranslationMemory *tm = GetTransMem();

    if (tm == NULL)
        return false;
    
    size_t cnt = m_catalog->GetCount();
    size_t matches = 0;
    wxString msg;
    
    ProgressInfo pi;
    pi.SetTitle(_("Automatically translating..."));
    pi.SetGaugeMax(cnt);
    for (size_t i = 0; i < cnt; i++)
    {
        CatalogData& dt = (*m_catalog)[i];
        if (dt.IsFuzzy() || !dt.IsTranslated())
        {
            wxArrayString results;
            int score = tm->Lookup(dt.GetString(), results);
            if (score > 0)
            {
                m_catalog->Translate(dt.GetString(), results[0]);
                dt.SetAutomatic(true);
                dt.SetFuzzy(true);
                matches++;
                msg.Printf(_("Automatically translated %u strings"), matches);
                pi.UpdateMessage(msg);
            }
        }
        pi.UpdateGauge();
    }

    RefreshControls();
    
    return true;
}
#endif

wxMenu *poEditFrame::GetPopupMenu(size_t item)
{
    if (!m_catalog) return NULL;
    if (item >= (size_t)m_list->GetItemCount()) return NULL;
    
    const wxArrayString& refs = (*m_catalog)[item].GetReferences();
    wxMenu *menu = new wxMenu;
    
    menu->Append(XRCID("menu_insert_orig"),
                 wxString(_("Copy original to translation field")) 
                   + _T("\tAlt-C"));
    menu->AppendSeparator();

#ifdef CAN_MODIFY_DEFAULT_FONT
    wxMenuItem *it1 = new wxMenuItem(menu, ED_POPUP_DUMMY+0, _("References:"));
    it1->SetFont(m_boldGuiFont);
    menu->Append(it1);
#else
    menu->Append(ED_POPUP_DUMMY+0, _("References:"));
#endif    
    menu->AppendSeparator();
    for (size_t i = 0; i < refs.GetCount(); i++)
        menu->Append(ED_POPUP_REFS + i, _T("   ") + refs[i]);

#ifdef USE_TRANSMEM
    if (GetTransMem())
    {
        menu->AppendSeparator();
#ifdef CAN_MODIFY_DEFAULT_FONT
        wxMenuItem *it2 = new wxMenuItem(menu, ED_POPUP_DUMMY+1, 
                                         _("Automatic translations:"));
        it2->SetFont(m_boldGuiFont);
        menu->Append(it2);
#else
        menu->Append(ED_POPUP_DUMMY+1, _("Automatic translations:"));
#endif    
        menu->AppendSeparator();

        wxBusyCursor bcur;
        CatalogData& dt = (*m_catalog)[item];
        m_autoTranslations.Clear();
        if (GetTransMem()->Lookup(dt.GetString(), m_autoTranslations) > 0)
        {
            for (size_t i = 0; i < m_autoTranslations.GetCount(); i++)
            {
                // Convert from UTF-8 to environment's default charset:
                wxString s(m_autoTranslations[i].wc_str(wxConvUTF8), wxConvLocal);
                if (!s)
                    s = m_autoTranslations[i];
                menu->Append(ED_POPUP_TRANS + i, _T("   ") + s);
            }
        }
        else
        {
            menu->Append(ED_POPUP_DUMMY+2, _("none"));
            menu->Enable(ED_POPUP_DUMMY+2, false);
        }
    }
#endif

    return menu;
}


void poEditFrame::OnAbout(wxCommandEvent&)
{
    wxBusyCursor busy;
    wxDialog dlg;
    wxXmlResource::Get()->LoadDialog(&dlg, this, _T("about_box"));
    wxString version = wxString(_("version")) + _T(" ") + wxGetApp().GetAppVersion();
    XRCCTRL(dlg, "version", wxStaticText)->SetLabel(version);
                       
    dlg.GetSizer()->RecalcSizes();
    dlg.Layout();
    dlg.Centre();
    dlg.ShowModal();
}


void poEditFrame::OnHelp(wxCommandEvent&)
{
#ifdef __WXMSW__
    m_help.LoadFile(m_helpBook);
#endif
    m_help.DisplayContents();
}

void poEditFrame::OnHelpGettext(wxCommandEvent&)
{
#ifdef __WXMSW__
    m_help.LoadFile(wxGetApp().GetAppPath() + _T("/share/poedit/gettext.chm"));
    m_help.DisplayContents();
#endif
}


void poEditFrame::OnManager(wxCommandEvent&)
{
    wxFrame *f = ManagerFrame::Create();
    f->Raise();
}

void poEditFrame::SetCustomFonts()
{
    wxConfigBase *cfg = wxConfig::Get();
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
            m_list->SetFont(font);
        }
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
        }
    }
}

void poEditFrame::UpdateCommentWindowEditable()
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
                                        EDC_TEXTCOMMENT, wxEmptyString, 
                                        wxDefaultPosition, wxDefaultSize, 
                                        wxTE_MULTILINE);
        }
        else
        {
            m_textComment = new UnfocusableTextCtrl(m_bottomRightPanel,
                                        EDC_TEXTCOMMENT, wxEmptyString, 
                                        wxDefaultPosition, wxDefaultSize, 
                                        wxTE_MULTILINE | wxTE_READONLY);
        }
        UpdateDisplayCommentWin();
    }
}

void poEditFrame::UpdateDisplayCommentWin()
{
    m_displayCommentWin = GetMenuBar()->IsChecked(XRCID("menu_comment_win"));
    m_displayAutoCommentsWin = GetMenuBar()->IsChecked(XRCID("menu_auto_comments_win"));
    if (m_displayCommentWin || m_displayAutoCommentsWin)
    {
        m_bottomSplitter->SplitVertically(
                m_bottomLeftPanel, m_bottomRightPanel,
                wxConfig::Get()->Read(_T("bottom_splitter"), -200L));
        m_bottomRightPanel->Show(true);
        
        // force recalculation of layout of panel so that text boxes take up
        // all the space they can
        if (m_bottomRightPanel->GetSizer() != NULL) // sizer may be NULL on first call
        {
            m_bottomRightPanel->GetSizer()->Remove(m_textAutoComments);  // need to remove and add again to ensure accurate resizing
            m_bottomRightPanel->GetSizer()->Remove(m_textComment);
            m_bottomRightPanel->GetSizer()->Add(m_textAutoComments, 1, wxEXPAND);
            m_bottomRightPanel->GetSizer()->Add(m_textComment, 1, wxEXPAND);
            m_bottomRightPanel->GetSizer()->Show(m_textComment, m_displayCommentWin);
            m_bottomRightPanel->GetSizer()->Show(m_textAutoComments, m_displayAutoCommentsWin);
            m_bottomRightPanel->GetSizer()->Layout();
        }
    }
    else
    {
        wxConfig::Get()->Write(_T("bottom_splitter"),
                               (long)m_bottomSplitter->GetSashPosition());
        m_bottomRightPanel->Show(false);
        m_bottomSplitter->Unsplit();
    }
    m_list->SetDisplayLines(m_displayLines);
    RefreshControls();
}

void poEditFrame::OnCommentWindowText(wxCommandEvent&)
{
    if (!m_commentWindowEditable)
        return;

    wxString comment;
    comment = convertFromLocalCharset(
            CommentDialog::AddStartHash(m_textComment->GetValue()));
    CatalogData& data((*m_catalog)[m_list->GetItemData(m_sel)]);
   
    wxLogTrace(_T("poedit"), _T("   comm:'%s'"), comment.c_str());
    wxLogTrace(_T("poedit"), _T("datcomm:'%s'"), data.GetComment().c_str());
    if (comment == data.GetComment())
        return;

    data.SetComment(comment);
    
    m_list->RefreshItem(m_sel);
 
    if (m_modified == false)
    {
        m_modified = true;
        UpdateTitle();
    }
}


void poEditFrame::OnIdle(wxIdleEvent& event)
{
    if (!m_itemsToValidate.empty() && m_itemBeingValidated == -1)
        BeginItemValidation();
    event.Skip();
}

void poEditFrame::OnEndProcess(wxProcessEvent& event)
{
    event.Skip(); // deletes wxProcess object
    EndItemValidation();
    wxWakeUpIdle();
}
        
void poEditFrame::CancelItemsValidation()
{
    // stop checking entries in the background:
    m_itemsToValidate.clear();
    m_itemBeingValidated = -1;
}
        
void poEditFrame::RestartItemsValidation()
{
    // start checking catalog's entries in the background:
    int cnt = m_list->GetItemCount();
    for (int i = 0; i < cnt; i++)
        m_itemsToValidate.push_back(i);
}

void poEditFrame::BeginItemValidation()
{
    int item = m_itemsToValidate.front();
    int index = m_list->GetItemData(item);
    CatalogData& dt = (*m_catalog)[index];

    if (!dt.IsTranslated())
        return;

    if (dt.GetValidity() != CatalogData::Val_Unknown)
        return;

    // run this entry through msgfmt (in a single-entry catalog) to check if
    // it is correct:
    Catalog cat;
    cat.AddItem(new CatalogData(dt));

    if (m_catalog->Header().HasHeader(_T("Plural-Forms")))
    {
        cat.Header().SetHeader(
                _T("Plural-Forms"),
                m_catalog->Header().GetHeader(_T("Plural-Forms")));
    }
    
    wxString tmp1 = wxGetTempFileName(_T("poedit"));
    wxString tmp2 = wxGetTempFileName(_T("poedit"));
    cat.Save(tmp1, false);
    wxString cmdline = _T("msgfmt -c -f -o \"") + tmp2 +
                       _T("\" \"") + tmp1 + _T("\"");

    m_validationProcess.tmp1 = tmp1;
    m_validationProcess.tmp2 = tmp2;
    
    if (ExecuteGettextNonblocking(cmdline, &m_validationProcess, this))
    {
        m_itemBeingValidated = item;
        m_itemsToValidate.pop_front();
    }
    else
    {
        EndItemValidation();
    }
}

void poEditFrame::EndItemValidation()
{ 
    wxRemoveFile(m_validationProcess.tmp1);
    wxRemoveFile(m_validationProcess.tmp2);

    if (m_itemBeingValidated != -1)
    {
        int item = m_itemBeingValidated;
        int index = m_list->GetItemData(item);
        CatalogData& dt = (*m_catalog)[index];
       
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
}
        
void poEditFrame::ShowPluralFormUI(bool show)
{
    if (show && (!m_catalog || m_catalog->GetPluralFormsCount() == 0))
        show = false;

    wxSizer *origSizer = m_textOrig->GetContainingSizer();
#if wxCHECK_VERSION(2,5,0)
    origSizer->Show(m_labelSingular, show);
    origSizer->Show(m_labelPlural, show);
    origSizer->Show(m_textOrigPlural, show);
#else
    // the above code doesn't work well with wx-2.4, so here's a no-so-good
    // workaround:
    if (show && m_labelSingular->GetSizer() == NULL)
    {
        origSizer->Add(m_labelPlural, 0, wxALIGN_CENTER_VERTICAL | wxALL, 3);
        origSizer->Add(m_textOrigPlural, 1, wxEXPAND);
        origSizer->SetItemMinSize(m_textOrigPlural, 1, 1);
    }
    else
    {
        origSizer->Remove(m_labelPlural);
        origSizer->Remove(m_textOrigPlural);
    }
    m_labelPlural->Show(show);
    m_textOrigPlural->Show(show);
#endif
    origSizer->Layout();

    wxSizer *textSizer = m_textTrans->GetContainingSizer();
    textSizer->Show(m_textTrans, !show);
    textSizer->Show(m_pluralNotebook, show);
    textSizer->Layout();
}


void poEditFrame::RecreatePluralTextCtrls()
{
    for (size_t i = 0; i < m_textTransPlural.size(); i++)
        m_textTransPlural[i]->PopEventHandler(true/*delete*/);
    m_textTransPlural.clear();
    m_pluralNotebook->DeleteAllPages();

    if (!m_catalog)
        return;
        
    PluralFormsCalculator *calc = PluralFormsCalculator::make(
                m_catalog->Header().GetHeader(_T("Plural-Forms")).ToAscii());

    unsigned cnt = m_catalog->GetPluralFormsCount();
    for (unsigned i = 0; i < cnt; i++)
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
            desc.Printf(_("Form %u"), i);
        else
            desc.Printf(_("Form %u (e.g. \"%u\")"), i, example);

        // create text control and notebook page for it:
        wxTextCtrl *txt = new wxTextCtrl(m_pluralNotebook, -1,
                                         wxEmptyString, 
                                         wxDefaultPosition, wxDefaultSize, 
                                         wxTE_MULTILINE);
        txt->PushEventHandler(new TextctrlHandler(this));
        m_textTransPlural.push_back(txt);
        m_pluralNotebook->AddPage(txt, desc);
    }

    delete calc;

    SetCustomFonts();
    InitSpellchecker();
    UpdateToTextCtrl();
}

void poEditFrame::OnListRightClick(wxMouseEvent& event)
{
    long item;
    int flags = wxLIST_HITTEST_ONITEM;
    wxListCtrl *list = (wxListCtrl*)event.GetEventObject();

    item = list->HitTest(event.GetPosition(), flags);
    if (item != -1 && (flags & wxLIST_HITTEST_ONITEM))
        list->SetItemState(item, wxLIST_STATE_SELECTED, 
                                 wxLIST_STATE_SELECTED);

    wxMenu *menu = GetPopupMenu(m_list->GetItemData(m_sel));/*(m_frame) ? 
                   m_frame->GetPopupMenu(*m_selItem) : NULL;*/
    if (menu)
    {  
        list->PopupMenu(menu, event.GetPosition());
        delete menu;
    }
    else event.Skip();
}

void poEditFrame::OnListFocus(wxFocusEvent& event)
{
    if (gs_focusToText)
    {
        if (m_textTrans->IsShown())
            m_textTrans->SetFocus();
        else if (!m_textTransPlural.empty())
            (m_textTransPlural)[0]->SetFocus();
    }
    else
        event.Skip();
}



