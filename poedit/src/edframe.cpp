
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

#include <wx/wx.h>
#include <wx/config.h>
#include <wx/html/htmlwin.h>
#include <wx/statline.h>
#include <wx/sizer.h>
#include <wx/fs_mem.h>
#include <wx/datetime.h>
#include <wx/tokenzr.h>
#include <wx/xml/xmlres.h>
#include <wx/settings.h>


#include "catalog.h"
#include "edframe.h"
#include "settingsdlg.h"
#include "prefsdlg.h"
#include "fileviewer.h"

// Event & controls IDs:
enum 
{
    EDC_LIST = 1000,
    EDC_TEXTORIG,
    EDC_TEXTTRANS,
    
    ED_POPUP = 2000
};

// colours used in the list:
static wxColour g_ItemColourUntranslated(0xA5, 0xEA, 0xEF)/*blue*/, 
                g_ItemColourFuzzy(0xF4, 0xF1, 0xC1)/*yellow*/;


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
             #ifdef __WXMSW__
             w -= wxSystemSettings::GetSystemMetric(wxSYS_VSCROLL_X) + 6;
             #endif
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
            KeysHandler(wxListCtrl *list, wxTextCtrl *text, Catalog **catalog, int *sel, int *selitem) :
                     wxEvtHandler(), m_List(list), m_Text(text),
                     m_Catalog(catalog), m_Sel(sel), m_SelItem(selitem) {}

    private:
            void OnKeyDown(wxKeyEvent& event)
            {
                switch (event.KeyCode())
                {
                    case WXK_UP:
                        if (*m_Sel > 0)
                        {
                            m_List->SetItemState(*m_Sel - 1, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
                            m_List->EnsureVisible(*m_Sel - 1);
                        }
                        break;
                    case WXK_DOWN:
                        if (*m_Sel < m_List->GetItemCount() - 1)
                        {
                            m_List->SetItemState(*m_Sel + 1, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
                            m_List->EnsureVisible(*m_Sel + 1);
                        }
                        break;
                    case WXK_PRIOR:
                        {
                            int newy = *m_Sel - 10;
                            if (newy < 0) newy = 0;
                            m_List->SetItemState(newy, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
                            m_List->EnsureVisible(newy);
                        }
                        break;
                    case WXK_NEXT:
                        {
                            int newy = *m_Sel + 10;
                            if (newy >= m_List->GetItemCount()) newy = m_List->GetItemCount() - 1;
                            m_List->SetItemState(newy, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
                            m_List->EnsureVisible(newy);
                        }
                        break;
                    default:
                        event.Skip();
                }
            }

            void OnListSel(wxListEvent& event)
            {
				*m_Sel = event.GetIndex();
                *m_SelItem = m_List->GetItemData(*m_Sel);
                event.Skip();
            }

            void OnSetFocus(wxListEvent& event)
            {
                if (event.GetEventObject() == m_List)
                {
                    m_Text->SetFocus();
                }                    
                else event.Skip();                    
            }

            void OnRightClick(wxMouseEvent& event)
            {
                long item;
                int flags = wxLIST_HITTEST_ONITEM;
                item = m_List->HitTest(event.GetPosition(), flags);
                if (item != -1 && (flags & wxLIST_HITTEST_ONITEM))
                    m_List->SetItemState(item, wxLIST_STATE_SELECTED, 
                                               wxLIST_STATE_SELECTED);
            
                if (m_Catalog && *m_Catalog)
                {   
                    const wxArrayString& refs = (**m_Catalog)[*m_SelItem].GetReferences();
                    wxMenu *menu = new wxMenu;
                    #ifdef __WXGTK__
                    menu->Append(wxID_OK/*anything*/, _("References"));
                    menu->Enable(wxID_OK, FALSE);
                    menu->AppendSeparator();
                    #else
                    menu->SetTitle(_("References"));
                    // not supported by wxGTK :(
                    #endif
                    for (unsigned i = 0; i < refs.GetCount(); i++)
                        menu->Append(ED_POPUP + i, refs[i]);

                    m_List->PopupMenu(menu, event.GetPosition());
                    delete menu;
                }
                else event.Skip();                    
            }

            DECLARE_EVENT_TABLE() 
            
            wxListCtrl *m_List;
            wxTextCtrl *m_Text;
            Catalog **m_Catalog;
            int *m_Sel, *m_SelItem;
};

BEGIN_EVENT_TABLE(KeysHandler, wxEvtHandler)
   EVT_CHAR(KeysHandler::OnKeyDown)
   EVT_LIST_ITEM_SELECTED(EDC_LIST, KeysHandler::OnListSel)
   EVT_SET_FOCUS(KeysHandler::OnSetFocus)
   EVT_RIGHT_DOWN(KeysHandler::OnRightClick)
END_EVENT_TABLE()




BEGIN_EVENT_TABLE(poEditFrame, wxFrame)
   EVT_MENU                 (XMLID("menu_quit"),        poEditFrame::OnQuit)
   EVT_MENU                 (XMLID("menu_help"),       poEditFrame::OnHelp)
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
   
   EVT_MENU_RANGE           (ED_POPUP, ED_POPUP + 100, poEditFrame::OnReference)

   EVT_LIST_ITEM_SELECTED   (EDC_LIST,       poEditFrame::OnListSel)
   EVT_LIST_ITEM_DESELECTED (EDC_LIST,       poEditFrame::OnListDesel)
   EVT_CLOSE                (                poEditFrame::OnCloseWindow)
END_EVENT_TABLE()



#ifdef __UNIX__
#include "bitmaps/appicon.xpm"
#endif

// Frame class:

poEditFrame::poEditFrame(const wxString& title, const wxString& catalog) :
    wxFrame(NULL, -1, title, wxPoint(
                                 wxConfig::Get()->Read("frame_x", -1),
                                 wxConfig::Get()->Read("frame_y", -1)),
                             wxSize(
                                 wxConfig::Get()->Read("frame_w", 600),
                                 wxConfig::Get()->Read("frame_h", 400)),
                             wxDEFAULT_FRAME_STYLE | wxCLIP_CHILDREN),
    m_Catalog(NULL), 
    m_Title(title), 
    m_List(NULL),
    m_Modified(false),
    m_HasObsoleteItems(false),
    m_Sel(0), m_SelItem(0)
{
    wxConfigBase *cfg = wxConfig::Get();
 
    m_DisplayQuotes = (bool)cfg->Read("display_quotes", (long)false);

    SetIcon(wxICON(appicon));

    wxMenuBar *MenuBar = wxTheXmlResource->LoadMenuBar("mainmenu");
    if (MenuBar)
    {
        m_History.UseMenu(MenuBar->GetMenu(MenuBar->FindMenu(_("&File"))));
        SetMenuBar(MenuBar);
        m_History.AddFilesToMenu();
        m_History.Load(*cfg);
    }
    else
    {
        wxLogError("Cannot load main menu from resource, something must have went terribly wrong.");
        wxLog::FlushActive();
    }

    SetToolBar(wxTheXmlResource->LoadToolBar(this, "toolbar"));

    GetToolBar()->ToggleTool(XMLID("menu_quotes"), m_DisplayQuotes);
    GetMenuBar()->Check(XMLID("menu_quotes"), m_DisplayQuotes);
    
    m_Splitter = new wxSplitterWindow(this, -1, );
    wxPanel *panel = new wxPanel(m_Splitter);

    m_List = new poEditListCtrl(m_Splitter, EDC_LIST, wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxLC_SINGLE_SEL);
    
    m_TextOrig = new wxTextCtrl(panel, EDC_TEXTORIG, "", wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE | wxTE_READONLY);
    m_TextTrans = new wxTextCtrl(panel, EDC_TEXTTRANS, "", wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE);
    m_TextTrans->SetFocus();
    
    wxSizer *sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(m_TextOrig, 1, wxEXPAND);
    sizer->Add(m_TextTrans, 1, wxEXPAND);

    panel->SetAutoLayout(true);
    panel->SetSizer(sizer);
    
    m_Splitter->SetMinimumPaneSize(40);
    m_Splitter->SplitHorizontally(m_List, panel, cfg->Read("splitter", 240L));

    KeysHandler *hand = new KeysHandler(m_List, m_TextTrans, NULL, &m_Sel, &m_SelItem);
    m_TextTrans->PushEventHandler(hand);
    m_List->PushEventHandler(new KeysHandler(m_List, m_TextTrans, &m_Catalog, &m_Sel, &m_SelItem));

    CreateStatusBar();

    if (!catalog.IsEmpty())
        ReadCatalog(catalog);
    UpdateMenu();

#if defined(__WXMSW__)
    m_Help.Initialize("poedit.chm");
#elif defined(__UNIX__)
    m_Help.Initialize(wxString(POEDIT_PREFIX) + "/share/poedit/poedit_help.htb");
#endif
}



poEditFrame::~poEditFrame()
{
    m_TextTrans->PopEventHandler(true/*delete*/);
    m_List->PopEventHandler(true/*delete*/);

    wxSize sz = GetSize();
    wxPoint pos = GetPosition();
    wxConfigBase *cfg = wxConfig::Get();
    cfg->Write("frame_w", (long)sz.x);
    cfg->Write("frame_h", (long)sz.y);
    cfg->Write("frame_x", (long)pos.x);
    cfg->Write("frame_y", (long)pos.y);
    cfg->Write("splitter", (long)m_Splitter->GetSashPosition());
    cfg->Write("display_quotes", m_DisplayQuotes);

    m_History.Save(*cfg);
    
    delete m_Catalog;
}


void poEditFrame::OnQuit(wxCommandEvent&)
{
    Close(true);
}



void poEditFrame::OnCloseWindow(wxCloseEvent&)
{
    UpdateFromTextCtrl();
    if (m_Catalog && m_Modified && wxMessageBox(_("Catalog modified. Do you want to save changes?"), _("Save changes"), 
                            wxYES_NO | wxCENTRE | wxICON_QUESTION) == wxYES)
        WriteCatalog(m_FileName);
    Destroy();
}



void poEditFrame::OnOpen(wxCommandEvent&)
{
    UpdateFromTextCtrl();
    if (m_Catalog && m_Modified && wxMessageBox(_("Catalog modified. Do you want to save changes?"), _("Save changes"), 
                            wxYES_NO | wxCENTRE | wxICON_QUESTION) == wxYES)
        WriteCatalog(m_FileName);

    wxString path = wxPathOnly(m_FileName);
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
    if (m_Catalog && m_Modified && wxMessageBox(_("Catalog modified. Do you want to save changes?"), _("Save changes"), 
                            wxYES_NO | wxCENTRE | wxICON_QUESTION) == wxYES)
        WriteCatalog(m_FileName);

    wxString f(m_History.GetHistoryFile(event.GetId() - wxID_FILE1));
    if (f != "" && wxFileExists(f))
        ReadCatalog(f);
    else
        wxLogError(_("File ") + f + _(" doesn't exist!"));
}



void poEditFrame::OnSave(wxCommandEvent& event)
{
    UpdateFromTextCtrl();
    if (m_FileName == "") OnSaveAs(event);
    else
        WriteCatalog(m_FileName);
}



void poEditFrame::OnSaveAs(wxCommandEvent&)
{
    UpdateFromTextCtrl();

    wxString name(wxFileNameFromPath(m_FileName));

    if (name == "") name = "default.po";
    
    name = wxFileSelector(_("Save as..."), wxPathOnly(m_FileName), name, "", 
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
    if (m_Catalog && m_Modified && wxMessageBox(_("Catalog modified. Do you want to save changes?"), _("Save changes"), 
                            wxYES_NO | wxCENTRE | wxICON_QUESTION) == wxYES)
        WriteCatalog(m_FileName);

    SettingsDialog dlg(this);
    Catalog *catalog = new Catalog;
    
    Catalog::HeaderData &dt = catalog->Header();
    
    wxDateTime timenow = wxDateTime::Now();
    int offs = wxDateTime::TimeZone(wxDateTime::Local).GetOffset();
    dt.CreationDate.Printf("%s%s%02i%02i",
                                 timenow.Format("%Y-%m-%d %H:%M").c_str(),
                                 (offs > 0) ? "+" : "-",
                                 offs / 3600, (abs(offs) / 60) % 60);
    dt.RevisionDate = dt.CreationDate;

    dt.Language = "";
    dt.Project = "";
    dt.Team = "";
    dt.TeamEmail = "";
    dt.Charset = "iso8859-1";
    dt.Translator = wxConfig::Get()->Read("translator_name", "");
    dt.TranslatorEmail = wxConfig::Get()->Read("translator_email", "");
    dt.Keywords.Add("_");
    dt.Keywords.Add("gettext");
    dt.Keywords.Add("gettext_noop");
    dt.BasePath = ".";
    
    dlg.TransferTo(catalog);
    if (dlg.ShowModal() == wxID_OK)
    {
        dlg.TransferFrom(catalog);
        if (m_Catalog) delete m_Catalog;
        m_Catalog = catalog;
        m_FileName = "";
        m_Modified = true;
        OnSave(event);
        OnUpdate(event);
    }
    else delete catalog;
    UpdateTitle();
    UpdateStatusBar();
}



void poEditFrame::OnSettings(wxCommandEvent&)
{
    SettingsDialog dlg(this);
    
    dlg.TransferTo(m_Catalog);
    if (dlg.ShowModal() == wxID_OK)
    {
        dlg.TransferFrom(m_Catalog);
        m_Modified = true;
        UpdateTitle();
        UpdateMenu();
    }
}



void poEditFrame::OnPreferences(wxCommandEvent&)
{
    PreferencesDialog dlg(this);
    
    dlg.TransferTo(wxConfig::Get());
    if (dlg.ShowModal() == wxID_OK)
        dlg.TransferFrom(wxConfig::Get());
}



void poEditFrame::OnUpdate(wxCommandEvent&)
{
    UpdateFromTextCtrl();
    m_Modified = m_Catalog->Update() || m_Modified;
    RefreshControls();
}



void poEditFrame::OnListSel(wxListEvent& event)
{
    UpdateToTextCtrl(event.GetIndex());

    if (FindFocus() != m_TextTrans) m_TextTrans->SetFocus();
}



void poEditFrame::OnListDesel(wxListEvent& event)
{
    UpdateFromTextCtrl(event.GetIndex());
}



void poEditFrame::OnReferencesMenu(wxCommandEvent& event)
{
    const wxArrayString& refs = (*m_Catalog)[m_SelItem].GetReferences();

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
        if (result != -1) ShowReference(result);
    }

}


void poEditFrame::OnReference(wxCommandEvent& event)
{
    ShowReference(event.GetId() - ED_POPUP);
}



void poEditFrame::ShowReference(int num)
{
    wxBusyCursor bcur;
    wxStringTokenizer tkn((*m_Catalog)[m_SelItem].GetReferences()[num], ":");
    wxString file(tkn.GetNextToken());
    long linenum;
    tkn.GetNextToken().ToLong(&linenum);
    
    wxString cwd = wxGetCwd();
    if (m_FileName != "") 
    {
        wxString path;
        
        if (wxIsAbsolutePath(m_Catalog->Header().BasePath))
            path = m_Catalog->Header().BasePath;
        else
            path = wxPathOnly(m_FileName) + "/" + m_Catalog->Header().BasePath;;
        
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
    m_DisplayQuotes = GetToolBar()->GetToolState(XMLID("menu_quotes"));
    UpdateToTextCtrl();
}



void poEditFrame::OnInsertOriginal(wxCommandEvent& event)
{
    int ind = m_List->GetItemData(m_Sel);
    if (ind >= (int)m_Catalog->GetCount()) return;

    wxString quote;
    if (m_DisplayQuotes) quote = "\""; else quote = "";

    m_TextTrans->SetValue(quote + (*m_Catalog)[ind].GetString() + quote);
}






void poEditFrame::UpdateFromTextCtrl(int item)
{
    if (m_Catalog == NULL) return;
    if (item == -1) item = m_Sel;
    if (m_Sel == -1 || m_Sel >= m_List->GetItemCount()) return;
    int ind = m_List->GetItemData(item);
    if (ind >= (int)m_Catalog->GetCount()) return;
    
    bool newfuzzy = GetToolBar()->GetToolState(XMLID("menu_fuzzy"));
    wxString key = (*m_Catalog)[ind].GetString();
    wxString newval = m_TextTrans->GetValue();
    newval.Replace("\n", "");
    if (m_DisplayQuotes)
    {
        if (newval[0] == '"') newval.Remove(0, 1);
        if (newval[newval.Length()-1] == '"') newval.RemoveLast();
    }
      
    if (newval != (*m_Catalog)[ind].GetTranslation() ||
        (*m_Catalog)[ind].IsFuzzy() != newfuzzy)
    {
        if (newval[0] == '"') newval.Prepend("\\");
        for (unsigned i = 1; i < newval.Length(); i++)
            if (newval[i] == '"' && newval[i-1] != '\\')
            {
                newval = newval.Mid(0, i) + "\\\"" + newval.Mid(i+1);
                i++;
            }
        m_Catalog->Translate(key, newval);
        m_List->SetItem(item, 1, (*m_Catalog)[ind].GetTranslation());

        CatalogData* data = m_Catalog->FindItem(key);

        if (newfuzzy == data->IsFuzzy()) newfuzzy = false;
        data->SetFuzzy(newfuzzy);
        GetToolBar()->ToggleTool(XMLID("menu_fuzzy"), newfuzzy);

        wxListItem listitem;
        data->SetTranslated(!newval.IsEmpty());
        listitem.SetId(item);
        m_List->GetItem(listitem);
        if (!data->IsTranslated())
            listitem.SetBackgroundColour(g_ItemColourUntranslated);
        else if (data->IsFuzzy())
            listitem.SetBackgroundColour(g_ItemColourFuzzy);
        else
            listitem.SetBackgroundColour(wxSystemSettings::GetSystemColour(wxSYS_COLOUR_LISTBOX));
        m_List->SetItem(listitem);
        if (m_Modified == false)
        {
            m_Modified = true;
            UpdateTitle();
        }
        
        UpdateStatusBar();
    }
}



void poEditFrame::UpdateToTextCtrl(int item)
{
    if (m_Catalog == NULL) return;
    if (item == -1) item = m_Sel;
    if (m_Sel == -1 || m_Sel >= m_List->GetItemCount()) return;
    int ind = m_List->GetItemData(item);
    if (ind >= (int)m_Catalog->GetCount()) return;

    wxString quote;
    wxString t_o, t_t;
    if (m_DisplayQuotes) quote = "\""; else quote = "";
    t_o = quote + (*m_Catalog)[ind].GetString() + quote;
    t_o.Replace("\\n", "\\n\n");
    m_TextOrig->SetValue(t_o);
    t_t = quote + (*m_Catalog)[ind].GetTranslation() + quote;
    t_t.Replace("\\n", "\\n\n");
    m_TextTrans->SetValue(t_t);
    if (m_DisplayQuotes) m_TextTrans->SetInsertionPoint(1);
    GetToolBar()->ToggleTool(XMLID("menu_fuzzy"), (*m_Catalog)[ind].IsFuzzy());
    GetMenuBar()->Check(XMLID("menu_fuzzy"), (*m_Catalog)[ind].IsFuzzy());
}



void poEditFrame::ReadCatalog(const wxString& catalog)
{
    if (m_Catalog) delete m_Catalog;
    m_Catalog = new Catalog(catalog);
    
    m_FileName = catalog;
    m_Modified = false;
    
    RefreshControls();
    UpdateTitle();
    
    m_History.AddFileToHistory(m_FileName);
}



void poEditFrame::RefreshControls()
{
    m_HasObsoleteItems = false;    
    if (!m_Catalog->IsOk())
    {
        wxLogError(_("Error loading message catalog file '") + m_FileName + _("'."));
        m_FileName = "";
        UpdateMenu();
        UpdateTitle();
        delete m_Catalog;
        m_Catalog = NULL;
        return;
    }
    
    wxBusyCursor bcur;
    UpdateMenu();

    m_List->Hide(); //win32 speed-up
    m_List->ClearAll();
    m_List->CreateColumns();

    wxListItem listitem;

    unsigned i, pos = 0;
    
    for (i = 0; i < m_Catalog->GetCount(); i++)
        if (!(*m_Catalog)[i].IsTranslated())
        {
            m_List->InsertItem(pos, (*m_Catalog)[i].GetString(), -1);
            m_List->SetItem(pos, 1, (*m_Catalog)[i].GetTranslation());
            m_List->SetItemData(pos, i);
            listitem.SetId(pos);
            m_List->GetItem(listitem);
            listitem.SetBackgroundColour(g_ItemColourUntranslated);
            m_List->SetItem(listitem);
            pos++;
        }

    for (i = 0; i < m_Catalog->GetCount(); i++)
        if ((*m_Catalog)[i].IsFuzzy())
        {
            m_List->InsertItem(pos, (*m_Catalog)[i].GetString(), -1);
            m_List->SetItem(pos, 1, (*m_Catalog)[i].GetTranslation());
            m_List->SetItemData(pos, i);
            listitem.SetId(pos);
            m_List->GetItem(listitem);
            listitem.SetBackgroundColour(g_ItemColourFuzzy);
            m_List->SetItem(listitem);
            m_HasObsoleteItems = true;
            pos++;
        }

    for (i = 0; i < m_Catalog->GetCount(); i++)
        if ((*m_Catalog)[i].IsTranslated() && !(*m_Catalog)[i].IsFuzzy())
        {
            m_List->InsertItem(pos, (*m_Catalog)[i].GetString(), -1);
            m_List->SetItem(pos, 1, (*m_Catalog)[i].GetTranslation());
            m_List->SetItemData(pos, i);
            pos++;
        }

    m_List->Show();
    if (m_Catalog->GetCount() > 0) 
        m_List->SetItemState(0, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
    
    UpdateTitle();
    UpdateStatusBar();
    Refresh();
}



void poEditFrame::UpdateStatusBar()
{
    int all, fuzzy, untranslated;
    if (m_Catalog)
    {
        wxString txt;
        m_Catalog->GetStatistics(&all, &fuzzy, &untranslated);
        txt.Printf(_("%i strings (%i fuzzy, %i not translated)"), 
                   all, fuzzy, untranslated);
        GetStatusBar()->SetStatusText(txt);
    }
}



void poEditFrame::UpdateTitle()
{
    if (m_Modified)
        SetTitle(m_Title + " : " + m_FileName + _(" (modified)"));
    else
        SetTitle(m_Title + " : " + m_FileName);
}



void poEditFrame::UpdateMenu()
{
    if (m_Catalog == NULL)
    {
        GetMenuBar()->Enable(XMLID("menu_save"), false);
        GetMenuBar()->Enable(XMLID("menu_saveas"), false);
        GetToolBar()->EnableTool(XMLID("menu_save"), false);
        GetToolBar()->EnableTool(XMLID("menu_update"), false);
        GetToolBar()->EnableTool(XMLID("menu_fuzzy"), false);
        GetMenuBar()->EnableTop(1, false);
        GetMenuBar()->EnableTop(2, false);
        m_TextTrans->Enable(false);
        m_List->Enable(false);
    }
    else
    {
        GetMenuBar()->Enable(XMLID("menu_save"), true);
        GetMenuBar()->Enable(XMLID("menu_saveas"), true);
        GetToolBar()->EnableTool(XMLID("menu_save"), true);
        GetToolBar()->EnableTool(XMLID("menu_fuzzy"), true);
        GetMenuBar()->EnableTop(1, true);
        GetMenuBar()->EnableTop(2, true);
        m_TextTrans->Enable(true);
        m_List->Enable(true);
	bool doupdate = m_Catalog->Header().SearchPaths.GetCount() > 0;
	GetToolBar()->EnableTool(XMLID("menu_update"), doupdate);
	GetMenuBar()->Enable(XMLID("menu_update"), doupdate);
    }
}



void poEditFrame::WriteCatalog(const wxString& catalog)
{
    wxBusyCursor bcur;

    Catalog::HeaderData& dt = m_Catalog->Header();
    dt.Translator = wxConfig::Get()->Read("translator_name", dt.Translator);
    dt.TranslatorEmail = wxConfig::Get()->Read("translator_email", dt.TranslatorEmail);

    m_Catalog->Save(catalog);
    m_FileName = catalog;
    m_Modified = false;

    m_History.AddFileToHistory(m_FileName);
    UpdateTitle();
}




void poEditFrame::OnAbout(wxCommandEvent&)
{
    wxDialog dlg;
    wxTheXmlResource->LoadDialog(&dlg, this, "about_box");
    dlg.Centre();
    dlg.ShowModal();
}



void poEditFrame::OnHelp(wxCommandEvent&)
{
    m_Help.DisplayContents();
}

