
/*

    poedit, a wxWindows i18n catalogs editor

    ---------------
      edframe.cpp
    
      Editor frame
    
      (c) Vaclav Slavik, 2000

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
#include <wx/xrc/xmlres.h>
#include <wx/settings.h>


#include "catalog.h"
#include "edapp.h"
#include "edframe.h"
#include "settingsdlg.h"
#include "prefsdlg.h"
#include "fileviewer.h"
#include "findframe.h"
#include "transmem.h"
#include "iso639.h"
#include "progressinfo.h"

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
static wxColour g_ItemColourUntranslated(0xA5, 0xEA, 0xEF)/*blue*/, 
                g_ItemColourFuzzy(0xF4, 0xF1, 0xC1)/*yellow*/;

#include "nothing.xpm"
#include "modified.xpm"
#include "automatic.xpm"

// list control with both columns equally wide:
class poEditListCtrl : public wxListCtrl
{
    public:
       poEditListCtrl(wxWindow *parent,
                      wxWindowID id = -1,
                      const wxPoint &pos = wxDefaultPosition,
                      const wxSize &size = wxDefaultSize,
                      long style = wxLC_ICON,
                      const wxValidator& validator = wxDefaultValidator,
                      const wxString &name = "listctrl")
             : wxListCtrl(parent, id, pos, size, style, validator, name)
        {
            CreateColumns();
            wxImageList *list = new wxImageList(16, 16);
            list->Add(wxBitmap(nothing_xpm));
            list->Add(wxBitmap(modified_xpm));
            list->Add(wxBitmap(automatic_xpm));
            AssignImageList(list, wxIMAGE_LIST_SMALL);
        }
        
        void CreateColumns()
        {
            InsertColumn(0, _("Original string"));
            InsertColumn(1, _("Translation"));
            SizeColumns();
        }

        void SizeColumns()
        {
             int w = GetSize().x;
             w -= wxSystemSettings::GetSystemMetric(wxSYS_VSCROLL_X) + 6;
             SetColumnWidth(0, w / 2);
             SetColumnWidth(1, w - w / 2);
        }

    private:
        DECLARE_EVENT_TABLE()
        void OnSize(wxSizeEvent& event)
        {
            SizeColumns();
        }
};

BEGIN_EVENT_TABLE(poEditListCtrl, wxListCtrl)
   EVT_SIZE(poEditListCtrl::OnSize)
END_EVENT_TABLE()


// special handling of keyboard in listctrl and textctrl
class KeysHandler : public wxEvtHandler
{ 
    public:
        KeysHandler(wxListCtrl *list, wxTextCtrl *text, poEditFrame *frame,
                    int *sel, int *selitem, bool *multiline) :
                 wxEvtHandler(), m_list(list), m_text(text),
                 m_frame(frame), m_sel(sel), m_selItem(selitem),
                 m_multiLine(multiline) {}

