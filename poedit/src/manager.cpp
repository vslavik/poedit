
/*

    poedit, a wxWindows i18n catalogs editor

    ---------------
      manager.cpp
    
      Catalogs manager
    
      (c) Vaclav Slavik, 2001

*/


#ifdef __GNUG__
#pragma implementation
#endif

#include <wx/wxprec.h>

#include <wx/imaglist.h>
#include <wx/config.h>
#include <wx/textctrl.h>
#include <wx/listctrl.h>
#include <wx/xrc/xmlres.h>
#include <wx/settings.h>
#include <wx/intl.h>
#include <wx/frame.h>
#include <wx/gizmos/editlbox.h>
#include <wx/tokenzr.h>
#include <wx/msgdlg.h>
#include <wx/button.h>
#include <wx/dir.h>
#include <wx/imaglist.h>
#include <wx/dirdlg.h>
#include <wx/log.h>

#include "catalog.h"
#include "edframe.h"
#include "manager.h"

#ifdef __UNIX__
#include "appicon.xpm"
#endif

ManagerFrame *ManagerFrame::ms_instance = NULL;

/*static*/ ManagerFrame *ManagerFrame::Create()
{
    if (!ms_instance)
    {
        ms_instance = new ManagerFrame;
        ms_instance->Show(true);
    }
    return ms_instance;
}

#include "cat_ok.xpm"
#include "cat_mid.xpm"
#include "cat_no.xpm"

ManagerFrame::ManagerFrame() :
    wxFrame(NULL, -1, _("poEdit - Catalogs manager"), wxPoint(
                                 wxConfig::Get()->Read("manager_x", -1),
                                 wxConfig::Get()->Read("manager_y", -1)),
                             wxSize(
                                 wxConfig::Get()->Read("manager_w", 400),
                                 wxConfig::Get()->Read("manager_h", 300)),
                             wxDEFAULT_FRAME_STYLE | wxNO_FULL_REPAINT_ON_RESIZE)
{
    SetIcon(wxICON(appicon));
    
    ms_instance = this;

    SetToolBar(wxTheXmlResource->LoadToolBar(this, _T("manager_toolbar")));
    
    wxPanel *panel = wxTheXmlResource->LoadPanel(this, _T("manager_panel"));
    
    m_listPrj = XMLCTRL(*panel, "prj_list", wxListBox);
    m_listCat = XMLCTRL(*panel, "prj_files", wxListCtrl);
    
    wxImageList *list = new wxImageList(16, 16);
    list->Add(wxBitmap(cat_no_xpm));
    list->Add(wxBitmap(cat_mid_xpm));
    list->Add(wxBitmap(cat_ok_xpm));
    m_listCat->AssignImageList(list, wxIMAGE_LIST_SMALL);

    m_curPrj = -1;

    int last = wxConfig::Get()->Read(_T("manager_last_selected"), (long)0);
    UpdateListPrj(last);
    if (m_listPrj->GetCount() > 0)
        UpdateListCat(last);
}



ManagerFrame::~ManagerFrame()
{
    wxSize sz = GetSize();
    wxPoint pos = GetPosition();
    wxConfigBase *cfg = wxConfig::Get();
    cfg->Write(_T("manager_w"), (long)sz.x);
    cfg->Write(_T("manager_h"), (long)sz.y);
    cfg->Write(_T("manager_x"), (long)pos.x);
    cfg->Write(_T("manager_y"), (long)pos.y);
	
	int sel = m_listPrj->GetSelection();
	if (sel != -1)
        cfg->Write(_T("manager_last_selected"), (long)m_listPrj->GetClientData(sel));
    
    ms_instance = NULL;
}


void ManagerFrame::UpdateListPrj(int select)
{
    wxConfigBase *cfg = wxConfig::Get();
    int max = cfg->Read(_T("Manager/max_project_num"), (long)0) + 1;
    wxString key, name;
    
    m_listPrj->Clear();
    int item = 0;
    for (int i = 0; i <= max; i++)
    {
        key.Printf(_T("Manager/project_%i/Name"), i);
        name = cfg->Read(key, wxEmptyString);
        if (!name.IsEmpty())
        {
            m_listPrj->Append(name, (void*)i);
            if (i == select)
            {
                m_listPrj->SetSelection(item);
                m_curPrj = select;
                select = -1;
            }
            item++;
        }
    }
}


