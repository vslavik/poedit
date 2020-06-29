/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 2015-2020 Vaclav Slavik
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


#include "crowdin_gui.h"

#include "crowdin_client.h"

#include "catalog.h"
#include "cloud_sync.h"
#include "colorscheme.h"
#include "concurrency.h"
#include "customcontrols.h"
#include "errors.h"
#include "hidpi.h"
#include "http_client.h"
#include "languagectrl.h"
#include "str_helpers.h"
#include "utility.h"

#include <wx/app.h>
#include <wx/artprov.h>
#include <wx/button.h>
#include <wx/choice.h>
#include <wx/config.h>
#include <wx/dataview.h>
#include <wx/dialog.h>
#include <wx/msgdlg.h>
#include <wx/renderer.h>
#include <wx/sizer.h>
#include <wx/statbmp.h>
#include <wx/stdpaths.h>
#include <wx/weakref.h>
#include <wx/windowptr.h>

#ifdef __WXMSW__
    #include <wx/generic/private/markuptext.h>
#endif

#if !wxCHECK_VERSION(3,1,0)
    #define CenterVertical() Center()
#endif

#include <boost/algorithm/string.hpp>

CrowdinLoginPanel::CrowdinLoginPanel(wxWindow *parent, int flags)
    : wxPanel(parent, wxID_ANY),
      m_state(State::Uninitialized)
{
    wxBoxSizer *sizer = new wxBoxSizer(wxVERTICAL);
    sizer->SetMinSize(PX(400), -1);
    SetSizer(sizer);

    sizer->AddSpacer(PX(10));
    auto logo = new StaticBitmap(this, "CrowdinLogoTemplate");
    logo->SetCursor(wxCURSOR_HAND);
    logo->Bind(wxEVT_LEFT_UP, [](wxMouseEvent&){ wxLaunchDefaultBrowser(CrowdinClient::AttributeLink("/")); });
    sizer->Add(logo, wxSizerFlags().PXDoubleBorder(wxBOTTOM));
    auto explain = new ExplanationLabel(this, _("Crowdin is an online localization management platform and collaborative translation tool. Poedit can seamlessly sync PO files managed at Crowdin."));
    sizer->Add(explain, wxSizerFlags().Expand());

    m_loginInfo = new wxBoxSizer(wxHORIZONTAL);
    auto loginInfoContainer = new wxBoxSizer(wxVERTICAL);
    loginInfoContainer->SetMinSize(-1, PX(50));
    loginInfoContainer->AddStretchSpacer();
    loginInfoContainer->Add(m_loginInfo, wxSizerFlags().Center());
    loginInfoContainer->AddStretchSpacer();

    sizer->Add(loginInfoContainer, wxSizerFlags().Expand().ReserveSpaceEvenIfHidden().Border(wxTOP|wxBOTTOM, PX(10)));

    m_signIn = new wxButton(this, wxID_ANY, MSW_OR_OTHER(_("Sign in"), _("Sign In")));
    m_signIn->Bind(wxEVT_BUTTON, &CrowdinLoginPanel::OnSignIn, this);
    m_signOut= new wxButton(this, wxID_ANY, MSW_OR_OTHER(_("Sign out"), _("Sign Out")));
    m_signOut->Bind(wxEVT_BUTTON, &CrowdinLoginPanel::OnSignOut, this);

    auto learnMore = new LearnAboutCrowdinLink(this);

    auto buttons = new wxBoxSizer(wxHORIZONTAL);
    sizer->Add(buttons, wxSizerFlags().Expand().Border(wxBOTTOM, 1));
    buttons->Add(learnMore, wxSizerFlags().Center().Border(wxLEFT, PX(LearnMoreLink::EXTRA_INDENT)));
    buttons->AddStretchSpacer();
    buttons->Add(m_signIn, wxSizerFlags());
    buttons->Add(m_signOut, wxSizerFlags());

    if (flags & DialogButtons)
    {
        auto cancel = new wxButton(this, wxID_CANCEL);
#ifdef __WXMSW__
        buttons->Add(cancel, wxSizerFlags().Border(wxLEFT, PX(3)));
#else
        buttons->Insert(2, cancel, wxSizerFlags().Border(wxRIGHT, PX(6)));
#endif
        m_signIn->SetDefault();
        m_signIn->SetFocus();
    }

    ChangeState(State::Uninitialized);
}

