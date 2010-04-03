/*
 *  This file is part of Poedit (http://www.poedit.net)
 *
 *  Copyright (C) 2001-2008 Vaclav Slavik
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

#include <wx/imaglist.h>
#include <wx/config.h>
#include <wx/textctrl.h>
#include <wx/listctrl.h>
#include <wx/xrc/xmlres.h>
#include <wx/settings.h>
#include <wx/intl.h>
#include <wx/frame.h>
#include <wx/listbox.h>
#include "wxeditablelistbox.h"
#include <wx/tokenzr.h>
#include <wx/msgdlg.h>
#include <wx/button.h>
#include <wx/dir.h>
#include <wx/imaglist.h>
#include <wx/dirdlg.h>
#include <wx/log.h>
#include <wx/artprov.h>
#include <wx/iconbndl.h>

#include "catalog.h"
#include "edframe.h"
#include "manager.h"
#include "prefsdlg.h"
#include "progressinfo.h"


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

ManagerFrame::ManagerFrame() :
    wxFrame(NULL, -1, _("Poedit - Catalogs manager"),
            wxDefaultPosition, wxDefaultSize,
            wxDEFAULT_FRAME_STYLE | wxNO_FULL_REPAINT_ON_RESIZE)
{
#ifdef __UNIX__
    wxIconBundle appicons;
    appicons.AddIcon(wxArtProvider::GetIcon(_T("poedit"), wxART_FRAME_ICON, wxSize(16,16)));
    appicons.AddIcon(wxArtProvider::GetIcon(_T("poedit"), wxART_FRAME_ICON, wxSize(32,32)));
    appicons.AddIcon(wxArtProvider::GetIcon(_T("poedit"), wxART_FRAME_ICON, wxSize(48,48)));
    SetIcons(appicons);
#else
    SetIcon(wxICON(appicon));
#endif

    ms_instance = this;

    SetToolBar(wxXmlResource::Get()->LoadToolBar(this, _T("manager_toolbar")));
    SetMenuBar(wxXmlResource::Get()->LoadMenuBar(_T("manager_menu")));

    wxPanel *panel = wxXmlResource::Get()->LoadPanel(this, _T("manager_panel"));

    m_listPrj = XRCCTRL(*panel, "prj_list", wxListBox);
    m_listCat = XRCCTRL(*panel, "prj_files", wxListCtrl);

    wxImageList *list = new wxImageList(16, 16);
    list->Add(wxArtProvider::GetBitmap(_T("poedit-status-cat-no")));
    list->Add(wxArtProvider::GetBitmap(_T("poedit-status-cat-mid")));
    list->Add(wxArtProvider::GetBitmap(_T("poedit-status-cat-ok")));
    m_listCat->AssignImageList(list, wxIMAGE_LIST_SMALL);

    m_curPrj = -1;

    int last = wxConfig::Get()->Read(_T("manager_last_selected"), (long)0);

    // FIXME: do this in background (here and elsewhere)
    UpdateListPrj(last);
    if (m_listPrj->GetCount() > 0)
        UpdateListCat(last);

    wxConfigBase *cfg = wxConfig::Get();
    int width = cfg->Read(_T("manager_w"), 400);
    int height = cfg->Read(_T("manager_h"), 300);
    SetClientSize(width, height);

#ifndef __WXGTK__
    int posx = cfg->Read(_T("manager_x"), -1);
    int posy = cfg->Read(_T("manager_y"), -1);
    Move(posx, posy);
#endif

#if !defined(__WXGTK12__) || defined(__WXGTK20__)
    // GTK+ 1.2 doesn't support this
    if (wxConfig::Get()->Read(_T("manager_maximized"), long(0)))
        Maximize();
#endif
}



ManagerFrame::~ManagerFrame()
{
    wxSize sz = GetClientSize();
    wxPoint pos = GetPosition();
    wxConfigBase *cfg = wxConfig::Get();

    if (!IsIconized())
    {
        if (!IsMaximized())
        {
            cfg->Write(_T("manager_w"), (long)sz.x);
            cfg->Write(_T("manager_h"), (long)sz.y);
            cfg->Write(_T("manager_x"), (long)pos.x);
            cfg->Write(_T("manager_y"), (long)pos.y);
        }

        cfg->Write(_T("manager_maximized"), (long)IsMaximized());
    }

    int sel = m_listPrj->GetSelection();
    if (sel != -1)
    {
        cfg->Write(_T("manager_last_selected"),
                   (long)m_listPrj->GetClientData(sel));
    }

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
        if (!name.empty())
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
    int all = 0, fuzzy = 0, untranslated = 0, badtokens = 0;
    wxString lastmodified;
    time_t modtime;
    wxString key;
    wxString file2(file);
    file2.Replace(_T("/"), _T("_"));
    file2.Replace(_T("\\"), _T("_"));
    // FIXME: move cache to cache file and out of *config* file!
    key.Printf(_T("Manager/project_%i/FilesCache/%s/"), id, file2.c_str());

    modtime = cfg->Read(key + _T("timestamp"), (long)0);

    if (modtime == wxFileModificationTime(file))
    {
        all = cfg->Read(key + _T("all"), (long)0);
        fuzzy = cfg->Read(key + _T("fuzzy"), (long)0);
        badtokens = cfg->Read(key + _T("badtokens"), (long)0);
        untranslated = cfg->Read(key + _T("untranslated"), (long)0);
        lastmodified = cfg->Read(key + _T("lastmodified"), _T("?"));
    }
    else
    {
        // supress error messages, we don't mind if the catalog is corrupted
        // FIXME: *do* indicate error somehow
        wxLogNull nullLog;

        // FIXME: don't re-load the catalog if it's already loaded in the
        //        editor, reuse loaded instance
        Catalog cat(file);
        cat.GetStatistics(&all, &fuzzy, &badtokens, &untranslated);
        modtime = wxFileModificationTime(file);
        lastmodified = cat.Header().RevisionDate;
        cfg->Write(key + _T("timestamp"), (long)modtime);
        cfg->Write(key + _T("all"), (long)all);
        cfg->Write(key + _T("fuzzy"), (long)fuzzy);
        cfg->Write(key + _T("badtokens"), (long)badtokens);
        cfg->Write(key + _T("untranslated"), (long)untranslated);
        cfg->Write(key + _T("lastmodified"), lastmodified);
    }

    int icon;
    if (fuzzy+untranslated+badtokens == 0) icon = 2;
    else if ((double)all / (fuzzy+untranslated+badtokens) <= 3) icon = 0;
    else icon = 1;

    wxString tmp;
    // FIXME: don't put full filename there, remove common prefix (of all
    //        directories in project's settings)
    list->InsertItem(i, file, icon);
    tmp.Printf(_T("%i"), all);
    list->SetItem(i, 1, tmp);
    tmp.Printf(_T("%i"), untranslated);
    list->SetItem(i, 2, tmp);
    tmp.Printf(_T("%i"), fuzzy);
    list->SetItem(i, 3, tmp);
    tmp.Printf(_T("%i"), badtokens);
    list->SetItem(i, 4, tmp);
    list->SetItem(i, 5, lastmodified);
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
                           _T("*.po"), wxDIR_FILES | wxDIR_DIRS);

    m_catalogs.Sort();

    m_listCat->Freeze();

    m_listCat->ClearAll();
    m_listCat->InsertColumn(0, _("Catalog"));
    m_listCat->InsertColumn(1, _("Total"));
    m_listCat->InsertColumn(2, _("Untrans"));
    m_listCat->InsertColumn(3, _("Fuzzy"));
    m_listCat->InsertColumn(4, _("Bad Tokens"));
    m_listCat->InsertColumn(5, _("Last modified"));

    // FIXME: this is time-consuming, it should be done in parallel on
    //        multi-core/SMP systems
    for (size_t i = 0; i < m_catalogs.GetCount(); i++)
        AddCatalogToList(m_listCat, i, id, m_catalogs[i]);

    m_listCat->SetColumnWidth(0, wxLIST_AUTOSIZE);
    m_listCat->SetColumnWidth(1, wxLIST_AUTOSIZE_USEHEADER);
    m_listCat->SetColumnWidth(2, wxLIST_AUTOSIZE_USEHEADER);
    m_listCat->SetColumnWidth(3, wxLIST_AUTOSIZE_USEHEADER);
    m_listCat->SetColumnWidth(4, wxLIST_AUTOSIZE_USEHEADER);
    m_listCat->SetColumnWidth(5, wxLIST_AUTOSIZE);

    m_listCat->Thaw();
}


class ProjectDlg : public wxDialog
{
    protected:
        DECLARE_EVENT_TABLE()
        void OnBrowse(wxCommandEvent& event);
};

BEGIN_EVENT_TABLE(ProjectDlg, wxDialog)
   EVT_BUTTON(XRCID("adddir"), ProjectDlg::OnBrowse)
END_EVENT_TABLE()

void ProjectDlg::OnBrowse(wxCommandEvent&)
{
    wxDirDialog dlg(this, _("Select directory"));
    if (dlg.ShowModal() == wxID_OK)
    {
        wxArrayString a;
        wxEditableListBox *l = XRCCTRL(*this, "prj_dirs", wxEditableListBox);
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
    wxXmlResource::Get()->LoadDialog(&dlg, this, _T("manager_prj_dlg"));
    wxXmlResource::Get()->AttachUnknownControl(_T("prj_dirs"),
                new wxEditableListBox(this, -1, _("Directories:")));

    XRCCTRL(dlg, "prj_name", wxTextCtrl)->SetValue(cfg->Read(key + _T("Name")));

    wxString dirs = cfg->Read(key + _T("Dirs"));
    wxArrayString adirs;
    wxStringTokenizer tkn(dirs, wxPATH_SEP);
    while (tkn.HasMoreTokens()) adirs.Add(tkn.GetNextToken());
    XRCCTRL(*this, "prj_dirs", wxEditableListBox)->SetStrings(adirs);

    if (dlg.ShowModal() == wxID_OK)
    {
        cfg->Write(key + _T("Name"),
                   XRCCTRL(dlg, "prj_name", wxTextCtrl)->GetValue());
        XRCCTRL(*this, "prj_dirs", wxEditableListBox)->GetStrings(adirs);
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

void ManagerFrame::NotifyFileChanged(const wxString& /*catalog*/)
{
   // VS: We must do full update even if the file 'catalog' is not in
   //     m_catalogs. The reason is simple: the user might use SaveAs
   //     function and save new file in one of directories that
   //     this project matches...
   UpdateListCat();
}


