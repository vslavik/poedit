/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 2001-2017 Vaclav Slavik
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

#include <wx/imaglist.h>
#include <wx/config.h>
#include <wx/textctrl.h>
#include <wx/listctrl.h>
#include <wx/xrc/xmlres.h>
#include <wx/settings.h>
#include <wx/stdpaths.h>
#include <wx/intl.h>
#include <wx/frame.h>
#include <wx/listbox.h>
#include <wx/editlbox.h>
#include <wx/tokenzr.h>
#include <wx/msgdlg.h>
#include <wx/button.h>
#include <wx/dir.h>
#include <wx/imaglist.h>
#include <wx/dirdlg.h>
#include <wx/log.h>
#include <wx/artprov.h>
#include <wx/iconbndl.h>
#include <wx/windowptr.h>
#include <wx/splitter.h>

#ifdef __WXMSW__
#include <wx/msw/uxtheme.h>
#endif

#include "catalog.h"
#include "cat_update.h"
#include "edapp.h"
#include "edframe.h"
#include "hidpi.h"
#include "manager.h"
#include "progressinfo.h"
#include "utility.h"


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
            wxDEFAULT_FRAME_STYLE | wxNO_FULL_REPAINT_ON_RESIZE,
            "manager")
{
#if defined(__WXGTK__)
    wxIconBundle appicons;
    appicons.AddIcon(wxArtProvider::GetIcon("poedit", wxART_FRAME_ICON, wxSize(16,16)));
    appicons.AddIcon(wxArtProvider::GetIcon("poedit", wxART_FRAME_ICON, wxSize(32,32)));
    appicons.AddIcon(wxArtProvider::GetIcon("poedit", wxART_FRAME_ICON, wxSize(48,48)));
    SetIcons(appicons);
#elif defined(__WXMSW__)
    SetIcons(wxIconBundle(wxStandardPaths::Get().GetResourcesDir() + "\\Resources\\Poedit.ico"));
#endif

    ms_instance = this;

    auto tb = wxXmlResource::Get()->LoadToolBar(this, "manager_toolbar");
    (void)tb;
#ifdef __WXMSW__
    // De-uglify the toolbar a bit on Windows 10:
    if (IsWindows10OrGreater())
    {
        const wxUxThemeEngine* theme = wxUxThemeEngine::GetIfActive();
        if (theme)
        {
            wxUxThemeHandle hTheme(tb, L"ExplorerMenu::Toolbar");
            tb->SetBackgroundColour(wxRGBToColour(theme->GetThemeSysColor(hTheme, COLOR_WINDOW)));
        }
    }
#endif

    wxPanel *panel = wxXmlResource::Get()->LoadPanel(this, "manager_panel");

    m_listPrj = XRCCTRL(*panel, "prj_list", wxListBox);
    m_listCat = XRCCTRL(*panel, "prj_files", wxListCtrl);
    m_splitter = XRCCTRL(*panel, "manager_splitter", wxSplitterWindow);

    wxImageList *list = new wxImageList(PX(16), PX(16));
    list->Add(wxArtProvider::GetBitmap("poedit-status-cat-no"));
    list->Add(wxArtProvider::GetBitmap("poedit-status-cat-mid"));
    list->Add(wxArtProvider::GetBitmap("poedit-status-cat-ok"));
    m_listCat->AssignImageList(list, wxIMAGE_LIST_SMALL);

    m_curPrj = -1;

    int last = (int)wxConfig::Get()->Read("manager_last_selected", (long)0);

    // FIXME: do this in background (here and elsewhere)
    UpdateListPrj(last);
    if (m_listPrj->GetCount() > 0)
        UpdateListCat(last);

    RestoreWindowState(this, wxSize(PX(400), PX(300)));

    m_splitter->SetSashPosition((int)wxConfig::Get()->Read("manager_splitter", PX(200)));
}



ManagerFrame::~ManagerFrame()
{
    SaveWindowState(this);

    wxConfigBase *cfg = wxConfig::Get();

    int sel = m_listPrj->GetSelection();
    if (sel != -1)
    {
        cfg->Write("manager_last_selected",
                   (long)m_listPrj->GetClientData(sel));
    }

    wxConfig::Get()->Write("manager_splitter", (long)m_splitter->GetSashPosition());

    ms_instance = NULL;
}