    private:
        void OnKeyDown(wxKeyEvent& event)
        {
            switch (event.KeyCode())
            {
                case WXK_UP:
                    if (*m_multiLine && !event.ControlDown())
                        event.Skip();
                    else if (*m_sel > 0)
                    {
                        m_list->SetItemState(*m_sel - 1, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
                        m_list->EnsureVisible(*m_sel - 1);
                    }
                    break;
                case WXK_DOWN:
                    if (*m_multiLine && !event.ControlDown())
                        event.Skip();
                    else if (*m_sel < m_list->GetItemCount() - 1)
                    {
                        m_list->SetItemState(*m_sel + 1, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
                        m_list->EnsureVisible(*m_sel + 1);
                    }
                    break;
                case WXK_PRIOR:
                    {
                        int newy = *m_sel - 10;
                        if (newy < 0) newy = 0;
                        m_list->SetItemState(newy, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
                        m_list->EnsureVisible(newy);
                    }
                    break;
                case WXK_NEXT:
                    {
                        int newy = *m_sel + 10;
                        if (newy >= m_list->GetItemCount()) newy = m_list->GetItemCount() - 1;
                        m_list->SetItemState(newy, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
                        m_list->EnsureVisible(newy);
                    }
                    break;
                default:
                    event.Skip();
            }
        }

        void OnListSel(wxListEvent& event)
        {
			*m_sel = event.GetIndex();
            *m_selItem = m_list->GetItemData(*m_sel);
            event.Skip();
        }

        void OnSetFocus(wxListEvent& event)
        {
            if (event.GetEventObject() == m_list)
            {
                m_text->SetFocus();
            }                    
            else event.Skip();                    
        }

        void OnRightClick(wxMouseEvent& event)
        {
            long item;
            int flags = wxLIST_HITTEST_ONITEM;
            item = m_list->HitTest(event.GetPosition(), flags);
            if (item != -1 && (flags & wxLIST_HITTEST_ONITEM))
                m_list->SetItemState(item, wxLIST_STATE_SELECTED, 
                                           wxLIST_STATE_SELECTED);

            wxMenu *menu = (m_frame) ? 
                           m_frame->GetPopupMenu(*m_selItem) : NULL;
            if (menu)
            {  
                m_list->PopupMenu(menu, event.GetPosition());
                delete menu;
            }
            else event.Skip();                    
        }

        DECLARE_EVENT_TABLE() 

        wxListCtrl *m_list;
        wxTextCtrl *m_text;
        poEditFrame *m_frame;
        int *m_sel, *m_selItem;
        bool *m_multiLine;
};

BEGIN_EVENT_TABLE(KeysHandler, wxEvtHandler)
   EVT_CHAR(KeysHandler::OnKeyDown)
   EVT_LIST_ITEM_SELECTED(EDC_LIST, KeysHandler::OnListSel)
   EVT_SET_FOCUS(KeysHandler::OnSetFocus)
   EVT_RIGHT_DOWN(KeysHandler::OnRightClick)
END_EVENT_TABLE()




BEGIN_EVENT_TABLE(poEditFrame, wxFrame)
   EVT_MENU                 (XMLID("menu_quit"),        poEditFrame::OnQuit)
   EVT_MENU                 (XMLID("menu_help"),        poEditFrame::OnHelp)
   EVT_MENU                 (XMLID("menu_about"),       poEditFrame::OnAbout)
   EVT_MENU                 (XMLID("menu_new"),         poEditFrame::OnNew)
   EVT_MENU                 (XMLID("menu_open"),        poEditFrame::OnOpen)
   EVT_MENU                 (XMLID("menu_save"),        poEditFrame::OnSave)
   EVT_MENU                 (XMLID("menu_saveas"),      poEditFrame::OnSaveAs)
   EVT_MENU_RANGE           (wxID_FILE1, wxID_FILE9,    poEditFrame::OnOpenHist)
   EVT_MENU                 (XMLID("menu_catsettings"), poEditFrame::OnSettings)
   EVT_MENU                 (XMLID("menu_preferences"), poEditFrame::OnPreferences)
   EVT_MENU                 (XMLID("menu_update"),      poEditFrame::OnUpdate)
   EVT_MENU                 (XMLID("menu_fuzzy"),       poEditFrame::OnFuzzyFlag)
   EVT_MENU                 (XMLID("menu_quotes"),      poEditFrame::OnQuotesFlag)
   EVT_MENU                 (XMLID("menu_insert_orig"), poEditFrame::OnInsertOriginal)
   EVT_MENU                 (XMLID("menu_references"),  poEditFrame::OnReferencesMenu)
   EVT_MENU                 (XMLID("menu_fullscreen"),  poEditFrame::OnFullscreen) 
   EVT_MENU                 (XMLID("menu_find"),        poEditFrame::OnFind)
   EVT_MENU_RANGE           (ED_POPUP_REFS, ED_POPUP_REFS + 999, poEditFrame::OnReference)
#ifdef USE_TRANSMEM
   EVT_MENU_RANGE           (ED_POPUP_TRANS, ED_POPUP_TRANS + 999, poEditFrame::OnAutoTranslate)
#endif
   EVT_LIST_ITEM_SELECTED   (EDC_LIST,       poEditFrame::OnListSel)
   EVT_LIST_ITEM_DESELECTED (EDC_LIST,       poEditFrame::OnListDesel)
   EVT_CLOSE                (                poEditFrame::OnCloseWindow)
END_EVENT_TABLE()



#ifdef __UNIX__
#include "appicon.xpm"
#endif

// Frame class:

poEditFrame::poEditFrame(const wxString& title, const wxString& catalog) :
    wxFrame(NULL, -1, title, wxPoint(
                                 wxConfig::Get()->Read("frame_x", -1),
                                 wxConfig::Get()->Read("frame_y", -1)),
                             wxSize(
                                 wxConfig::Get()->Read("frame_w", 600),
                                 wxConfig::Get()->Read("frame_h", 400)),
                             wxDEFAULT_FRAME_STYLE | wxNO_FULL_REPAINT_ON_RESIZE),
    m_catalog(NULL), 
#ifdef USE_TRANSMEM
    m_transMem(NULL),
    m_transMemLoaded(false),
#endif    
    m_title(title), 
    m_list(NULL),
    m_modified(false),
    m_hasObsoleteItems(false),
    m_sel(0), m_selItem(0)
{
    wxConfigBase *cfg = wxConfig::Get();
 
    m_displayQuotes = (bool)cfg->Read("display_quotes", (long)false);
    m_multiLine = cfg->Read("multiline_textctrl", true);

    SetIcon(wxICON(appicon));

#ifdef __WXMSW__
    m_boldGuiFont = wxSystemSettings::GetSystemFont(wxSYS_DEFAULT_GUI_FONT);
    m_boldGuiFont.SetWeight(wxFONTWEIGHT_BOLD);
#endif    
    
    wxMenuBar *MenuBar = wxTheXmlResource->LoadMenuBar("mainmenu");
    if (MenuBar)
    {
        m_history.UseMenu(MenuBar->GetMenu(MenuBar->FindMenu(_("&File"))));
        SetMenuBar(MenuBar);
        m_history.AddFilesToMenu();
        m_history.Load(*cfg);
    }
    else
    {
        wxLogError("Cannot load main menu from resource, something must have went terribly wrong.");
        wxLog::FlushActive();
    }

    SetToolBar(wxTheXmlResource->LoadToolBar(this, "toolbar"));

    GetToolBar()->ToggleTool(XMLID("menu_quotes"), m_displayQuotes);
    GetMenuBar()->Check(XMLID("menu_quotes"), m_displayQuotes);
    
    m_splitter = new wxSplitterWindow(this, -1);
    wxPanel *panel = new wxPanel(m_splitter);

    m_list = new poEditListCtrl(m_splitter, EDC_LIST, wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxLC_SINGLE_SEL);

    m_textOrig = new wxTextCtrl(panel, EDC_TEXTORIG, "", 
                                wxDefaultPosition, wxDefaultSize, 
                                wxTE_MULTILINE | wxTE_READONLY);
    m_textTrans = new wxTextCtrl(panel, EDC_TEXTTRANS, "", 
                                wxDefaultPosition, wxDefaultSize, 
                                wxTE_MULTILINE);
    m_textTrans->SetFocus();
    
    wxSizer *sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(m_textOrig, 1, wxEXPAND);
    sizer->Add(m_textTrans, 1, wxEXPAND);

    panel->SetAutoLayout(true);
    panel->SetSizer(sizer);
    
    m_splitter->SetMinimumPaneSize(40);
    m_splitter->SplitHorizontally(m_list, panel, cfg->Read("splitter", 240L));

    KeysHandler *hand = new KeysHandler(m_list, m_textTrans, NULL, 
                                        &m_sel, &m_selItem, &m_multiLine);
    m_textTrans->PushEventHandler(hand);
    m_list->PushEventHandler(new KeysHandler(m_list, m_textTrans, this, 
                                             &m_sel, &m_selItem, &m_multiLine));

    CreateStatusBar();

    if (!catalog.IsEmpty())
        ReadCatalog(catalog);
    UpdateMenu();

#if defined(__WXMSW__)
    m_help.Initialize(wxGetApp().GetAppPath() + "/share/poedit/poedit.chm");
#elif defined(__UNIX__)
    m_help.Initialize(wxGetApp().GetAppPath() + "/share/poedit/help.zip");
#endif
}



poEditFrame::~poEditFrame()
{
    m_textTrans->PopEventHandler(true/*delete*/);
    m_list->PopEventHandler(true/*delete*/);

    wxSize sz = GetSize();
    wxPoint pos = GetPosition();
    wxConfigBase *cfg = wxConfig::Get();
    cfg->Write("frame_w", (long)sz.x);
    cfg->Write("frame_h", (long)sz.y);
    cfg->Write("frame_x", (long)pos.x);
    cfg->Write("frame_y", (long)pos.y);
    cfg->Write("splitter", (long)m_splitter->GetSashPosition());
    cfg->Write("display_quotes", m_displayQuotes);

    m_history.Save(*cfg);

#ifdef USE_TRANSMEM    
    delete m_transMem;
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
        wxString dbPath = cfg->Read("TM/database_path", "");
        
        if (!m_catalog->Header().Language.IsEmpty())
        {
            const char *lng = m_catalog->Header().Language.c_str();
            for (size_t i = 0; isoLanguages[i].iso != NULL; i++)
            {
                if (strcmp(isoLanguages[i].lang, lng) == 0)
                {
                    lang = isoLanguages[i].iso;
                    break;
                }
            }
        }
        if (!lang)
        {
            wxArrayString lngs;
            int index;
            wxStringTokenizer tkn(cfg->Read("TM/languages", ""), ":");

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
                m_transMem->SetParams(cfg->Read("TM/max_delta", 2),
                                      cfg->Read("TM/max_omitted", 2));
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
    if (path.IsEmpty()) 
        path = wxConfig::Get()->Read("last_file_path", "");
	
    wxString name = wxFileSelector(_("Open catalog"), 
                    path, "", "", 
                    "GNU GetText catalogs (*.po)|*.po|All files (*.*)|*.*", 
                    wxOPEN | wxFILE_MUST_EXIST, this);
    if (!name.IsEmpty())
    {
        wxConfig::Get()->Write("last_file_path", wxPathOnly(name));
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
    if (f != "" && wxFileExists(f))
        ReadCatalog(f);
    else
        wxLogError(_("File ") + f + _(" doesn't exist!"));
}



void poEditFrame::OnSave(wxCommandEvent& event)
{
    UpdateFromTextCtrl();
    if (m_fileName == "") OnSaveAs(event);
    else
        WriteCatalog(m_fileName);
}



void poEditFrame::OnSaveAs(wxCommandEvent&)
{
    UpdateFromTextCtrl();

    wxString name(wxFileNameFromPath(m_fileName));

    if (name == "") name = "default.po";
    
    name = wxFileSelector(_("Save as..."), wxPathOnly(m_fileName), name, "", 
                          "GNU GetText catalogs (*.po)|*.po|All files (*.*)|*.*",
                          wxSAVE | wxOVERWRITE_PROMPT, this);
    if (!name.IsEmpty())
    {
        wxConfig::Get()->Write("last_file_path", wxPathOnly(name));
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
        m_fileName = "";
        m_modified = true;
        OnSave(event);
        OnUpdate(event);
    }
    else delete catalog;
    UpdateTitle();
    UpdateStatusBar();

#ifdef USE_TRANSMEM
    delete m_transMem;
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
        m_multiLine = wxConfig::Get()->Read("multiline_textctrl", true);
            // refresh so that textctrl changes behavior
    }
}



void poEditFrame::OnUpdate(wxCommandEvent&)
{
    UpdateFromTextCtrl();
    m_modified = m_catalog->Update() || m_modified;

#ifdef USE_TRANSMEM
    if (wxConfig::Get()->Read("use_tm_when_updating", true) &&
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
                    dt.SetFuzzy(score != 100);
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
    if (FindFocus() != m_textTrans) 
        m_textTrans->SetFocus();
}



void poEditFrame::OnListDesel(wxListEvent& event)
{
    UpdateFromTextCtrl(event.GetIndex());
}



void poEditFrame::OnReferencesMenu(wxCommandEvent& event)
{
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
    wxStringTokenizer tkn((*m_catalog)[m_selItem].GetReferences()[num], ":");
    wxString file(tkn.GetNextToken());
    long linenum;
    tkn.GetNextToken().ToLong(&linenum);
    
    wxString cwd = wxGetCwd();
    if (m_fileName != "") 
    {
        wxString path;
        
        if (wxIsAbsolutePath(m_catalog->Header().BasePath))
            path = m_catalog->Header().BasePath;
        else
            path = wxPathOnly(m_fileName) + "/" + m_catalog->Header().BasePath;;
        
        if (wxIsAbsolutePath(path))
            wxSetWorkingDirectory(path);
        else
            wxSetWorkingDirectory(cwd + "/" + path);
    }
    
    
    (new FileViewer(this, file, linenum))->Show(true);
    
    wxSetWorkingDirectory(cwd);
}



void poEditFrame::OnFuzzyFlag(wxCommandEvent& event)
{
    if (event.GetEventObject() == GetToolBar())
    {
        GetMenuBar()->Check(XMLID("menu_fuzzy"), 
                            GetToolBar()->GetToolState(XMLID("menu_fuzzy")));
    }
    else
    {
        GetToolBar()->ToggleTool(XMLID("menu_fuzzy"),  
                                 GetMenuBar()->IsChecked(XMLID("menu_fuzzy")));
    }
    UpdateFromTextCtrl();
}



void poEditFrame::OnQuotesFlag(wxCommandEvent& event)
{
    UpdateFromTextCtrl();
    if (event.GetEventObject() == GetToolBar())
        GetMenuBar()->Check(XMLID("menu_quotes"), 
                            GetToolBar()->GetToolState(XMLID("menu_quotes")));
    else
        GetToolBar()->ToggleTool(XMLID("menu_quotes"),  
                                 GetMenuBar()->IsChecked(XMLID("menu_quotes")));
    m_displayQuotes = GetToolBar()->GetToolState(XMLID("menu_quotes"));
    UpdateToTextCtrl();
}



void poEditFrame::OnInsertOriginal(wxCommandEvent& event)
{
    int ind = m_list->GetItemData(m_sel);
    if (ind >= (int)m_catalog->GetCount()) return;

    wxString quote;
    if (m_displayQuotes) quote = "\""; else quote = "";

    m_textTrans->SetValue(quote + (*m_catalog)[ind].GetString() + quote);
}



void poEditFrame::OnFullscreen(wxCommandEvent& event)
{
    bool fs = IsFullScreen();
    wxConfigBase *cfg = wxConfigBase::Get();

    GetMenuBar()->Check(XMLID("menu_fullscreen"), !fs);
    GetToolBar()->ToggleTool(XMLID("menu_fullscreen"), !fs);

    if (fs)
    {
        cfg->Write("splitter_fullscreen", (long)m_splitter->GetSashPosition());
        m_splitter->SetSashPosition(cfg->Read("splitter", 240L));
    }
    else
    {
        long oldSash = m_splitter->GetSashPosition();
        cfg->Write("splitter", oldSash);
        m_splitter->SetSashPosition(cfg->Read("splitter_fullscreen", oldSash));
    }

    ShowFullScreen(!fs, wxFULLSCREEN_NOBORDER | wxFULLSCREEN_NOCAPTION);
}



void poEditFrame::OnFind(wxCommandEvent& event)
{
    FindFrame *f = (FindFrame*)FindWindow("find_frame");

    if (!f)
        f = new FindFrame(this, m_list, m_catalog);
    f->Show(true);
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
    bool newfuzzy = GetToolBar()->GetToolState(XMLID("menu_fuzzy"));

    // check if anything changed:
    if (newval == m_edittedTextOrig && 
        (*m_catalog)[ind].IsFuzzy() == newfuzzy)
        return; 

    newval.Replace("\n", "");
    if (m_displayQuotes)
    {
        if (newval[0u] == '"') newval.Remove(0, 1);
        if (newval[newval.Length()-1] == '"') newval.RemoveLast();
    }
      
    if (newval[0u] == '"') newval.Prepend("\\");
    for (unsigned i = 1; i < newval.Length(); i++)
        if (newval[i] == '"' && newval[i-1] != '\\')
        {
            newval = newval.Mid(0, i) + "\\\"" + newval.Mid(i+1);
            i++;
        }

    // convert to UTF-8 using user's environment default charset:
    wxString newvalUtf8 = 
        wxString(newval.wc_str(wxConvLocal), wxConvUTF8);

    m_catalog->Translate(key, newvalUtf8);
    m_list->SetItem(item, 1, newval);

    CatalogData* data = m_catalog->FindItem(key);

    if (newfuzzy == data->IsFuzzy()) newfuzzy = false;
    data->SetFuzzy(newfuzzy);
    GetToolBar()->ToggleTool(XMLID("menu_fuzzy"), newfuzzy);

    wxListItem listitem;
    data->SetModified(true);
    data->SetAutomatic(false);
    data->SetTranslated(!newval.IsEmpty());
    listitem.SetId(item);
    m_list->GetItem(listitem);
    if (!data->IsTranslated())
        listitem.SetBackgroundColour(g_ItemColourUntranslated);
    else if (data->IsFuzzy())
        listitem.SetBackgroundColour(g_ItemColourFuzzy);
    else
        listitem.SetBackgroundColour(wxSystemSettings::GetSystemColour(wxSYS_COLOUR_LISTBOX));
    m_list->SetItemImage(listitem, 1, 1);
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
    if (m_displayQuotes) quote = "\""; else quote = "";
    t_o = quote + (*m_catalog)[ind].GetString() + quote;
    t_o.Replace("\\n", "\\n\n");
    m_textOrig->SetValue(t_o);
    t_t = quote + (*m_catalog)[ind].GetTranslation() + quote;
    t_t.Replace("\\n", "\\n\n");
    
    // Convert from UTF-8 to environment's default charset:
    wxString t_t2(t_t.wc_str(wxConvUTF8), wxConvLocal);
    if (!t_t2)
        t_t2 = t_t;
    
    m_textTrans->SetValue(t_t2);
    m_edittedTextOrig = t_t2;
    if (m_displayQuotes) 
        m_textTrans->SetInsertionPoint(1);
    GetToolBar()->ToggleTool(XMLID("menu_fuzzy"), (*m_catalog)[ind].IsFuzzy());
    GetMenuBar()->Check(XMLID("menu_fuzzy"), (*m_catalog)[ind].IsFuzzy());
}



void poEditFrame::ReadCatalog(const wxString& catalog)
{
    delete m_catalog;
    m_catalog = new Catalog(catalog);

#ifdef USE_TRANSMEM
    delete m_transMem;
    m_transMemLoaded = false;
#endif
    
    m_fileName = catalog;
    m_modified = false;
    
    RefreshControls();
    UpdateTitle();
    
    m_history.AddFileToHistory(m_fileName);
}



void poEditFrame::RefreshControls()
{
    m_hasObsoleteItems = false;    
    if (!m_catalog->IsOk())
    {
        wxLogError(_("Error loading message catalog file '") + m_fileName + _("'."));
        m_fileName = "";
        UpdateMenu();
        UpdateTitle();
        delete m_catalog;
        m_catalog = NULL;
        return;
    }
    
    wxBusyCursor bcur;
    UpdateMenu();

    m_list->Hide(); //win32 speed-up
    m_list->ClearAll();
    m_list->CreateColumns();

    wxListItem listitem;
    wxString trans;
    size_t i, pos = 0;
    size_t cnt = m_catalog->GetCount();
    
    for (i = 0; i < cnt; i++)
        if (!(*m_catalog)[i].IsTranslated())
        {
            m_list->InsertItem(pos, (*m_catalog)[i].GetString(), 0);
            
            // Convert from UTF-8 to environment's default charset:
            trans = 
                wxString((*m_catalog)[i].GetTranslation().wc_str(wxConvUTF8), 
                         wxConvLocal);
            if (!trans)
                trans = (*m_catalog)[i].GetTranslation();
            
            m_list->SetItem(pos, 1, trans);
            m_list->SetItemData(pos, i);
            listitem.SetId(pos);
            m_list->GetItem(listitem);
            listitem.SetBackgroundColour(g_ItemColourUntranslated);
            m_list->SetItem(listitem);
            pos++;
        }

    for (i = 0; i < cnt; i++)
        if ((*m_catalog)[i].IsFuzzy())
        {
            m_list->InsertItem(pos, (*m_catalog)[i].GetString(),
                               (*m_catalog)[i].IsAutomatic() ? 2 : 0);

            // Convert from UTF-8 to environment's default charset:
            trans = 
                wxString((*m_catalog)[i].GetTranslation().wc_str(wxConvUTF8), 
                         wxConvLocal);
            if (!trans)
                trans = (*m_catalog)[i].GetTranslation();

            m_list->SetItem(pos, 1, trans);
            m_list->SetItemData(pos, i);
            listitem.SetId(pos);
            m_list->GetItem(listitem);
            listitem.SetBackgroundColour(g_ItemColourFuzzy);
            m_list->SetItem(listitem);
            m_hasObsoleteItems = true;
            pos++;
        }

    for (i = 0; i < cnt; i++)
        if ((*m_catalog)[i].IsTranslated() && !(*m_catalog)[i].IsFuzzy())
        {
            m_list->InsertItem(pos, (*m_catalog)[i].GetString(),
                               (*m_catalog)[i].IsAutomatic() ? 2 : 0);

            // Convert from UTF-8 to environment's default charset:
            trans = 
                wxString((*m_catalog)[i].GetTranslation().wc_str(wxConvUTF8), 
                         wxConvLocal);
            if (!trans)
                trans = (*m_catalog)[i].GetTranslation();

            m_list->SetItem(pos, 1, trans);
            m_list->SetItemData(pos, i);
            pos++;
        }

    m_list->Show();
    if (cnt > 0) 
        m_list->SetItemState(0, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);

    FindFrame *f = (FindFrame*)FindWindow("find_frame");
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
    }
}



void poEditFrame::UpdateTitle()
{
    if (m_modified)
        SetTitle(m_title + " : " + m_fileName + _(" (modified)"));
    else
        SetTitle(m_title + " : " + m_fileName);
}



void poEditFrame::UpdateMenu()
{
    if (m_catalog == NULL)
    {
        GetMenuBar()->Enable(XMLID("menu_save"), false);
        GetMenuBar()->Enable(XMLID("menu_saveas"), false);
        GetToolBar()->EnableTool(XMLID("menu_save"), false);
        GetToolBar()->EnableTool(XMLID("menu_update"), false);
        GetToolBar()->EnableTool(XMLID("menu_fuzzy"), false);
        GetMenuBar()->EnableTop(1, false);
        GetMenuBar()->EnableTop(2, false);
        m_textTrans->Enable(false);
        m_list->Enable(false);
    }
    else
    {
        GetMenuBar()->Enable(XMLID("menu_save"), true);
        GetMenuBar()->Enable(XMLID("menu_saveas"), true);
        GetToolBar()->EnableTool(XMLID("menu_save"), true);
        GetToolBar()->EnableTool(XMLID("menu_fuzzy"), true);
        GetMenuBar()->EnableTop(1, true);
        GetMenuBar()->EnableTop(2, true);
        m_textTrans->Enable(true);
        m_list->Enable(true);
        bool doupdate = m_catalog->Header().SearchPaths.GetCount() > 0;
        GetToolBar()->EnableTool(XMLID("menu_update"), doupdate);
        GetMenuBar()->Enable(XMLID("menu_update"), doupdate);
    }
}



void poEditFrame::WriteCatalog(const wxString& catalog)
{
    wxBusyCursor bcur;

    Catalog::HeaderData& dt = m_catalog->Header();
    dt.Translator = wxConfig::Get()->Read("translator_name", dt.Translator);
    dt.TranslatorEmail = wxConfig::Get()->Read("translator_email", dt.TranslatorEmail);

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

#ifdef __WXMSW__
    wxMenuItem *it1 = new wxMenuItem(menu, ED_POPUP_DUMMY+0, _("References:"));
    it1->SetFont(m_boldGuiFont);
    menu->Append(it1);
#else
    menu->Append(ED_POPUP_DUMMY+0, _("References:"));
#endif    
    menu->AppendSeparator();
    for (size_t i = 0; i < refs.GetCount(); i++)
        menu->Append(ED_POPUP_REFS + i, refs[i]);

#ifdef USE_TRANSMEM
    menu->AppendSeparator();
#ifdef __WXMSW__
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
                menu->Append(ED_POPUP_TRANS + i, s);
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
    wxTheXmlResource->LoadDialog(&dlg, this, "about_box");
    dlg.Centre();
    dlg.ShowModal();
}



void poEditFrame::OnHelp(wxCommandEvent&)
{
    m_help.DisplayContents();
}