BEGIN_EVENT_TABLE(ManagerFrame, wxFrame)
   EVT_MENU                 (XRCID("prj_new"),    ManagerFrame::OnNewProject)
   EVT_MENU                 (XRCID("prj_edit"),   ManagerFrame::OnEditProject)
   EVT_MENU                 (XRCID("prj_delete"), ManagerFrame::OnDeleteProject)
   EVT_MENU                 (XRCID("prj_update"), ManagerFrame::OnUpdateProject)
   EVT_LISTBOX              (XRCID("prj_list"),   ManagerFrame::OnSelectProject)
   EVT_LIST_ITEM_ACTIVATED  (XRCID("prj_files"),  ManagerFrame::OnOpenCatalog)
   EVT_MENU                 (wxID_EXIT,           ManagerFrame::OnQuit)
   EVT_MENU                 (wxID_PREFERENCES,    ManagerFrame::OnPreferences)
END_EVENT_TABLE()


void ManagerFrame::OnNewProject(wxCommandEvent&)
{
    wxConfigBase *cfg = wxConfig::Get();
    int max = cfg->Read(_T("Manager/max_project_num"), (long)0) + 1;
    wxString key;
    for (int i = 0; i <= max; i++)
    {
        key.Printf(_T("Manager/project_%i/Name"), i);
        if (cfg->Read(key, wxEmptyString).empty())
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


void ManagerFrame::OnEditProject(wxCommandEvent&)
{
    int sel = m_listPrj->GetSelection();
    if (sel == -1) return;
    EditProject((long)m_listPrj->GetClientData(sel));
}


void ManagerFrame::OnDeleteProject(wxCommandEvent&)
{
    int sel = m_listPrj->GetSelection();
    if (sel == -1) return;
    if (wxMessageBox(_("Do you want to delete the project?"),
               _("Confirmation"), wxYES_NO | wxICON_QUESTION, this) == wxYES)
        DeleteProject((long)m_listPrj->GetClientData(sel));
}


void ManagerFrame::OnSelectProject(wxCommandEvent&)
{
    int sel = m_listPrj->GetSelection();
    if (sel == -1) return;
    m_curPrj = (long)m_listPrj->GetClientData(sel);
    UpdateListCat(m_curPrj);
}


void ManagerFrame::OnUpdateProject(wxCommandEvent&)
{
    int sel = m_listPrj->GetSelection();
    if (sel == -1) return;

    if (wxMessageBox(_("Do you really want to do mass update of\nall catalogs in this project?"),
               _("Confirmation"), wxYES_NO | wxICON_QUESTION, this) == wxYES)
    {
        wxBusyCursor bcur;

        for (size_t i = 0; i < m_catalogs.GetCount(); i++)
        {
            // FIXME: there should be only one progress bar for _all_
            //        catalogs, it shouldn't restart on next catalog
            ProgressInfo pinfo(this);

            wxString f = m_catalogs[i];
            PoeditFrame *fr = PoeditFrame::Find(f);
            if (fr)
            {
                fr->UpdateCatalog();
            }
            else
            {
                Catalog cat(f);
                cat.Update(&pinfo);
                cat.Save(f, false);
            }
         }

        UpdateListCat();
    }
}


void ManagerFrame::OnOpenCatalog(wxListEvent& event)
{
    PoeditFrame::Create(m_catalogs[event.GetIndex()])->Raise();
}


void ManagerFrame::OnPreferences(wxCommandEvent&)
{
    PreferencesDialog dlg(this);

    dlg.TransferTo(wxConfig::Get());
    if (dlg.ShowModal() == wxID_OK)
    {
        dlg.TransferFrom(wxConfig::Get());
    }
}

void ManagerFrame::OnQuit(wxCommandEvent&)
{
    Close(true);
}