static void AddCatalogToList(wxListCtrl *list, int i, int id, const wxString& file)
{
    wxConfigBase *cfg = wxConfig::Get();
    int all = 0, fuzzy = 0, untranslated = 0;
    wxString lastmodified;
    time_t modtime;
    wxString key;
    wxString file2(file);
    file2.Replace(_T("/"), _T("_"));
    file2.Replace(_T("\\"), _T("_"));
    key.Printf(_T("Manager/project_%i/FilesCache/%s/"), id, file2.c_str());

    modtime = cfg->Read(key + _T("timestamp"), (long)0);
    
    if (modtime == wxFileModificationTime(file))
    {
        all = cfg->Read(key + _T("all"), (long)0);
        fuzzy = cfg->Read(key + _T("fuzzy"), (long)0);
        untranslated = cfg->Read(key + _T("untranslated"), (long)0);
        lastmodified = cfg->Read(key + _T("lastmodified"), _T("?"));
    }
    else
    {
        // supress error messages, we don't mind if the catalog is corrupted
        wxLogNull nullLog;
        
        Catalog cat(file);
        cat.GetStatistics(&all, &fuzzy, &untranslated);
        modtime = wxFileModificationTime(file);
        lastmodified = cat.Header().RevisionDate;
        cfg->Write(key + _T("timestamp"), modtime);
        cfg->Write(key + _T("all"), (long)all);
        cfg->Write(key + _T("fuzzy"), (long)fuzzy);
        cfg->Write(key + _T("untranslated"), (long)untranslated);
        cfg->Write(key + _T("lastmodified"), lastmodified);
    }
    
    int icon;
    if (fuzzy+untranslated == 0) icon = 2;
    else if ((double)all / (fuzzy+untranslated) <= 3) icon = 0;
    else icon = 1;
    
    wxString tmp;
    list->InsertItem(i, file, icon);
    tmp.Printf(_T("%i"), all);
    list->SetItem(i, 1, tmp);
    tmp.Printf(_T("%i"), untranslated);
    list->SetItem(i, 2, tmp);
    tmp.Printf(_T("%i"), fuzzy);    
    list->SetItem(i, 3, tmp);
    list->SetItem(i, 4, lastmodified);
}

void ManagerFrame::UpdateListCat(int id)
{
    wxBusyCursor bcur;
    
    if (id == -1) id = m_curPrj;

    wxConfigBase *cfg = wxConfig::Get();
    wxString key;
    key.Printf(_T("Manager/project_%i/"), id);

    wxString dirs = cfg->Read(key + _T("Dirs"), wxEmptyString);
    wxStringTokenizer tkn(dirs, wxPATH_SEP);
    
    m_catalogs.Clear();
    while (tkn.HasMoreTokens())
        wxDir::GetAllFiles(tkn.GetNextToken(), &m_catalogs, 
                           _T("*.po"), wxDIR_FILES);

    m_listCat->Freeze();

    m_listCat->ClearAll();
    m_listCat->InsertColumn(0, _("Catalog"));
    m_listCat->InsertColumn(1, _("Total"));
    m_listCat->InsertColumn(2, _("Untrans"));
    m_listCat->InsertColumn(3, _("Fuzzy"));
    m_listCat->InsertColumn(4, _("Last modified"));
    
    for (size_t i = 0; i < m_catalogs.GetCount(); i++)
        AddCatalogToList(m_listCat, i, id, m_catalogs[i]);
    
    m_listCat->SetColumnWidth(0, wxLIST_AUTOSIZE);
    m_listCat->SetColumnWidth(1, wxLIST_AUTOSIZE_USEHEADER);
    m_listCat->SetColumnWidth(2, wxLIST_AUTOSIZE_USEHEADER);
    m_listCat->SetColumnWidth(3, wxLIST_AUTOSIZE_USEHEADER);
    m_listCat->SetColumnWidth(4, wxLIST_AUTOSIZE);
    
    m_listCat->Thaw();
}


class ProjectDlg : public wxDialog
{
    protected:
        DECLARE_EVENT_TABLE()
        void OnBrowse(wxCommandEvent& event);
};

BEGIN_EVENT_TABLE(ProjectDlg, wxDialog)
   EVT_BUTTON(XMLID("adddir"), ProjectDlg::OnBrowse)
END_EVENT_TABLE()

void ProjectDlg::OnBrowse(wxCommandEvent& event)
{
    wxDirDialog dlg(this, _("Select directory"));
    if (dlg.ShowModal() == wxID_OK)
    {
        wxArrayString a;
        wxEditableListBox *l = XMLCTRL(*this, "prj_dirs", wxEditableListBox);
        l->GetStrings(a);
        a.Add(dlg.GetPath());
        l->SetStrings(a);
    }
}