void CrowdinLoginPanel::EnsureInitialized()
{
    if (m_state != State::Uninitialized)
        return;

    if (CrowdinClient::Get().IsSignedIn())
        UpdateUserInfo();
    else
        ChangeState(State::SignedOut);
}

void CrowdinLoginPanel::ChangeState(State state)
{
    m_state = state;

    bool canSignIn = (state == State::SignedOut || state == State::Authenticating);
    auto sizer = m_signIn->GetContainingSizer();
    m_signIn->GetContainingSizer()->Show(m_signIn, canSignIn);
    if (m_signOut)
        m_signOut->GetContainingSizer()->Show(m_signOut, !canSignIn);
    sizer->Layout();

    CreateLoginInfoControls(state);
}

void CrowdinLoginPanel::CreateLoginInfoControls(State state)
{
    auto sizer = m_loginInfo;
    sizer->Clear(/*delete_windows=*/true);

    switch (state)
    {
        case State::Authenticating:
        case State::UpdatingInfo:
        {
            auto text = (state == State::Authenticating)
                      ? _(L"Waiting for authentication…")
                      : _(L"Updating user information…");
            auto waitingLabel = new ActivityIndicator(this);
            sizer->Add(waitingLabel, wxSizerFlags().Center());
            waitingLabel->Start(text);
            break;
        }

        case State::Uninitialized:
        case State::SignedOut:
        {
            // nothing to show in the UI except for "sign in" button
            break;
        };

        case State::SignedIn:
        {
            auto profile = new AvatarIcon(this, wxSize(PX(48), PX(48)));
            auto name = new wxStaticText(this, wxID_ANY, m_userName);
            auto username = new SecondaryLabel(this, m_userLogin);

            sizer->Add(profile, wxSizerFlags().Center());
            sizer->AddSpacer(PX(6));
            auto box = new wxBoxSizer(wxVERTICAL);
            box->Add(name, wxSizerFlags().Left());
            box->Add(username, wxSizerFlags().Left());
            sizer->Add(box, wxSizerFlags().Center());
            sizer->AddSpacer(PX(6));

            profile->SetUserName(m_userName);
            if (!m_userAvatar.empty())
            {
                http_client::download_from_anywhere(m_userAvatar)
                .then_on_window(profile, [=](downloaded_file f)
                {
                    profile->LoadIcon(f.filename());
                });
            }

            break;
        }
    }

    Layout();
}

void CrowdinLoginPanel::UpdateUserInfo()
{
    ChangeState(State::UpdatingInfo);

    CrowdinClient::Get().GetUserInfo()
        .then_on_window(this, [=](CrowdinClient::UserInfo u) {
            m_userName = u.name;
            m_userLogin = u.login;
            m_userAvatar = u.avatar;
            ChangeState(State::SignedIn);
        })
        .catch_all([](dispatch::exception_ptr){});
}

void CrowdinLoginPanel::OnSignIn(wxCommandEvent&)
{
    ChangeState(State::Authenticating);
    CrowdinClient::Get().Authenticate()
        .then_on_window(this, &CrowdinLoginPanel::OnUserSignedIn);
}

void CrowdinLoginPanel::OnUserSignedIn()
{
    UpdateUserInfo();
    Raise();
}

void CrowdinLoginPanel::OnSignOut(wxCommandEvent&)
{
    CrowdinClient::Get().SignOut();
    ChangeState(State::SignedOut);
}


LearnAboutCrowdinLink::LearnAboutCrowdinLink(wxWindow *parent, const wxString& text)
    : LearnMoreLink(parent,
                    CrowdinClient::AttributeLink("/"),
                    text.empty() ? (MSW_OR_OTHER(_("Learn more about Crowdin"), _("Learn More About Crowdin"))) : text)
{
}



