/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 2015-2017 Vaclav Slavik
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
#include "concurrency.h"
#include "customcontrols.h"
#include "errors.h"
#include "hidpi.h"
#include "languagectrl.h"
#include "str_helpers.h"
#include "utility.h"

#include <wx/app.h>
#include <wx/artprov.h>
#include <wx/button.h>
#include <wx/choice.h>
#include <wx/config.h>
#include <wx/dialog.h>
#include <wx/msgdlg.h>
#include <wx/sizer.h>
#include <wx/statbmp.h>
#include <wx/stdpaths.h>
#include <wx/weakref.h>
#include <wx/windowptr.h>

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
    auto logo = new wxStaticBitmap(this, wxID_ANY, wxArtProvider::GetBitmap("CrowdinLogo"));
    logo->SetCursor(wxCURSOR_HAND);
    logo->Bind(wxEVT_LEFT_UP, [](wxMouseEvent&){ wxLaunchDefaultBrowser(CrowdinClient::WrapLink("/")); });
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
            auto account = new wxStaticText(this, wxID_ANY, _("Signed in as:"));
            account->SetForegroundColour(SecondaryLabel::GetTextColor());
            auto name = new wxStaticText(this, wxID_ANY, m_userName);
#ifdef __WXGTK3__
            // This is needed to avoid missizing text with bold font. See
            // https://github.com/vslavik/poedit/pull/411 and https://trac.wxwidgets.org/ticket/16088
            name->SetLabelMarkup("<b>" + EscapeMarkup(m_userName) + "</b>");
#else
            name->SetFont(name->GetFont().Bold());
#endif

            auto username = new SecondaryLabel(this, m_userLogin);

            sizer->Add(account, wxSizerFlags().BORDER_MACOS(wxTOP, PX(3)));
            sizer->AddSpacer(PX(2));
            auto box = new wxBoxSizer(wxVERTICAL);
            box->Add(name, wxSizerFlags().Left());
            box->Add(username, wxSizerFlags().Left());
            sizer->Add(box);
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
                    CrowdinClient::WrapLink("/"),
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


class CrowdinOpenDialog : public wxDialog
{
public:
    CrowdinOpenDialog(wxWindow *parent) : wxDialog(parent, wxID_ANY, _("Open Crowdin translation"))
    {
        auto topsizer = new wxBoxSizer(wxVERTICAL);
        topsizer->SetMinSize(PX(400), -1);

        topsizer->AddSpacer(PX(10));

        auto pickers = new wxFlexGridSizer(2, wxSize(PX(5),PX(6)));
        pickers->AddGrowableCol(1);
        topsizer->Add(pickers, wxSizerFlags().Expand().PXDoubleBorderAll());

        pickers->Add(new wxStaticText(this, wxID_ANY, _("Project:")),
                     wxSizerFlags().CenterVertical().Right().BORDER_MACOS(wxTOP, 1));
        m_project = new wxChoice(this, wxID_ANY);
        pickers->Add(m_project, wxSizerFlags().Expand().CenterVertical());

        pickers->Add(new wxStaticText(this, wxID_ANY, _("Language:")),
                     wxSizerFlags().CenterVertical().Right().BORDER_MACOS(wxTOP, 1));
        m_language = new wxChoice(this, wxID_ANY);
        pickers->Add(m_language, wxSizerFlags().Expand().CenterVertical());

        pickers->AddSpacer(PX(5));
        pickers->AddSpacer(PX(5));

        pickers->Add(new wxStaticText(this, wxID_ANY, _("File:")),
                     wxSizerFlags().CenterVertical().Right().BORDER_MACOS(wxTOP, 1));
        m_file = new wxChoice(this, wxID_ANY);
        pickers->Add(m_file, wxSizerFlags().Expand().CenterVertical());

        m_activity = new ActivityIndicator(this);
        topsizer->AddSpacer(PX(5));
        topsizer->Add(m_activity, wxSizerFlags().Expand().PXDoubleBorder(wxLEFT|wxRIGHT));
        topsizer->AddSpacer(PX(5));

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
        m_file->Bind(wxEVT_CHOICE, [=](wxCommandEvent&){ OnFileSelected(); });
        ok->Bind(wxEVT_UPDATE_UI, &CrowdinOpenDialog::OnUpdateOK, this);
        ok->Bind(wxEVT_BUTTON, &CrowdinOpenDialog::OnOK, this);

        ok->Disable();
        EnableAllChoices(false);

        FetchProjects();
    }