void ManagerFrame::UpdateListPrj(int select)
{
    wxConfigBase *cfg = wxConfig::Get();
    long max = cfg->Read("Manager/max_project_num", (long)0) + 1;
    wxString key, name;

    m_listPrj->Clear();
    int item = 0;
    for (int i = 0; i <= max; i++)
    {
        key.Printf("Manager/project_%i/Name", i);
        name = cfg->Read(key, wxEmptyString);
        if (!name.empty())
        {
            m_listPrj->Append(name, (void*)(wxIntPtr)i);
            if (i == select)
            {
                m_listPrj->SetSelection(item);
                m_curPrj = select;
                UpdateListCat(m_curPrj);
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
    file2.Replace("/", "_");
    file2.Replace("\\", "_");
    // FIXME: move cache to cache file and out of *config* file!
    key.Printf("Manager/project_%i/FilesCache/%s/", id, file2.c_str());

    modtime = cfg->Read(key + "timestamp", (long)0);

    if (modtime == wxFileModificationTime(file))
    {
        all = (int)cfg->Read(key + "all", (long)0);
        fuzzy = (int)cfg->Read(key + "fuzzy", (long)0);
        badtokens = (int)cfg->Read(key + "badtokens", (long)0);
        untranslated = (int)cfg->Read(key + "untranslated", (long)0);
        lastmodified = cfg->Read(key + "lastmodified", "?");
    }
    else
    {
        // suppress error messages, we don't mind if the catalog is corrupted
        // FIXME: *do* indicate error somehow
        wxLogNull nullLog;

        // FIXME: don't re-load the catalog if it's already loaded in the
        //        editor, reuse loaded instance
        Catalog cat(file);
        cat.GetStatistics(&all, &fuzzy, &badtokens, &untranslated, NULL);
        modtime = wxFileModificationTime(file);
        lastmodified = cat.Header().RevisionDate;
        cfg->Write(key + "timestamp", (long)modtime);
        cfg->Write(key + "all", (long)all);
        cfg->Write(key + "fuzzy", (long)fuzzy);
        cfg->Write(key + "badtokens", (long)badtokens);
        cfg->Write(key + "untranslated", (long)untranslated);
        cfg->Write(key + "lastmodified", lastmodified);
    }

    int icon;
    if (fuzzy+untranslated+badtokens == 0) icon = 2;
    else if ((double)all / (fuzzy+untranslated+badtokens) <= 3) icon = 0;
    else icon = 1;

    wxString tmp;
    // FIXME: don't put full filename there, remove common prefix (of all
    //        directories in project's settings)
    list->InsertItem(i, file, icon);
    tmp.Printf("%i", all);
    list->SetItem(i, 1, tmp);
    tmp.Printf("%i", untranslated);
    list->SetItem(i, 2, tmp);
    tmp.Printf("%i", fuzzy);
    list->SetItem(i, 3, tmp);
    tmp.Printf("%i", badtokens);
    list->SetItem(i, 4, tmp);
    list->SetItem(i, 5, lastmodified);
}

void ManagerFrame::UpdateListCat(int id)
{
    wxBusyCursor bcur;

    if (id == -1) id = m_curPrj;

    wxConfigBase *cfg = wxConfig::Get();
    wxString key;
    key.Printf("Manager/project_%i/", id);

    wxString dirs = cfg->Read(key + "Dirs", wxEmptyString);
    wxStringTokenizer tkn(dirs, wxPATH_SEP);

    m_catalogs.Clear();
    while (tkn.HasMoreTokens())
        wxDir::GetAllFiles(tkn.GetNextToken(), &m_catalogs,
                           "*.po", wxDIR_FILES | wxDIR_DIRS);

    m_catalogs.Sort();

    m_listCat->Freeze();

    m_listCat->ClearAll();
    m_listCat->InsertColumn(0, _("Catalog"));
    m_listCat->InsertColumn(1, _("Total"));
    m_listCat->InsertColumn(2, _("Untrans"));
    m_listCat->InsertColumn(3, _("Needs Work"));
    m_listCat->InsertColumn(4, _("Errors"));
    m_listCat->InsertColumn(5, _("Last modified"));

    // FIXME: this is time-consuming, it should be done in parallel on
    //        multi-core/SMP systems
    for (int i = 0; i < (int)m_catalogs.GetCount(); i++)
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

template<typename TFunctor>
void ManagerFrame::EditProject(int id, TFunctor completionHandler)
{
    wxConfigBase *cfg = wxConfig::Get();
    wxString key;
    key.Printf("Manager/project_%i/", id);

    wxWindowPtr<ProjectDlg> dlg(new ProjectDlg);
    wxXmlResource::Get()->LoadDialog(dlg.get(), this, "manager_prj_dlg");
    wxEditableListBox *prj_dirs = new wxEditableListBox(dlg.get(), XRCID("prj_dirs"), _("Directories:"));
    wxXmlResource::Get()->AttachUnknownControl("prj_dirs", prj_dirs);

    XRCCTRL(*dlg, "prj_name", wxTextCtrl)->SetValue(cfg->Read(key + "Name"));

    {
        wxString dirs = cfg->Read(key + "Dirs");
        wxArrayString adirs;
        wxStringTokenizer tkn(dirs, wxPATH_SEP);
        while (tkn.HasMoreTokens())
            adirs.Add(tkn.GetNextToken());
        prj_dirs->SetStrings(adirs);
    }

    dlg->ShowWindowModalThenDo([=](int retcode){
        if (retcode == wxID_OK)
        {
            wxString dirs;
            wxArrayString adirs;

            cfg->Write(key + "Name",
                       XRCCTRL(*dlg, "prj_name", wxTextCtrl)->GetValue());
            prj_dirs->GetStrings(adirs);
            if (adirs.GetCount() > 0)
                dirs = adirs[0];
            for (size_t i = 1; i < adirs.GetCount(); i++)
                dirs << wxPATH_SEP << adirs[i];
            cfg->Write(key + "Dirs", dirs);

            UpdateListPrj(id);
            UpdateListCat(id);
        }
        completionHandler(retcode == wxID_OK);
    });
}

void ManagerFrame::DeleteProject(int id)
{
    wxString key;

    key.Printf("Manager/project_%i", id);
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
   EVT_MENU                 (wxID_CLOSE,          ManagerFrame::OnCloseCmd)
END_EVENT_TABLE()


void ManagerFrame::OnNewProject(wxCommandEvent&)
{
    wxConfigBase *cfg = wxConfig::Get();
    long max = cfg->Read("Manager/max_project_num", (long)0) + 1;
    wxString key;
    for (int i = 0; i <= max; i++)
    {
        key.Printf("Manager/project_%i/Name", i);
        if (cfg->Read(key, wxEmptyString).empty())
        {
            m_listPrj->Append(_("<unnamed>"), (void*)(wxIntPtr)i);
            m_curPrj = i;
            EditProject(i, [=](bool added){
                if (added)
                {
                    if (i == max)
                        cfg->Write("Manager/max_project_num", (long)max);
                }
                else
                {
                    DeleteProject(i);
                }
            });
            return;
        }
    }
}


void ManagerFrame::OnEditProject(wxCommandEvent&)
{
    int sel = m_listPrj->GetSelection();
    if (sel == -1) return;
    EditProject((int)(wxIntPtr)m_listPrj->GetClientData(sel), [](bool){});
}


void ManagerFrame::OnDeleteProject(wxCommandEvent&)
{
    int sel = m_listPrj->GetSelection();
    if (sel == -1) return;
    if (wxMessageBox(_("Do you want to delete the project?"),
               _("Confirmation"), wxYES_NO | wxICON_QUESTION, this) == wxYES)
        DeleteProject((int)(wxIntPtr)m_listPrj->GetClientData(sel));
}


void ManagerFrame::OnSelectProject(wxCommandEvent&)
{
    int sel = m_listPrj->GetSelection();
    if (sel == -1) return;
    m_curPrj = (int)(wxIntPtr)m_listPrj->GetClientData(sel);
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
            //ProgressInfo pinfo(this, _("Updating catalog"));

            wxString f = m_catalogs[i];
            PoeditFrame *fr = PoeditFrame::Find(f);
            if (fr)
            {
                fr->UpdateCatalog();
            }
            else
            {
                auto cat = std::make_shared<Catalog>(f);
                UpdateResultReason reason;
                if (PerformUpdateFromSources(this, cat, reason, Update_DontShowSummary))
                {
                    int validation_errors = 0;
                    Catalog::CompilationStatus mo_status;
                    cat->Save(f, false, validation_errors, mo_status);
                }
            }
         }

        UpdateListCat();
    }
}


void ManagerFrame::OnOpenCatalog(wxListEvent& event)
{
    PoeditFrame *f = PoeditFrame::Create(m_catalogs[event.GetIndex()]);
    if (f)
        f->Raise();
}


void ManagerFrame::OnCloseCmd(wxCommandEvent&)
{
    Close();
}
