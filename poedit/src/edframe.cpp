
/*

    poedit, a wxWindows i18n catalogs editor

    ---------------
      edframe.cpp
    
      Editor frame
    
      (c) Vaclav Slavik, 2000-2002

*/


#ifdef __GNUG__
#pragma implementation
#endif

#include <wx/wxprec.h>

#include <wx/wx.h>
#include <wx/imaglist.h>
#include <wx/config.h>
#include <wx/html/htmlwin.h>
#include <wx/statline.h>
#include <wx/sizer.h>
#include <wx/fs_mem.h>
#include <wx/datetime.h>
#include <wx/tokenzr.h>
#include <wx/listctrl.h>
#include <wx/xrc/xmlres.h>
#include <wx/settings.h>
#include <wx/button.h>
#include <wx/statusbr.h>
#include <wx/splitter.h>

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

#include <wx/listimpl.cpp>
WX_DEFINE_LIST(poEditFramesList);
poEditFramesList poEditFrame::ms_instances;

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
        f = new poEditFrame;
    else
    {
        f = poEditFrame::Find(filename);
        if (!f)
            f = new poEditFrame(filename);
    }
    f->Show(true);
    return f;
}


// Event & controls IDs:
enum 
{
    EDC_LIST = 1000,
    EDC_TEXTORIG,
    EDC_TEXTTRANS,
    
    ED_POPUP_REFS = 2000, 
    ED_POPUP_TRANS = 3000,
    ED_POPUP_DUMMY = 4000
};

// colours used in the list:
#define g_darkColourFactor 0.95
#define DARKEN_COLOUR(r,g,b) (wxColour(int((r)*g_darkColourFactor),\
                                       int((g)*g_darkColourFactor),\
                                       int((b)*g_darkColourFactor)))
#define LIST_COLOURS(r,g,b) { wxColour(r,g,b), DARKEN_COLOUR(r,g,b) }
static wxColour 
    g_ItemColourNormal[2] =       LIST_COLOURS(0xFF,0xFF,0xFF), // white
    g_ItemColourUntranslated[2] = LIST_COLOURS(0xA5,0xEA,0xEF), // blue
    g_ItemColourFuzzy[2] =        LIST_COLOURS(0xF4,0xF1,0xC1); // yellow


#include "nothing.xpm"
#include "modified.xpm"
#include "automatic.xpm"
#include "comment.xpm"
#include "comment_modif.xpm"

// list control with both columns equally wide:
class poEditListCtrl : public wxListCtrl
{
    public:
       poEditListCtrl(wxWindow *parent,
                      wxWindowID id = -1,
                      const wxPoint &pos = wxDefaultPosition,
                      const wxSize &size = wxDefaultSize,
                      long style = wxLC_ICON,
                      bool dispLines = false,
                      const wxValidator& validator = wxDefaultValidator,
                      const wxString &name = _T("listctrl"))
             : wxListCtrl(parent, id, pos, size, style, validator, name)
        {
            m_displayLines = dispLines;
            CreateColumns();
            wxImageList *list = new wxImageList(16, 16);
            list->Add(wxBitmap(nothing_xpm));
            list->Add(wxBitmap(modified_xpm));
            list->Add(wxBitmap(automatic_xpm));
            list->Add(wxBitmap(comment_xpm));
            list->Add(wxBitmap(comment_modif_xpm));
            AssignImageList(list, wxIMAGE_LIST_SMALL);
        }
        
        void CreateColumns()
        {
            InsertColumn(0, _("Original string"));
            InsertColumn(1, _("Translation"));
            if (m_displayLines)
                InsertColumn(2, _("Line"), wxLIST_FORMAT_RIGHT);
            SizeColumns();
        }

        void SizeColumns()
        {
            const int LINE_COL_SIZE = m_displayLines ? 50 : 0;

            int w = GetSize().x
                    - wxSystemSettings::GetSystemMetric(wxSYS_VSCROLL_X) - 10
                    - LINE_COL_SIZE;
            SetColumnWidth(0, w / 2);
            SetColumnWidth(1, w - w / 2);
            if (m_displayLines)
                SetColumnWidth(2, LINE_COL_SIZE);

            m_colWidth = (w/2) / GetCharWidth();
        }