namespace
{

class CrowdinLoginDialog : public wxDialog
{
public:
    CrowdinLoginDialog(wxWindow *parent) : wxDialog(parent, wxID_ANY, _("Sign in to Crowdin"))
    {
        auto topsizer = new wxBoxSizer(wxHORIZONTAL);
        auto panel = new Panel(this);
        panel->SetClientSize(panel->GetBestSize());
        topsizer->Add(panel, wxSizerFlags(1).Expand().Border(wxALL, PX(15)));
        SetSizerAndFit(topsizer);
        CenterOnParent();
    }

private:
    class Panel : public CrowdinLoginPanel
    {
    public:
        Panel(CrowdinLoginDialog *parent) : CrowdinLoginPanel(parent, DialogButtons), m_owner(parent)
        {
            EnsureInitialized();
        }

    protected:
        void OnUserSignedIn() override
        {
            m_owner->Raise();
            m_owner->EndModal(wxID_OK);
        }

        CrowdinLoginDialog *m_owner;
    };
};


class CrowdinFileList : public wxDataViewListCtrl
{
public:
    using FileInfo = CrowdinClient::FileInfo;

    CrowdinFileList(wxWindow *parent)
        : wxDataViewListCtrl(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                             wxDV_NO_HEADER | MSW_OR_OTHER(wxBORDER_SIMPLE, wxBORDER_SUNKEN))
    {
    #if wxCHECK_VERSION(3,1,1)
        SetRowHeight(PX(36));
    #endif
        SetMinSize(wxSize(PX(500), PX(200)));

        auto renderer = new MultilineTextRenderer();
        auto column = new wxDataViewColumn(_("File"), renderer, 0, -1, wxALIGN_NOT, wxDATAVIEW_COL_RESIZABLE);
        AppendColumn(column, "string");

        ColorScheme::SetupWindowColors(this, [=]{ RefreshFileList(); });
    }

    void ClearFiles()
    {
        m_files.clear();
        DeleteAllItems();
    }

    void SetFiles(std::vector<FileInfo>& files)
    {
        m_files = files;
        RefreshFileList();
    }

private:
    void RefreshFileList()
    {
    #ifdef __WXGTK__
        auto secondaryFormatting = "alpha='50%'";
    #else
        auto secondaryFormatting = wxString::Format("foreground='%s'", ColorScheme::Get(Color::SecondaryLabel).GetAsString(wxC2S_HTML_SYNTAX));
    #endif

        DeleteAllItems();

        for (auto& f : m_files)
        {
        #if wxCHECK_VERSION(3,1,1)
            wxString details;
            if (!f.branchName.empty())
                details += str::to_wx(f.branchName) + L" → ";
            details += str::to_wx(f.fullPath);

            wxString text = wxString::Format
            (
                "%s\n<small><span %s>%s</span></small>",
                EscapeCString(f.title),
                secondaryFormatting,
                EscapeCString(details)
            );
        #else
            wxString text = str::to_wx(f.title);
        #endif

            wxVector<wxVariant> data;
            data.push_back({text});
            AppendItem(data);
        }
    }

    class MultilineTextRenderer : public wxDataViewTextRenderer
    {
    public:
        MultilineTextRenderer() : wxDataViewTextRenderer()
        {
    #if wxCHECK_VERSION(3,1,1)
            EnableMarkup();
    #endif
        }

    #ifdef __WXMSW__
        bool Render(wxRect rect, wxDC *dc, int state)
        {
            int flags = 0;
            if ( state & wxDATAVIEW_CELL_SELECTED )
                flags |= wxCONTROL_SELECTED;
            
            for (auto& line: wxSplit(m_text, '\n'))
            {
                wxItemMarkupText markup(line);
                markup.Render(GetView(), *dc, rect, flags, GetEllipsizeMode());
                rect.y += rect.height / 2;
            }
            
            return true;
        }