    wxString OutLocalFilename;

private:
    void EnableAllChoices(bool enable = true)
    {
        m_project->Enable(enable);
        m_file->Enable(enable);
        m_language->Enable(enable);
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
            CrowdinClient::Get().GetProjectInfo(m_projects[sel-1].identifier)
                .then_on_window(this, &CrowdinOpenDialog::OnFetchedProjectInfo)
                .catch_all(m_activity->HandleError);
        }
    }

    void OnFetchedProjectInfo(CrowdinClient::ProjectInfo prj)
    {
        m_info = prj;
        // Put supported files first in the list:
        std::vector<std::wstring> f_unsup;
        m_info.files.clear();
        m_supportedFilesCount = 0;
        for (auto& i: prj.files)
        {
            if (IsFileSupported(i))
            {
                m_info.files.push_back(i);
                m_supportedFilesCount++;
            }
            else
            {
                f_unsup.push_back(i);
            }
        }
        std::move(f_unsup.begin(), f_unsup.end(), std::inserter(m_info.files, m_info.files.end()));

        m_language->Clear();
        m_language->Append("");
        for (auto& i: m_info.languages)
            m_language->Append(i.DisplayName());

        m_file->Clear();
        m_file->Append("");
        for (auto& i: m_info.files)
        {
            if (IsFileSupported(i))
            {
                m_file->Append(i);
            }
            else
            {
                /// TRANSLATORS: This is a file selector list, %s is filename, and it is shown for Crowdin files not editable in Poedit
                m_file->Append(wxString::Format(L"%s — not supported", i));
            }
        }

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

        if (m_supportedFilesCount == 1)
            m_file->SetSelection(1);

        if (m_supportedFilesCount == 0)
        {
            m_activity->StopWithError(_("This project has no files that can be translated in Poedit."));
            m_file->Disable();
            m_language->Disable();
        }
    }

    void OnFileSelected()
    {
        auto filesel = m_file->GetSelection();
        if (filesel - 1 < m_supportedFilesCount)
            m_activity->Stop();
        else
            m_activity->StopWithError(_(L"This file can only be edited in Crowdin’s web interface."));
    }

    void OnUpdateOK(wxUpdateUIEvent& e)
    {
        auto filesel = m_file->GetSelection();
        e.Enable(!m_activity->IsRunning() &&
                 m_project->GetSelection() > 0 &&
                 m_language->GetSelection() > 0 &&
                 filesel > 0 && filesel - 1 < m_supportedFilesCount);
    }

    void OnOK(wxCommandEvent&)
    {
        auto crowdin_prj = m_info.identifier;
        auto crowdin_file = m_info.files[m_file->GetSelection() - 1];
        auto crowdin_lang = m_info.languages[m_language->GetSelection() - 1];
        LanguageDialog::SetLastChosen(crowdin_lang);
        OutLocalFilename = CreateLocalFilename(crowdin_file, crowdin_lang);

        m_activity->Start(_(L"Downloading latest translations…"));

        auto outfile = std::make_shared<TempOutputFileFor>(OutLocalFilename);
        CrowdinClient::Get().DownloadFile(
                crowdin_prj, crowdin_file, crowdin_lang,
                outfile->FileName().ToStdWstring()
            )
            .then_on_window(this, [=]{
                outfile->Commit();
                AcceptAndClose();
            })
            .catch_all(m_activity->HandleError);
    }

    bool IsFileSupported(const wxString& name) const
    {
        return boost::ends_with(name, ".po") || boost::ends_with(name, ".pot");
    }

    wxString CreateLocalFilename(const wxString& name, const Language& lang)
    {
        wxString cache;
    #if defined(__WXOSX__)
        cache = wxGetHomeDir() + "/Library/Caches/net.poedit.Poedit";
    #elif defined(__UNIX__)
        if (!wxGetEnv("XDG_CACHE_HOME", &cache))
            cache = wxGetHomeDir() + "/.cache";
        cache += "/poedit";
    #else
        cache = wxStandardPaths::Get().GetUserDataDir() + wxFILE_SEP_PATH + "Cache";
    #endif

        cache += wxFILE_SEP_PATH;
        cache += "Crowdin";

        if (!wxFileName::DirExists(cache))
            wxFileName::Mkdir(cache, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);

        auto basename = name.AfterLast('/').BeforeLast('.');

        return wxString::Format("%s%c%s_%s_%s.po", cache, wxFILE_SEP_PATH, m_info.name, basename, lang.Code());
    }

private:
    wxButton *m_ok;
    wxChoice *m_project, *m_file, *m_language;
    ActivityIndicator *m_activity;

    std::vector<CrowdinClient::ProjectListing> m_projects;
    CrowdinClient::ProjectInfo m_info;
    int m_supportedFilesCount;
};

} // anonymous namespace


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

    const auto& header = catalog->Header();
    auto crowdin_prj = header.GetHeader("X-Crowdin-Project");
    auto crowdin_file = header.GetHeader("X-Crowdin-File");
    auto crowdin_lang = header.HasHeader("X-Crowdin-Language")
                        ? Language::TryParse(header.GetHeader("X-Crowdin-Language").ToStdWstring())
                        : catalog->GetLanguage();

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
        CrowdinClient::Get().UploadFile(
                str::to_utf8(crowdin_prj), str::to_wstring(crowdin_file), crowdin_lang,
                catalog->SaveToBuffer()
            )
            .then([=]{
                auto tmpdir = std::make_shared<TempDirectory>();
                auto outfile = tmpdir->CreateFileName("crowdin.po");

                dispatch::on_main([=]{
                    dlg->Activity->Start(_(L"Downloading latest translations…"));
                });

                return CrowdinClient::Get().DownloadFile(
                        str::to_utf8(crowdin_prj), str::to_wstring(crowdin_file), crowdin_lang,
                        outfile.ToStdWstring()
                    )
                    .then_on_main([=]
                    {
                        CatalogPtr newcat = std::make_shared<Catalog>(outfile);
                        newcat->SetFileName(catalog->GetFileName());

                        tmpdir->Clear();
                        dlg->EndModal(wxID_OK);

                        onDone(newcat);
                    })
                    .catch_all(handle_error);
            })
            .catch_all(handle_error);
    });

    dlg->ShowWindowModal();
}


dispatch::future<void> CrowdinSyncDestination::Upload(CatalogPtr file)
{
    const auto& header = file->Header();
    auto crowdin_prj = header.GetHeader("X-Crowdin-Project");
    auto crowdin_file = header.GetHeader("X-Crowdin-File");
    auto crowdin_lang = header.HasHeader("X-Crowdin-Language")
                        ? Language::TryParse(header.GetHeader("X-Crowdin-Language").ToStdWstring())
                        : file->GetLanguage();

    return CrowdinClient::Get().UploadFile(
                str::to_utf8(crowdin_prj), str::to_wstring(crowdin_file), crowdin_lang,
                file->SaveToBuffer()
            );
}