        void SetDisplayLines(bool dl) { m_displayLines = dl; }

        // Returns average width of one column in number of characters:
        size_t GetMaxColChars() const
        {
            return m_colWidth * 2/*safety coefficient*/;
        }

    private:
        DECLARE_EVENT_TABLE()
        void OnSize(wxSizeEvent& event)
        {
            SizeColumns();
            event.Skip();
        }

        bool m_displayLines;
        unsigned m_colWidth;
};

BEGIN_EVENT_TABLE(poEditListCtrl, wxListCtrl)
   EVT_SIZE(poEditListCtrl::OnSize)
END_EVENT_TABLE()


// I don't like this global flag, but all poEditFrame instances should share it :(
static bool gs_focusToText = false;
static bool gs_shadedList = false;

// special handling of keyboard in listctrl and textctrl
class ListHandler : public wxEvtHandler
{ 
    public:
        ListHandler(wxTextCtrl *text, poEditFrame *frame,
                    int *sel, int *selitem) :
                 wxEvtHandler(), m_text(text),
                 m_frame(frame), m_sel(sel), m_selItem(selitem) {}

    private:
        void OnActivated(wxListEvent& event)
        {
            if (gs_focusToText)
			    m_text->SetFocus();
			else
				event.Skip();
        }

        void OnListSel(wxListEvent& event)
        {
			*m_sel = event.GetIndex();
            *m_selItem = ((wxListCtrl*)event.GetEventObject())->GetItemData(*m_sel);
            event.Skip();
        }

        void OnRightClick(wxMouseEvent& event)
        {
            long item;
            int flags = wxLIST_HITTEST_ONITEM;
            wxListCtrl *list = (wxListCtrl*)event.GetEventObject();

            item = list->HitTest(event.GetPosition(), flags);
            if (item != -1 && (flags & wxLIST_HITTEST_ONITEM))
                list->SetItemState(item, wxLIST_STATE_SELECTED, 
                                         wxLIST_STATE_SELECTED);

            wxMenu *menu = (m_frame) ? 
                           m_frame->GetPopupMenu(*m_selItem) : NULL;
            if (menu)
            {  
                list->PopupMenu(menu, event.GetPosition());
                delete menu;
            }
            else event.Skip();                    
        }
        
        void OnFocus(wxFocusEvent& event)
        {
            if (gs_focusToText)
                m_text->SetFocus();
			else
				event.Skip();
        }

        DECLARE_EVENT_TABLE() 

        wxTextCtrl *m_text;
        poEditFrame *m_frame;
        int *m_sel, *m_selItem;
};

BEGIN_EVENT_TABLE(ListHandler, wxEvtHandler)
   EVT_LIST_ITEM_ACTIVATED(EDC_LIST, ListHandler::OnActivated)
   EVT_LIST_ITEM_SELECTED(EDC_LIST, ListHandler::OnListSel)
   EVT_RIGHT_DOWN(ListHandler::OnRightClick)
   EVT_SET_FOCUS(ListHandler::OnFocus)
END_EVENT_TABLE()


class TextctrlHandler : public wxEvtHandler
{ 
    public:
        TextctrlHandler(wxListCtrl *list, int *sel) :
                 wxEvtHandler(), m_list(list), m_sel(sel) {}