        wxSize GetSize() const
        {
            if (m_text.empty())
                return wxSize(wxDVC_DEFAULT_RENDERER_SIZE,wxDVC_DEFAULT_RENDERER_SIZE);

            auto size = wxDataViewTextRenderer::GetSize();
            size.y *= 2; // approximation enough for our needs
            return size;
        }
    #endif // __WXMSW__
    };

private:
    std::vector<FileInfo> m_files;
};


class CrowdinOpenDialog : public wxDialog
{
public:
    CrowdinOpenDialog(wxWindow *parent) : wxDialog(parent, wxID_ANY, _("Open Crowdin translation"), wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
    {
        auto topsizer = new wxBoxSizer(wxVERTICAL);
        topsizer->SetMinSize(PX(400), -1);

        auto loginSizer = new wxBoxSizer(wxHORIZONTAL);
        topsizer->AddSpacer(PX(6));
        topsizer->Add(loginSizer, wxSizerFlags().Right().PXDoubleBorder(wxLEFT|wxRIGHT));
        auto loginText = new SecondaryLabel(this, "");
        auto loginImage = new AvatarIcon(this, wxSize(PX(24), PX(24)));
        loginSizer->Add(loginText, wxSizerFlags().ReserveSpaceEvenIfHidden().Center().Border(wxRIGHT, PX(5)));
        loginSizer->Add(loginImage, wxSizerFlags().ReserveSpaceEvenIfHidden().Center());
        FetchLoginInfo(loginText, loginImage);

        auto pickers = new wxFlexGridSizer(2, wxSize(PX(5),PX(6)));
        pickers->AddGrowableCol(1);
        topsizer->Add(pickers, wxSizerFlags().Expand().PXDoubleBorderAll());

        pickers->Add(new wxStaticText(this, wxID_ANY, _("Project:")), wxSizerFlags().CenterVertical().Right());
        m_project = new wxChoice(this, wxID_ANY);
        pickers->Add(m_project, wxSizerFlags().Expand().CenterVertical());

        pickers->Add(new wxStaticText(this, wxID_ANY, _("Language:")), wxSizerFlags().CenterVertical().Right());
        m_language = new wxChoice(this, wxID_ANY);
        pickers->Add(m_language, wxSizerFlags().Expand().CenterVertical());

        m_files = new CrowdinFileList(this);
        topsizer->Add(m_files, wxSizerFlags(1).Expand().PXDoubleBorderAll());

        m_activity = new ActivityIndicator(this);
        topsizer->Add(m_activity, wxSizerFlags().Expand().PXDoubleBorder(wxLEFT|wxRIGHT));
        topsizer->AddSpacer(MSW_OR_OTHER(PX(4), PX(2)));

        auto buttons = CreateButtonSizer(wxOK | wxCANCEL);
        auto ok = static_cast<wxButton*>(FindWindow(wxID_OK));
        ok->SetDefault();
    #ifdef __WXOSX__
        topsizer->Add(buttons, wxSizerFlags().Expand());
    #else
        topsizer->Add(buttons, wxSizerFlags().Expand().PXBorderAll());
        topsizer->AddSpacer(PX(5));
    #endif

        SetSizerAndFit(topsizer);
        CenterOnParent();

        m_project->Bind(wxEVT_CHOICE, [=](wxCommandEvent&){ OnProjectSelected(); });
        ok->Bind(wxEVT_UPDATE_UI, &CrowdinOpenDialog::OnUpdateOK, this);
        ok->Bind(wxEVT_BUTTON, &CrowdinOpenDialog::OnOK, this);

        ok->Disable();
        EnableAllChoices(false);

        FetchProjects();
    }

    wxString OutLocalFilename;

private:
    void FetchLoginInfo(SecondaryLabel *label, AvatarIcon *icon)
    {
        label->Hide();
        icon->Hide();

        CrowdinClient::Get().GetUserInfo()
        .then_on_window(this, [=](CrowdinClient::UserInfo u)
        {
            label->SetLabel(_("Signed in as:") + " " + u.name);
            icon->SetUserName(u.name);
            if (u.avatar.empty())
            {
                icon->Show();
            }
            else
            {
                http_client::download_from_anywhere(u.avatar)
                .then_on_window(this, [=](downloaded_file f)
                {
                    icon->LoadIcon(f.filename());
                    icon->Show();
                });
            }
            Layout();
            label->Show();
        })
        .catch_all([](dispatch::exception_ptr){});
    }

    void EnableAllChoices(bool enable = true)
    {
        m_project->Enable(enable);
        m_language->Enable(enable);
        m_files->Enable(enable);
    }

    void FetchProjects()
    {
        m_activity->Start();
        CrowdinClient::Get().GetUserProjects()
            .then_on_window(this, &CrowdinOpenDialog::OnFetchedProjects)
            .catch_all(m_activity->HandleError);
    }

    void OnFetchedProjects(std::vector<CrowdinClient::ProjectListing> prjs)
    {
        m_projects = prjs;
        m_project->Append("");
        for (auto& p: prjs)
            m_project->Append(p.name);
        m_project->Enable(!prjs.empty());

        if (prjs.empty())
            m_activity->StopWithError(_("No translation projects listed in your Crowdin account."));
        else
            m_activity->Stop();

        if (prjs.size() == 1)
        {
            m_project->SetSelection(1);
            OnProjectSelected();
        }
    }

    void OnProjectSelected()
    {
        auto sel = m_project->GetSelection();
        if (sel > 0)
        {
            m_activity->Start();
            EnableAllChoices(false);
            m_files->ClearFiles();
            CrowdinClient::Get().GetProjectInfo(m_projects[sel-1].id)
                .then_on_window(this, &CrowdinOpenDialog::OnFetchedProjectInfo)
                .catch_all([=](dispatch::exception_ptr e){
                    m_activity->HandleError(e);
                    EnableAllChoices(true);
                });
        }
    }

    void OnFetchedProjectInfo(CrowdinClient::ProjectInfo prj)
    {
        m_info = prj;
        m_language->Clear();
        m_language->Append("");
        for (auto& i: m_info.languages)
            m_language->Append(i.DisplayName());

        m_files->SetFiles(m_info.files);

        EnableAllChoices();
        m_activity->Stop();

        if (m_info.languages.size() == 1)
        {
            m_language->SetSelection(1);
        }
        else
        {
            auto preferred = LanguageDialog::GetLastChosen();
            if (preferred.IsValid())
            {
                for (size_t i = 0; i < m_info.languages.size(); i++)
                {
                    if (m_info.languages[i] == preferred)
                    {
                        m_language->SetSelection(1 + int(i));
                        break;
                    }
                }
            }
        }

        if (m_info.files.size() == 1)
            m_files->SelectRow(0);

    }

    void OnUpdateOK(wxUpdateUIEvent& e)
    {
        e.Enable(!m_activity->IsRunning() &&
                 m_project->GetSelection() > 0 &&
                 m_language->GetSelection() > 0 &&
                 m_files->GetSelectedRow() != wxNOT_FOUND);
    }

    void OnOK(wxCommandEvent&)
    {
        auto crowdin_file = m_info.files[m_files->GetSelectedRow()];
        auto crowdin_lang = m_info.languages[m_language->GetSelection() - 1];
        LanguageDialog::SetLastChosen(crowdin_lang);
        OutLocalFilename = CreateLocalFilename(crowdin_file.id, crowdin_file.fileName, crowdin_lang, m_info.id, m_info.name);

        m_activity->Start(_(L"Downloading latest translations…"));

        auto outfile = std::make_shared<TempOutputFileFor>(OutLocalFilename);
        CrowdinClient::Get().DownloadFile(
                m_info.id,
                crowdin_lang,
                crowdin_file.id,
                std::string(wxFileName(crowdin_file.fileName, wxPATH_UNIX).GetExt().utf8_str()),
                outfile->FileName().ToStdWstring()
            )
            .then_on_window(this, [=]{
                outfile->Commit();
                AcceptAndClose();
            })
            .catch_all(m_activity->HandleError);
    }

    wxString CreateLocalFilename(int fileId, std::string fileName, const Language& lang, int projectId, const wxString& projectName)
    {
        std::replace_if(fileName.begin(), fileName.end(), boost::is_any_of("\\/:\"<>|?*"), '_');

        wxFileName crowdinFileName(fileName, wxPATH_UNIX);
        wxFileName localFileName(
            CloudSyncDestination::GetCacheDir() + wxFILE_SEP_PATH + "Crowdin"
            + wxFILE_SEP_PATH + projectName
            + wxFILE_SEP_PATH + lang.Code()
            + wxFILE_SEP_PATH + crowdinFileName.GetPath() + wxFILE_SEP_PATH
            + "CrowdinSync_" << projectId << '_' << fileId << '_' << crowdinFileName.GetFullName());

        auto ext = localFileName.GetExt().Lower();
        if (ext == "po")
        {
            // natively supported file format, will be opened as-is
        }
        else if (ext == "pot")
        {
            // POT files are natively supported, but need to be opened as PO to see translations
            localFileName.SetFullName(localFileName.GetFullName() + ".po");
        }
        else
        {
            // Everything else is exported as XLIFF
            localFileName.SetFullName(localFileName.GetFullName() + ".xliff");
        }

        if (!wxFileName::DirExists(localFileName.GetPath()))
            wxFileName::Mkdir(localFileName.GetPath(), wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);

        return localFileName.GetFullPath();
    }

private:
    wxButton *m_ok;
    wxChoice *m_project, *m_language;
    CrowdinFileList *m_files;
    ActivityIndicator *m_activity;

    std::vector<CrowdinClient::ProjectListing> m_projects;
    CrowdinClient::ProjectInfo m_info;
};

} // anonymous namespace


dispatch::future<void> CrowdinUpdateCatalogIDsFromHeaders(CatalogPtr catalog)
{
    if (catalog->GetCrowdinProjectId() > 0 && catalog->GetCrowdinFileId() > 0)
        return dispatch::make_ready_future();

    wxString projIdentifier = catalog->Header().GetHeader("X-Crowdin-Project");
    auto filePathName = catalog->Header().GetHeader("X-Crowdin-File");

    if (!filePathName.StartsWith(L"/"))
        filePathName.insert(0, L'/');

    return CrowdinClient::Get().GetUserProjects().then([=](std::vector<CrowdinClient::ProjectListing> projects)
    {
        auto projIt = find_if(projects.begin(), projects.end(),
                                [=](const auto& proj) { return projIdentifier == proj.identifier; });
        if (projIt == projects.end())
        {
            throw Exception(_(L"This file belongs to a Crowdin project that you don’t have access to."));
        }

        return CrowdinClient::Get().GetProjectInfo(projIt->id).then([=](CrowdinClient::ProjectInfo proj)
        {
            auto fileIt = find_if(proj.files.begin(), proj.files.end(),
                                  [=](const auto& file) { return filePathName == file.fullPath; });
            if (fileIt == proj.files.end())
            {
                throw Exception(wxString::Format(_(L"File “%s” doesn’t exist in Crowdin project “%s”."), filePathName, projIdentifier));
            }
            catalog->SetCrowdinProjectAndFileId(proj.id, fileIt->id);
        });
    });

}

void CrowdinOpenFile(wxWindow *parent, std::function<void(wxString)> onLoaded)
{
    if (!CrowdinClient::Get().IsSignedIn())
    {
        wxWindowPtr<CrowdinLoginDialog> login(new CrowdinLoginDialog(parent));
        login->ShowWindowModalThenDo([login,parent,onLoaded](int retval){
            if (retval == wxID_OK)
                CrowdinOpenFile(parent, onLoaded);
        });
        return;
    }

    wxWindowPtr<CrowdinOpenDialog> dlg(new CrowdinOpenDialog(parent));

    dlg->ShowWindowModalThenDo([dlg,onLoaded](int retval) {
        dlg->Hide();
        if (retval == wxID_OK)
            onLoaded(dlg->OutLocalFilename);
    });
}


void CrowdinSyncFile(wxWindow *parent, std::shared_ptr<Catalog> catalog,
                     std::function<void(std::shared_ptr<Catalog>)> onDone)
{
    if (!CrowdinClient::Get().IsSignedIn())
    {
        wxWindowPtr<CrowdinLoginDialog> login(new CrowdinLoginDialog(parent));
        login->ShowWindowModalThenDo([login,parent,catalog,onDone](int retval){
            if (retval == wxID_OK)
                CrowdinSyncFile(parent, catalog, onDone);
        });
        return;
    }

    wxLogTrace("poedit.crowdin", "Crowdin syncing file ...");

    wxWindowPtr<CloudSyncProgressWindow> dlg(new CloudSyncProgressWindow(parent));

    auto handle_error = [=](dispatch::exception_ptr e){
        dispatch::on_main([=]{
            dlg->EndModal(wxID_CANCEL);
            wxWindowPtr<wxMessageDialog> err(new wxMessageDialog
                (
                    parent,
                    _("Syncing with Crowdin failed."),
                    _("Crowdin error"),
                    wxOK | wxICON_ERROR
                ));
            err->SetExtendedMessage(DescribeException(e));
            err->ShowWindowModalThenDo([err](int){});
        });
    };

    dlg->Activity->Start(_(L"Uploading translations…"));

    // TODO: nicer API for this.
    // This must be done right after entering the modal loop (on non-OSX)
    dlg->CallAfter([=]{
        const auto& header = catalog->Header();
        auto crowdin_lang = header.HasHeader("X-Crowdin-Language")
                            ? Language::TryParse(header.GetHeader("X-Crowdin-Language").ToStdWstring())
                            : catalog->GetLanguage();
        CrowdinUpdateCatalogIDsFromHeaders(catalog)
        .then([=]()
        {
        CrowdinClient::Get().UploadFile(
                catalog->GetCrowdinProjectId(),
                crowdin_lang,
                catalog->GetCrowdinFileId(),
                std::string(wxFileName(catalog->GetFileName()).GetExt().utf8_str()),
                catalog->SaveToBuffer()
            )
            .then([=]
            {
                wxFileName filename = catalog->GetFileName();
                auto tmpdir = std::make_shared<TempDirectory>();
                auto outfile = tmpdir->CreateFileName("crowdin." + filename.GetExt());

                dispatch::on_main([=]{
                    dlg->Activity->Start(_(L"Downloading latest translations…"));
                });

                if (filename.GetExt().CmpNoCase("po") != 0)  // if not PO
                    filename.SetFullName(filename.GetName());  // set remote (Crowdin side) filename extension

                return CrowdinClient::Get().DownloadFile(
                        catalog->GetCrowdinProjectId(),
                        crowdin_lang,
                        catalog->GetCrowdinFileId(),
                        std::string(filename.GetExt().utf8_str()),
                        outfile.ToStdWstring()
                    )
                    .then_on_main([=]
                    {
                        auto newcat = Catalog::Create(outfile);
                        newcat->SetFileName(catalog->GetFileName());

                        tmpdir->Clear();
                        dlg->EndModal(wxID_OK);

                        onDone(newcat);
                    })
                    .catch_all(handle_error);
            })
            .catch_all(handle_error);
        });
    });

    dlg->ShowWindowModal();
}

bool CrowdinSyncDestination::AuthIfNeeded(wxWindow* parent) {
    return CrowdinClient::Get().IsSignedIn()
            || CrowdinLoginDialog(parent).ShowModal() == wxID_OK;
}

dispatch::future<void> CrowdinSyncDestination::Upload(CatalogPtr file)
{
    wxLogTrace("poedit.crowdin", "Uploading file: %s", file->GetFileName().c_str());
    const auto& header = file->Header();
    auto crowdin_lang = header.HasHeader("X-Crowdin-Language")
                        ? Language::TryParse(header.GetHeader("X-Crowdin-Language").ToStdWstring())
                        : file->GetLanguage();

    return CrowdinUpdateCatalogIDsFromHeaders(file)
    .then([=]()
    {
        return CrowdinClient::Get().UploadFile
               (
                   file->GetCrowdinProjectId(),
                   crowdin_lang,
                   file->GetCrowdinFileId(),
                   std::string(wxFileName(file->GetFileName()).GetExt().utf8_str()),
                   file->SaveToBuffer()
               );
    });
}
