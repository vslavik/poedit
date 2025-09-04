/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 2001-2025 Vaclav Slavik
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

#include "manager.h"

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
#include <wx/bmpbuttn.h>
#include <wx/dir.h>
#include <wx/imaglist.h>
#include <wx/dirdlg.h>
#include <wx/log.h>
#include <wx/artprov.h>
#include <wx/iconbndl.h>
#include <wx/windowptr.h>
#include <wx/sizer.h>

#include "catalog.h"
#include "cat_update.h"
#include "edapp.h"
#include "edframe.h"
#include "hidpi.h"
#include "menus.h"
#include "progress_ui.h"
#include "utility.h"


namespace
{

class PseudoToolbarButton : public wxButton
{
public:
    PseudoToolbarButton(wxWindow *parent, wxString bitmap, const wxString& label)
    {
#if defined(__WXOSX__)
        (void)label;
        if (bitmap == "poedit-update")
            bitmap = "UpdateTemplate";
        else if (bitmap == "stats")
            bitmap = "StatsTemplate";
        wxButton::Create(parent, wxID_ANY, "", wxDefaultPosition, wxSize(35, 28), wxBU_EXACTFIT);
        auto native = (NSButton*)GetHandle();
        native.image = [NSImage imageNamed:str::to_NS(bitmap)];
        native.imagePosition = NSImageOnly;
        native.bezelStyle = NSBezelStyleTexturedRounded;
        native.buttonType = NSButtonTypeMomentaryPushIn;
        native.showsBorderOnlyWhileMouseInside = YES;
#else
    #ifdef __WXGTK3__
        bitmap += "-symbolic";
    #endif
        wxButton::Create(parent, wxID_ANY, label, wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
        SetBitmap(wxArtProvider::GetBitmap(bitmap, wxART_TOOLBAR));
#endif
    }
};

} // anonymous namespace


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
    ManagerFrameBase(NULL, -1, _("Poedit - Catalogs manager"),
                     wxDefaultPosition, wxDefaultSize,
                     wxDEFAULT_FRAME_STYLE | wxNO_FULL_REPAINT_ON_RESIZE,
                     "manager")
{
#ifdef __WXMSW__
    SetIcons(wxIconBundle(wxStandardPaths::Get().GetResourcesDir() + "\\Resources\\Poedit.ico"));
#endif
#ifndef __WXOSX__
    SetMenuBar(wxGetApp().CreateMenu(Menu::WelcomeWindow));
#endif

    ms_instance = this;

    auto panel = new wxPanel(this, wxID_ANY);
    auto supertopsizer = new wxBoxSizer(wxHORIZONTAL);
    supertopsizer->Add(panel, wxSizerFlags(1).Expand());
    SetSizer(supertopsizer);

    auto topsizer = new wxBoxSizer(wxHORIZONTAL);
    panel->SetSizer(topsizer);

    auto sidebarSizer = new wxBoxSizer(wxVERTICAL);
    topsizer->Add(sidebarSizer, wxSizerFlags().Expand().Border(wxALL, PX(10)));

    m_listPrj = new wxListBox(panel, wxID_ANY, wxDefaultPosition, wxSize(PX(200), -1), 0, nullptr, MSW_OR_OTHER(wxBORDER_SIMPLE, wxBORDER_SUNKEN));
#ifdef __WXOSX__
    ((NSTableView*)[((NSScrollView*)m_listPrj->GetHandle()) documentView]).style = NSTableViewStyleFullWidth;
#endif
    sidebarSizer->Add(m_listPrj, wxSizerFlags(1).Expand());

#if defined(__WXOSX__)
    auto btn_new = new wxBitmapButton(panel, wxID_ANY, wxArtProvider::GetBitmap("NSAddTemplate"), wxDefaultPosition, wxSize(18, 18), wxBORDER_SIMPLE);
    auto btn_delete = new wxBitmapButton(panel, wxID_ANY, wxArtProvider::GetBitmap("NSRemoveTemplate"), wxDefaultPosition, wxSize(18,18), wxBORDER_SIMPLE);
    int editButtonStyle = wxBU_EXACTFIT | wxBORDER_SIMPLE;
#elif defined(__WXMSW__)
    auto btn_new = new wxBitmapButton(panel, wxID_ANY, wxArtProvider::GetBitmap("list-add"), wxDefaultPosition, wxSize(PX(19),PX(19)));
    auto btn_delete = new wxBitmapButton(panel, wxID_ANY, wxArtProvider::GetBitmap("list-remove"), wxDefaultPosition, wxSize(PX(19),PX(19)));
    int editButtonStyle = wxBU_EXACTFIT;
#elif defined(__WXGTK__)
    auto btn_new = new wxBitmapButton(panel, wxID_ANY, wxArtProvider::GetBitmap("list-add@symbolic"), wxDefaultPosition, wxDefaultSize, wxNO_BORDER);
    auto btn_delete = new wxBitmapButton(panel, wxID_ANY, wxArtProvider::GetBitmap("list-remove@symbolic"), wxDefaultPosition, wxDefaultSize, wxNO_BORDER);
    int editButtonStyle = wxBU_EXACTFIT | wxBORDER_NONE;
#endif
    auto btn_edit = new wxButton(panel, wxID_ANY, _(L"Edit…"), wxDefaultPosition, wxSize(-1, MSW_OR_OTHER(PX(19), -1)), editButtonStyle);
#ifndef __WXGTK__
    btn_edit->SetWindowVariant(wxWINDOW_VARIANT_SMALL);
#endif

    btn_new->SetToolTip(_("Create new translations project"));
    btn_delete->SetToolTip(_("Delete the project"));
    btn_edit->SetToolTip(_("Edit the project"));

    auto buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    buttonSizer->Add(btn_new);
#ifdef __WXOSX__
    buttonSizer->AddSpacer(PX(1));
#endif
    buttonSizer->Add(btn_delete);
#ifdef __WXOSX__
    buttonSizer->AddSpacer(PX(1));
#endif
    buttonSizer->Add(btn_edit);

    sidebarSizer->AddSpacer(PX(1));
    sidebarSizer->Add(buttonSizer, wxSizerFlags().Expand().BORDER_MACOS(wxLEFT, PX(1)));

    m_details = new wxPanel(panel, wxID_ANY, wxDefaultPosition, wxDefaultSize, MSW_OR_OTHER(wxBORDER_SIMPLE, wxBORDER_SUNKEN));
    topsizer->Add(m_details, wxSizerFlags(1).ReserveSpaceEvenIfHidden().Expand().Border(wxTOP|wxBOTTOM|wxRIGHT, PX(10)));

    auto detailsSizer = new wxBoxSizer(wxVERTICAL);
    m_details->SetSizer(detailsSizer);

    auto topbar = new wxBoxSizer(wxHORIZONTAL);
    detailsSizer->AddSpacer(PX(5));
    detailsSizer->Add(topbar, wxSizerFlags().Expand().Border(wxALL, PX(5)));
    detailsSizer->AddSpacer(PX(5));
    m_projectName = new wxStaticText(m_details, wxID_ANY, "");
    m_projectName->SetFont(m_projectName->GetFont().Larger().Bold());
    topbar->Add(m_projectName, wxSizerFlags().Center().Border(wxRIGHT, PX(10)));
    topbar->AddStretchSpacer();
    auto btn_update = new PseudoToolbarButton(m_details, "poedit-update", _("Update all"));
    btn_update->SetToolTip(_("Update all catalogs in the project"));
    topbar->Add(btn_update, wxSizerFlags().Border(wxLEFT, PX(5)));

    m_listCat = new wxListCtrl(m_details, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE | wxLC_REPORT | wxLC_SINGLE_SEL);
#ifdef __WXOSX__
    m_listCat->SetWindowVariant(wxWINDOW_VARIANT_SMALL);
#endif
    detailsSizer->Add(m_listCat, wxSizerFlags(1).Expand());

    auto bmp_no = wxArtProvider::GetBitmap("poedit-status-cat-no");
    wxImageList *list = new wxImageList(bmp_no.GetScaledWidth(), bmp_no.GetScaledHeight());
    list->Add(bmp_no);
    list->Add(wxArtProvider::GetBitmap("poedit-status-cat-mid"));
    list->Add(wxArtProvider::GetBitmap("poedit-status-cat-ok"));
    m_listCat->AssignImageList(list, wxIMAGE_LIST_SMALL);

    ColorScheme::SetupWindowColors(this, [=]
    {
        wxColour col;
        if (ColorScheme::GetWindowMode(this) == ColorScheme::Light)
            col = *wxWHITE;
        else
            col = GetDefaultAttributes().colBg;
        m_details->SetBackgroundColour(col);
#ifdef __WXMSW__
        SetBackgroundColour(col);
        btn_update->SetBackgroundColour(col);
#endif
    });

    m_details->Hide();
    m_curPrj = -1;

    int last = (int)wxConfig::Get()->Read("manager_last_selected", (long)0);

    // FIXME: do this in background (here and elsewhere)
    UpdateListPrj(last);
    if (m_listPrj->GetCount() > 0)
        UpdateListCat(last);

    RestoreWindowState(this, wxSize(PX(900), PX(600)));

    m_listPrj->Bind(wxEVT_LISTBOX, &ManagerFrame::OnSelectProject, this);
    m_listCat->Bind(wxEVT_LIST_ITEM_ACTIVATED, &ManagerFrame::OnOpenCatalog, this);
    btn_new->Bind(wxEVT_BUTTON, &ManagerFrame::OnNewProject, this);
    btn_delete->Bind(wxEVT_BUTTON, &ManagerFrame::OnDeleteProject, this);
    btn_delete->Bind(wxEVT_UPDATE_UI, [=](wxUpdateUIEvent& e) { e.Enable(m_listPrj->GetSelection() != wxNOT_FOUND); });
    btn_edit->Bind(wxEVT_BUTTON, &ManagerFrame::OnEditProject, this);
    btn_update->Bind(wxEVT_BUTTON, &ManagerFrame::OnUpdateProject, this);
}



ManagerFrame::~ManagerFrame()
{
    SaveWindowState(this);

    wxConfigBase *cfg = wxConfig::Get();

    int sel = m_listPrj->GetSelection();
    if (sel != -1)
    {
        cfg->Write("manager_last_selected",
                   (long)(wxIntPtr)m_listPrj->GetClientData(sel));
    }

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
        // suppress error messages, we don't care about specifics of the error
        // FIXME: *do* indicate error somehow
        wxLogNull nullLog;

        // FIXME: don't re-load the catalog if it's already loaded in the
        //        editor, reuse loaded instance
        try
        {
            auto cat = Catalog::Create(file);
            if (cat)
            {
                cat->GetStatistics(&all, &fuzzy, &badtokens, &untranslated, NULL);
                modtime = wxFileModificationTime(file);
                lastmodified = cat->Header().RevisionDate;
                cfg->Write(key + "timestamp", (long)modtime);
                cfg->Write(key + "all", (long)all);
                cfg->Write(key + "fuzzy", (long)fuzzy);
                cfg->Write(key + "badtokens", (long)badtokens);
                cfg->Write(key + "untranslated", (long)untranslated);
                cfg->Write(key + "lastmodified", lastmodified);
            }
        }
        catch (...)
        {
            // FIXME: Nicer way of showing errors, this is hacky
            lastmodified = L"⚠️ " + DescribeCurrentException();
            badtokens = 1;
        }
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

    m_details->Show();
    m_projectName->SetLabel(m_listPrj->GetStringSelection());
    m_details->Layout();

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
    m_listCat->InsertColumn(0, _("File"));
    m_listCat->InsertColumn(1, _("Total"));
    m_listCat->InsertColumn(2, _("Untrans"));
    m_listCat->InsertColumn(3, wxGETTEXT_IN_CONTEXT("column/row header", "Needs Work"));
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

    auto prjname = m_listPrj->GetStringSelection();
    wxWindowPtr<wxMessageDialog> dlg(new wxMessageDialog
                     (
                         this,
                         wxString::Format(_(L"Do you want to delete project “%s”?"), prjname),
                         MSW_OR_OTHER(_("Delete project"), ""),
                         wxYES_NO | wxNO_DEFAULT | wxICON_WARNING
                     ));
    dlg->SetExtendedMessage(_("Deleting the project will not delete any translation files."));
    dlg->ShowWindowModalThenDo([this,dlg,sel](int retval)
    {
        if (retval == wxID_YES)
            DeleteProject((int)(wxIntPtr)m_listPrj->GetClientData(sel));
    });
}


void ManagerFrame::OnSelectProject(wxCommandEvent&)
{
    int sel = m_listPrj->GetSelection();
    if (sel == -1)
    {
        m_details->Hide();
        return;
    }
    m_curPrj = (int)(wxIntPtr)m_listPrj->GetClientData(sel);

    UpdateListCat(m_curPrj);
}


void ManagerFrame::OnUpdateProject(wxCommandEvent&)
{
    int sel = m_listPrj->GetSelection();
    if (sel == -1) return;

    wxWindowPtr<wxMessageDialog> dlg(new wxMessageDialog(this, _("Update all catalogs in this project?"), MSW_OR_OTHER(_("Confirmation"), ""), wxYES_NO | wxICON_QUESTION));
    dlg->SetExtendedMessage(_("Performs update from source code on all files in the project."));
    dlg->ShowWindowModalThenDo([this,dlg,sel](int retval)
    {
         if (retval != wxID_YES)
            return;

        auto cancellation = std::make_shared<dispatch::cancellation_token>();
        wxWindowPtr<ProgressWindow> progress(new ProgressWindow(this, _("Updating project catalogs"), cancellation));
        progress->RunTaskThenDo([=]()
        {
            Progress progress((int)m_catalogs.GetCount());

            for (size_t i = 0; i < m_catalogs.GetCount(); i++)
            {
                if (cancellation->is_cancelled())
                    return;

                wxString f = m_catalogs[i];

                Progress subtask(1, progress, 1);

                auto cat = POCatalog::Create(f);
                if (auto merged = PerformUpdateFromSourcesSimple(cat))
                {
                    Catalog::ValidationResults validation_results;
                    Catalog::CompilationStatus mo_status;
                    merged.updated_catalog->Save(f, false, validation_results, mo_status);
                }
             }
        },
        [=]()
        {
            UpdateListCat();
        });
    });
}


void ManagerFrame::OnOpenCatalog(wxListEvent& event)
{
    PoeditFrame *f = PoeditFrame::Create(m_catalogs[event.GetIndex()]);
    if (f)
        f->Raise();
}