bool ManagerFrame::EditProject(int id)
{
    wxConfigBase *cfg = wxConfig::Get();
    wxString key;
    key.Printf(_T("Manager/project_%i/"), id);
    
    ProjectDlg dlg;
    wxTheXmlResource->LoadDialog(&dlg, this, _T("manager_prj_dlg"));
    wxTheXmlResource->AttachUnknownControl(_T("prj_dirs"), 
                new wxEditableListBox(this, -1, _("Directories:")));
    
    XMLCTRL(dlg, "prj_name", wxTextCtrl)->SetValue(
           cfg->Read(key + _T("Name"), _("My Project")));
           
    wxString dirs = cfg->Read(key + _T("Dirs"), wxGetCwd());
    wxArrayString adirs;
    wxStringTokenizer tkn(dirs, wxPATH_SEP);
    while (tkn.HasMoreTokens()) adirs.Add(tkn.GetNextToken());
    XMLCTRL(*this, "prj_dirs", wxEditableListBox)->SetStrings(adirs);
           
    if (dlg.ShowModal() == wxID_OK)
    {
        cfg->Write(key + _T("Name"), 
                   XMLCTRL(dlg, "prj_name", wxTextCtrl)->GetValue());
        XMLCTRL(*this, "prj_dirs", wxEditableListBox)->GetStrings(adirs);
        if (adirs.GetCount() > 0)
            dirs = adirs[0];
        for (size_t i = 1; i < adirs.GetCount(); i++)
            dirs << wxPATH_SEP << adirs[i];
        cfg->Write(key + _T("Dirs"), dirs);

        UpdateListPrj(id);
        UpdateListCat(id);
        return true;
    }
    else
        return false;
}

void ManagerFrame::DeleteProject(int id)
{
    wxString key;

    key.Printf(_T("Manager/project_%i"), id);
    wxConfig::Get()->DeleteGroup(key);
    UpdateListPrj();
    
    if (id == m_curPrj) 
    {
        m_listCat->ClearAll();
        m_curPrj = -1;
    }
}

void ManagerFrame::NotifyFileChanged(const wxString& catalog)
{
   // VS: We must do full update even if the file 'catalog' is not in
   //     m_catalogs. The reason is simple: the user might use SaveAs
   //     function and save new file in one of directories that
   //     this project matches...
   UpdateListCat();
}


BEGIN_EVENT_TABLE(ManagerFrame, wxFrame)
   EVT_MENU                 (XMLID("prj_new"),    ManagerFrame::OnNewProject)
   EVT_MENU                 (XMLID("prj_edit"),   ManagerFrame::OnEditProject)
   EVT_MENU                 (XMLID("prj_delete"), ManagerFrame::OnDeleteProject)
   EVT_MENU                 (XMLID("prj_update"), ManagerFrame::OnUpdateProject)
   EVT_LISTBOX              (XMLID("prj_list"),   ManagerFrame::OnSelectProject)
   EVT_LIST_ITEM_ACTIVATED  (XMLID("prj_files"),  ManagerFrame::OnOpenCatalog)
END_EVENT_TABLE()


void ManagerFrame::OnNewProject(wxCommandEvent& event)
{
    wxConfigBase *cfg = wxConfig::Get();
    int max = cfg->Read(_T("Manager/max_project_num"), (long)0) + 1;
    wxString key;
    for (int i = 0; i <= max; i++)
    {
        key.Printf(_T("Manager/project_%i/Name"), i);
        if (cfg->Read(key, wxEmptyString).IsEmpty())
        {
            m_listPrj->Append(_("<unnamed>"), (void*)i);
            m_curPrj = i;
            if (EditProject(i))
            {
                if (i == max)
                    cfg->Write(_T("Manager/max_project_num"), (long)max);
            }
            else
            {
                DeleteProject(i);
            }
            break;
        }
    }
}


void ManagerFrame::OnEditProject(wxCommandEvent& event)
{
	int sel = m_listPrj->GetSelection();
	if (sel == -1) return;
    EditProject((int)m_listPrj->GetClientData(sel));
}


void ManagerFrame::OnDeleteProject(wxCommandEvent& event)
{
	int sel = m_listPrj->GetSelection();
	if (sel == -1) return;
    if (wxMessageBox(_("Do you want to delete the project?"),
               _("Confirmation"), wxYES_NO | wxICON_QUESTION, this) == wxYES)
        DeleteProject((int)m_listPrj->GetClientData(sel));
}


void ManagerFrame::OnSelectProject(wxCommandEvent& event)
{
	int sel = m_listPrj->GetSelection();
	if (sel == -1) return;
    m_curPrj = (int)m_listPrj->GetClientData(sel);
    UpdateListCat(m_curPrj);
}


void ManagerFrame::OnUpdateProject(wxCommandEvent& event)
{
	int sel = m_listPrj->GetSelection();
	if (sel == -1) return;

    if (wxMessageBox(_("Do you really want to do mass update of\n"
                       "all catalogs in this project?"),
               _("Confirmation"), wxYES_NO | wxICON_QUESTION, this) == wxYES)
    {
        wxBusyCursor bcur;

        for (size_t i = 0; i < m_catalogs.GetCount(); i++)
        {
            wxString f = m_catalogs[i];
            poEditFrame *fr = poEditFrame::Find(f);
            if (fr)
            {
                fr->UpdateCatalog();
            }
            else
            {
                Catalog cat(f);
                cat.Update();
                cat.Save(f, false);
            }
         }
        
        UpdateListCat();
    }
}


void ManagerFrame::OnOpenCatalog(wxListEvent& event)
{
    poEditFrame::Create(m_catalogs[event.GetIndex()])->Raise();
}