    private:
        void OnKeyDown(wxKeyEvent& event)
        {
            if (!event.ControlDown())
                event.Skip();
            else 
            {
                switch (event.KeyCode())
                {
                    case WXK_UP:
                        if (*m_sel > 0)
                        {
                            m_list->EnsureVisible(*m_sel - 1);
                            m_list->SetItemState(*m_sel - 1, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
                        }
                        break;
                    case WXK_DOWN:
                        if (*m_sel < m_list->GetItemCount() - 1)
                        {
                            m_list->EnsureVisible(*m_sel + 1);
                            m_list->SetItemState(*m_sel + 1, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
                        }
                        break;
                    case WXK_PRIOR:
                        {
                            int newy = *m_sel - 10;
                            if (newy < 0) newy = 0;
                            m_list->EnsureVisible(newy);
                            m_list->SetItemState(newy, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
                        }
                        break;
                    case WXK_NEXT:
                        {
                            int newy = *m_sel + 10;
                            if (newy >= m_list->GetItemCount()) 
                                newy = m_list->GetItemCount() - 1;
                            m_list->EnsureVisible(newy);
                            m_list->SetItemState(newy, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
                        }
                        break;
                    default:
                        event.Skip();
                }
            }
        }

        DECLARE_EVENT_TABLE() 

        wxListCtrl *m_list;
        int        *m_sel;
};

BEGIN_EVENT_TABLE(TextctrlHandler, wxEvtHandler)
   EVT_KEY_DOWN(TextctrlHandler::OnKeyDown)
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
   EVT_MENU           (XRCID("menu_open"),        poEditFrame::OnOpen)
   EVT_MENU           (XRCID("menu_save"),        poEditFrame::OnSave)
   EVT_MENU           (XRCID("menu_saveas"),      poEditFrame::OnSaveAs)
   EVT_MENU_RANGE     (wxID_FILE1, wxID_FILE9,    poEditFrame::OnOpenHist)
   EVT_MENU           (XRCID("menu_catsettings"), poEditFrame::OnSettings)
   EVT_MENU           (XRCID("menu_preferences"), poEditFrame::OnPreferences)
   EVT_MENU           (XRCID("menu_update"),      poEditFrame::OnUpdate)
   EVT_MENU           (XRCID("menu_update_from_pot"),poEditFrame::OnUpdate)
   EVT_MENU           (XRCID("menu_fuzzy"),       poEditFrame::OnFuzzyFlag)
   EVT_MENU           (XRCID("menu_quotes"),      poEditFrame::OnQuotesFlag)
   EVT_MENU           (XRCID("menu_lines"),       poEditFrame::OnLinesFlag)
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
#endif
   EVT_LIST_ITEM_SELECTED
                      (EDC_LIST,       poEditFrame::OnListSel)
   EVT_LIST_ITEM_DESELECTED
                      (EDC_LIST,       poEditFrame::OnListDesel)
   EVT_CLOSE          (                poEditFrame::OnCloseWindow)
#ifdef __WXMSW__
   EVT_DROP_FILES     (poEditFrame::OnFileDrop)
#endif
END_EVENT_TABLE()


#ifdef __UNIX__
#include "icons/poedit.xpm"
#endif

// Frame class:

poEditFrame::poEditFrame(const wxString& catalog) :
    wxFrame(NULL, -1, _("poEdit"), wxDefaultPosition,
                             wxSize(
                                 wxConfig::Get()->Read(_T("frame_w"), 600),
                                 wxConfig::Get()->Read(_T("frame_h"), 400)),
                             wxDEFAULT_FRAME_STYLE | wxNO_FULL_REPAINT_ON_RESIZE),
    m_catalog(NULL), 
#ifdef USE_TRANSMEM
    m_transMem(NULL),
    m_transMemLoaded(false),
#endif    
    m_list(NULL),
    m_modified(false),
    m_hasObsoleteItems(false),
    m_sel(0), m_selItem(0)
{
    wxConfigBase *cfg = wxConfig::Get();
    
    // VS: a dirty hack of sort -- if this is the only poEdit frame opened,
    //     place it at remembered position, but don't do that if there already
    //     are other frames, because they would overlap and nobody could recognize
    //     that there are many of them
    if (ms_instances.GetCount() == 0)
        Move(cfg->Read(_T("frame_x"), -1), cfg->Read(_T("frame_y"), -1));
 
    m_displayQuotes = (bool)cfg->Read(_T("display_quotes"), (long)false);
    m_displayLines = (bool)cfg->Read(_T("display_lines"), (long)false);
    gs_focusToText = (bool)cfg->Read(_T("focus_to_text"), (long)false);
    gs_shadedList = (bool)cfg->Read(_T("shaded_list"), (long)true);

    SetIcon(wxICON(appicon));

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
    GetMenuBar()->Check(XRCID("menu_shaded"), gs_shadedList);
    
    m_splitter = new wxSplitterWindow(this, -1);
    wxPanel *panel = new wxPanel(m_splitter);

    m_list = new poEditListCtrl(m_splitter, EDC_LIST, 
                                wxDefaultPosition, wxDefaultSize,
                                wxLC_REPORT | wxLC_SINGLE_SEL,
                                m_displayLines);
    m_list->SetFocus();

    m_textOrig = new UnfocusableTextCtrl(panel, EDC_TEXTORIG, wxEmptyString, 
                                wxDefaultPosition, wxDefaultSize, 
                                wxTE_MULTILINE | wxTE_READONLY);
    m_textTrans = new wxTextCtrl(panel, EDC_TEXTTRANS, wxEmptyString, 
                                wxDefaultPosition, wxDefaultSize, 
                                wxTE_MULTILINE);
    
    wxSizer *sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(m_textOrig, 1, wxEXPAND);
    sizer->Add(m_textTrans, 1, wxEXPAND);

    panel->SetAutoLayout(true);
    panel->SetSizer(sizer);
    
    m_splitter->SetMinimumPaneSize(40);
    m_splitter->SplitHorizontally(m_list, panel, cfg->Read(_T("splitter"), 240L));

    m_textTrans->PushEventHandler(new TextctrlHandler(m_list, &m_sel));
    m_list->PushEventHandler(new ListHandler(m_textTrans, this, 
                                             &m_sel, &m_selItem));

    int widths[] = {-1, 200};
    CreateStatusBar(2, wxST_SIZEGRIP);
    wxStatusBar *bar = GetStatusBar();
    m_statusGauge = new wxGauge(bar, -1, 100, wxDefaultPosition, wxDefaultSize, wxGA_SMOOTH);
    bar->SetStatusWidths(2, widths);
    bar->PushEventHandler(new StatusbarHandler(bar, m_statusGauge));
#ifdef __WXMSW__	
	bar->SetSize(-1,-1,-1,-1);
#endif

    if (!catalog.IsEmpty())
        ReadCatalog(catalog);
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

    m_textTrans->PopEventHandler(true/*delete*/);
    m_list->PopEventHandler(true/*delete*/);

    wxSize sz = GetSize();
    wxPoint pos = GetPosition();
    wxConfigBase *cfg = wxConfig::Get();
    cfg->Write(_T("frame_w"), (long)sz.x);
    cfg->Write(_T("frame_h"), (long)sz.y);
    cfg->Write(_T("frame_x"), (long)pos.x);
    cfg->Write(_T("frame_y"), (long)pos.y);
    cfg->Write(_T("splitter"), (long)m_splitter->GetSashPosition());
    cfg->Write(_T("display_quotes"), m_displayQuotes);
    cfg->Write(_T("display_lines"), m_displayLines);
    cfg->Write(_T("shaded_list"), gs_shadedList);

    m_history.Save(*cfg);

#ifdef USE_TRANSMEM    
    if (m_transMem)
        m_transMem->Release();
#endif
    delete m_catalog;
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
        
        if (!m_catalog->Header().Language.empty())
        {
            lang = LookupLanguageCode(m_catalog->Header().Language.c_str());
            if (!m_catalog->Header().Country.empty())
            {
                lang += _T('_');
                lang += LookupCountryCode(m_catalog->Header().Country.c_str());
            }
        }
        if (!lang)
        {
            wxArrayString lngs;
            int index;
            wxStringTokenizer tkn(cfg->Read(_T("TM/languages"), wxEmptyString), _T(":"));

            lngs.Add(_("(none of these)"));
            while (tkn.HasMoreTokens()) lngs.Add(tkn.GetNextToken());
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
    if (m_catalog && m_modified && wxMessageBox(_("Catalog modified. Do you want to save changes?"), _("Save changes"), 
                            wxYES_NO | wxCENTRE | wxICON_QUESTION) == wxYES)
        WriteCatalog(m_fileName);
    Destroy();
}



void poEditFrame::OnOpen(wxCommandEvent&)
{
    UpdateFromTextCtrl();
    if (m_catalog && m_modified && wxMessageBox(_("Catalog modified. Do you want to save changes?"), _("Save changes"), 
                            wxYES_NO | wxCENTRE | wxICON_QUESTION) == wxYES)
        WriteCatalog(m_fileName);

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
    if (m_catalog && m_modified && wxMessageBox(_("Catalog modified. Do you want to save changes?"), _("Save changes"), 
                            wxYES_NO | wxCENTRE | wxICON_QUESTION) == wxYES)
        WriteCatalog(m_fileName);

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
        if (m_catalog && m_modified && wxMessageBox(_("Catalog modified. Do you want to save changes?"), _("Save changes"), 
                            wxYES_NO | wxCENTRE | wxICON_QUESTION) == wxYES)
            WriteCatalog(m_fileName);
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



void poEditFrame::OnSaveAs(wxCommandEvent&)
{
    UpdateFromTextCtrl();

    wxString name(wxFileNameFromPath(m_fileName));

    if (name.empty())
    {
        if (!m_catalog->Header().Language.empty())
        {
            name = LookupLanguageCode(m_catalog->Header().Language);
            if (!name.empty() && !m_catalog->Header().Country.empty())
            {
                wxString code = LookupCountryCode(m_catalog->Header().Country);
                if (!code.empty())
                    name += wxString(wxT('_')) + code;
            }
        }
        if (!name.empty())
            name += _T(".po");
        else
            name = _T("default.po");
    }
    
    name = wxFileSelector(_("Save as..."), wxPathOnly(m_fileName), name, wxEmptyString, 
                          _("GNU GetText catalogs (*.po)|*.po|All files (*.*)|*.*"),
                          wxSAVE | wxOVERWRITE_PROMPT, this);
    if (!name.IsEmpty())
    {
        wxConfig::Get()->Write(_T("last_file_path"), wxPathOnly(name));
        WriteCatalog(name);
    }
}



void poEditFrame::OnNew(wxCommandEvent& event)
{
    UpdateFromTextCtrl();
    if (m_catalog && m_modified && wxMessageBox(_("Catalog modified. Do you want to save changes?"), _("Save changes"), 
                            wxYES_NO | wxCENTRE | wxICON_QUESTION) == wxYES)
        WriteCatalog(m_fileName);

    SettingsDialog dlg(this);
    Catalog *catalog = new Catalog;
    
    dlg.TransferTo(catalog);
    if (dlg.ShowModal() == wxID_OK)
    {
        dlg.TransferFrom(catalog);
        if (m_catalog) delete m_catalog;
        m_catalog = catalog;
        m_fileName = wxEmptyString;
        m_modified = true;
        OnSave(event);
        OnUpdate(event);
    }
    else delete catalog;
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
}



void poEditFrame::OnSettings(wxCommandEvent&)
{
    SettingsDialog dlg(this);
    
    dlg.TransferTo(m_catalog);
    if (dlg.ShowModal() == wxID_OK)
    {
        dlg.TransferFrom(m_catalog);
        m_modified = true;
        UpdateTitle();
        UpdateMenu();
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
    }
}



void poEditFrame::UpdateCatalog(const wxString& pot_file)
{
    UpdateFromTextCtrl();
    bool succ;
    if (pot_file.empty())
        succ = m_catalog->Update();
    else
        succ = m_catalog->UpdateFromPOT(pot_file);
    
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
    }
    
    UpdateCatalog(pot_file);
 
#ifdef USE_TRANSMEM
    if (wxConfig::Get()->Read(_T("use_tm_when_updating"), true) &&
        GetTransMem() != NULL)
    {
        TranslationMemory *tm = GetTransMem();
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
    }
#endif

    RefreshControls();
}



void poEditFrame::OnListSel(wxListEvent& event)
{
    UpdateToTextCtrl(event.GetIndex());
    event.Skip();
}



void poEditFrame::OnListDesel(wxListEvent& event)
{
    UpdateFromTextCtrl(event.GetIndex());
    event.Skip();
}



void poEditFrame::OnReferencesMenu(wxCommandEvent& event)
{
    if (m_selItem < 0 || m_selItem >= (int)m_catalog->GetCount()) return;
    
    const wxArrayString& refs = (*m_catalog)[m_selItem].GetReferences();

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
                                 (*m_catalog)[m_selItem].GetReferences()[num]);
    }
    else
    {
        wxWindow *w = new FileViewer(this, basepath,
                                     (*m_catalog)[m_selItem].GetReferences(),
                                     num);
        w->Show(true);
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



void poEditFrame::OnShadedListFlag(wxCommandEvent& event)
{
    gs_shadedList = GetMenuBar()->IsChecked(XRCID("menu_shaded"));
    RefreshControls();
}



void poEditFrame::OnInsertOriginal(wxCommandEvent& event)
{
    m_textTrans->SetValue(m_textOrig->GetValue());
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
        f = new FindFrame(this, m_list, m_catalog);
    f->Show(true);
}


static int GetItemIcon(const CatalogData& item)
{
    if (item.HasComment())
    {
        if (item.IsModified())
            return 4;
        else
            return 3;
    }
    if (item.IsModified()) 
        return 1;
    if (item.IsAutomatic())
        return 2;
    return 0;
}

void poEditFrame::UpdateFromTextCtrl(int item)
{
    if (m_catalog == NULL) return;
    if (item == -1) item = m_sel;
    if (m_sel == -1 || m_sel >= m_list->GetItemCount()) return;
    int ind = m_list->GetItemData(item);
    if (ind >= (int)m_catalog->GetCount()) return;
    
    wxString key = (*m_catalog)[ind].GetString();
    wxString newval = m_textTrans->GetValue();
    bool newfuzzy = GetToolBar()->GetToolState(XRCID("menu_fuzzy"));

    // check if anything changed:
    if (newval == m_edittedTextOrig && 
        (*m_catalog)[ind].IsFuzzy() == newfuzzy)
        return; 

    newval.Replace(_T("\n"), _T(""));
    if (m_displayQuotes)
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

    // convert to UTF-8 using user's environment default charset:
    wxString newvalUtf8 = 
        wxString(newval.wc_str(wxConvLocal), wxConvUTF8);

    m_catalog->Translate(key, newvalUtf8);
    m_list->SetItem(item, 1, newval.substr(0, m_list->GetMaxColChars()));

    CatalogData* data = m_catalog->FindItem(key);

    if (newfuzzy == data->IsFuzzy()) newfuzzy = false;
    data->SetFuzzy(newfuzzy);
    GetToolBar()->ToggleTool(XRCID("menu_fuzzy"), newfuzzy);

    wxListItem listitem;
    data->SetModified(true);
    data->SetAutomatic(false);
    data->SetTranslated(!newval.IsEmpty());
    listitem.SetId(item);
    m_list->GetItem(listitem);

    if (gs_shadedList)
    {
        if (!data->IsTranslated())
            listitem.SetBackgroundColour(g_ItemColourUntranslated[item % 2]);
        else if (data->IsFuzzy())
            listitem.SetBackgroundColour(g_ItemColourFuzzy[item % 2]);
        else
            listitem.SetBackgroundColour(g_ItemColourNormal[item % 2]);
    }
    else
    {
        if (!data->IsTranslated())
            listitem.SetBackgroundColour(g_ItemColourUntranslated[0]);
        else if (data->IsFuzzy())
            listitem.SetBackgroundColour(g_ItemColourFuzzy[0]);
        else
            listitem.SetBackgroundColour(g_ItemColourNormal[0]);
    }
    
    int icon = GetItemIcon(*data);
    m_list->SetItemImage(listitem, icon, icon);
    m_list->SetItem(listitem);
    if (m_modified == false)
    {
        m_modified = true;
        UpdateTitle();
    }

    UpdateStatusBar();
}



void poEditFrame::UpdateToTextCtrl(int item)
{
    if (m_catalog == NULL) return;
    if (item == -1) item = m_sel;
    if (item == -1 || item >= m_list->GetItemCount()) return;
    int ind = m_list->GetItemData(item);
    if (ind >= (int)m_catalog->GetCount()) return;

    wxString quote;
    wxString t_o, t_t;
    if (m_displayQuotes) quote = _T("\""); else quote = wxEmptyString;
    t_o = quote + (*m_catalog)[ind].GetString() + quote;
    t_o.Replace(_T("\\n"), _T("\\n\n"));
    m_textOrig->SetValue(t_o);
    t_t = quote + (*m_catalog)[ind].GetTranslation() + quote;
    t_t.Replace(_T("\\n"), _T("\\n\n"));
    
    // Convert from UTF-8 to environment's default charset:
    wxString t_t2(t_t.wc_str(wxConvUTF8), wxConvLocal);
    if (!t_t2)
        t_t2 = t_t;
    
    m_textTrans->SetValue(t_t2);
    m_edittedTextOrig = t_t2;
    if (m_displayQuotes) 
        m_textTrans->SetInsertionPoint(1);
    GetToolBar()->ToggleTool(XRCID("menu_fuzzy"), (*m_catalog)[ind].IsFuzzy());
    GetMenuBar()->Check(XRCID("menu_fuzzy"), (*m_catalog)[ind].IsFuzzy());
}



void poEditFrame::ReadCatalog(const wxString& catalog)
{
    delete m_catalog;
    m_catalog = new Catalog(catalog);

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
    
    RefreshControls();
    UpdateTitle();
   
    wxFileName fn(m_fileName);
    fn.Normalize(wxPATH_NORM_DOTS | wxPATH_NORM_ABSOLUTE);
    m_history.AddFileToHistory(fn.GetFullPath());
}



static void AddItemsToList(const Catalog& catalog,
                           poEditListCtrl *list, size_t& pos,
                           bool (*filter)(const CatalogData& d),
                           const wxColour *clr)
{
    int clrPos;
    wxListItem listitem;
    size_t cnt = catalog.GetCount();
    size_t maxchars = list->GetMaxColChars();

    for (size_t i = 0; i < cnt; i++)
    {
        if (filter(catalog[i]))
        {
            list->InsertItem(pos,
                             catalog[i].GetString().substr(0, maxchars),
                             GetItemIcon(catalog[i]));
            
            // Convert from UTF-8 to environment's default charset:
            wxString trans = 
                wxString(catalog[i].GetTranslation().wc_str(wxConvUTF8),
                         wxConvLocal);
            if (!trans)
                trans = catalog[i].GetTranslation().substr(0, maxchars);
            
            list->SetItem(pos, 1, trans.substr(0, maxchars));
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
    }
}

static bool CatFilterUntranslated(const CatalogData& d)
{
    return !d.IsTranslated();
}

static bool CatFilterFuzzy(const CatalogData& d)
{
    return d.IsFuzzy() && d.IsTranslated();
}

static bool CatFilterRest(const CatalogData& d)
{
    return !d.IsFuzzy() && d.IsTranslated();
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

    m_list->Freeze();
    m_list->ClearAll();
    m_list->CreateColumns();

    wxString trans;
    size_t pos = 0;
    
    AddItemsToList(*m_catalog, m_list, pos, 
                   CatFilterUntranslated, g_ItemColourUntranslated);
    AddItemsToList(*m_catalog, m_list, pos, 
                   CatFilterFuzzy, g_ItemColourFuzzy);
    AddItemsToList(*m_catalog, m_list, pos, 
                   CatFilterRest, gs_shadedList ? g_ItemColourNormal : NULL);

    m_list->Thaw();
    
    if (m_catalog->GetCount() > 0) 
        m_list->SetItemState(0, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);

    FindFrame *f = (FindFrame*)FindWindow(_T("find_frame"));
    if (f)
        f->Reset(m_catalog);
    
    UpdateTitle();
    UpdateStatusBar();
    Refresh();
}



void poEditFrame::UpdateStatusBar()
{
    int all, fuzzy, untranslated;
    if (m_catalog)
    {
        wxString txt;
        m_catalog->GetStatistics(&all, &fuzzy, &untranslated);
        txt.Printf(_("%i strings (%i fuzzy, %i not translated)"), 
                   all, fuzzy, untranslated);
        GetStatusBar()->SetStatusText(txt);
        if (all > 0)
            m_statusGauge->SetValue(100 * (all-fuzzy-untranslated) / all);
        else
            m_statusGauge->SetValue(0);
    }
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
        GetToolBar()->EnableTool(XRCID("menu_save"), false);
        GetToolBar()->EnableTool(XRCID("menu_update"), false);
        GetToolBar()->EnableTool(XRCID("menu_fuzzy"), false);
        GetToolBar()->EnableTool(XRCID("menu_comment"), false);
        GetMenuBar()->EnableTop(1, false);
        GetMenuBar()->EnableTop(2, false);
        m_textTrans->Enable(false);
        m_list->Enable(false);
    }
    else
    {
        GetMenuBar()->Enable(XRCID("menu_save"), true);
        GetMenuBar()->Enable(XRCID("menu_saveas"), true);
        GetToolBar()->EnableTool(XRCID("menu_save"), true);
        GetToolBar()->EnableTool(XRCID("menu_fuzzy"), true);
        GetToolBar()->EnableTool(XRCID("menu_comment"), true);
        GetMenuBar()->EnableTop(1, true);
        GetMenuBar()->EnableTop(2, true);
        m_textTrans->Enable(true);
        m_list->Enable(true);
        bool doupdate = m_catalog->Header().SearchPaths.GetCount() > 0;
        GetToolBar()->EnableTool(XRCID("menu_update"), doupdate);
        GetMenuBar()->Enable(XRCID("menu_update"), doupdate);
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
                !dt.GetTranslation().IsEmpty())
                tm->Store(dt.GetString(), dt.GetTranslation());
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
    if (m_selItem < 0 || m_selItem >= (int)m_catalog->GetCount()) return;
    
    CommentDialog dlg(this, (*m_catalog)[m_selItem].GetComment());
    if (dlg.ShowModal() == wxID_OK)
    {
        m_modified = true;
        UpdateTitle();
        (*m_catalog)[m_selItem].SetComment(dlg.GetComment());

        wxListItem listitem;
        listitem.SetId(m_sel);
        m_list->GetItem(listitem);
        int icon = GetItemIcon((*m_catalog)[m_selItem]);
        m_list->SetItemImage(listitem, icon, icon);
        m_list->SetItem(listitem);
    }
}


#ifdef USE_TRANSMEM
void poEditFrame::OnAutoTranslate(wxCommandEvent& event)
{
    int ind = event.GetId() - ED_POPUP_TRANS;
    (*m_catalog)[m_selItem].SetTranslation(m_autoTranslations[ind]);
    UpdateToTextCtrl();
    // VS: This dirty trick ensures proper refresh of everything: 
    m_edittedTextOrig = wxEmptyString;
    UpdateFromTextCtrl();
}
#endif

wxMenu *poEditFrame::GetPopupMenu(size_t item)
{
    if (!m_catalog) return NULL;
    if (item >= (size_t)m_list->GetItemCount()) return NULL;
    
    const wxArrayString& refs = (*m_catalog)[item].GetReferences();
    wxMenu *menu = new wxMenu;

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
    if (GetTransMem())
    {
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
