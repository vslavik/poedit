/*
 *  This file is part of Poedit (http://poedit.net)
 *
 *  Copyright (C) 2015 Vaclav Slavik
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

#include "customcontrols.h"
#include "hidpi.h"
#include "utility.h"

#include <wx/app.h>
#include <wx/artprov.h>
#include <wx/button.h>
#include <wx/choice.h>
#include <wx/config.h>
#include <wx/dialog.h>
#include <wx/sizer.h>
#include <wx/statbmp.h>
#include <wx/stdpaths.h>
#include <wx/weakref.h>
#include <wx/windowptr.h>


CrowdinLoginPanel::CrowdinLoginPanel(wxWindow *parent, int flags)
    : wxPanel(parent, wxID_ANY),
      m_state(State::SignedOut)
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
    buttons->Add(m_signIn, wxSizerFlags().Right());
    buttons->Add(m_signOut, wxSizerFlags().Right());

    if (flags & DialogButtons)
    {
        auto cancel = new wxButton(this, wxID_CANCEL);
#ifdef __WXMSW__
        buttons->Add(cancel, wxSizerFlags().Right().Border(wxLEFT, PX(3)));
#else
        buttons->Insert(2, cancel, wxSizerFlags().Right().Border(wxRIGHT, PX(6)));
#endif
        m_signIn->SetDefault();
        m_signIn->SetFocus();
    }

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
            name->SetFont(name->GetFont().Bold());
            auto username = new SecondaryLabel(this, m_userLogin);

            sizer->Add(account, wxSizerFlags().PXBorder(wxRIGHT));
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

    CrowdinClient::Get().GetUserInfo(on_main_thread_for_window<CrowdinClient::UserInfo>(
        this,
        [=](CrowdinClient::UserInfo u) {
            m_userName = u.name;
            m_userLogin = u.login;
            ChangeState(State::SignedIn);
    }));
}

void CrowdinLoginPanel::OnSignIn(wxCommandEvent&)
{
    ChangeState(State::Authenticating);
    CrowdinClient::Get().Authenticate(on_main_thread(this, &CrowdinLoginPanel::OnUserSignedIn));
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
        Panel(CrowdinLoginDialog *parent) : CrowdinLoginPanel(parent, DialogButtons), m_owner(parent) {}

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
                     wxSizerFlags().Center().Right().BORDER_OSX(wxTOP, 1));
        m_project = new wxChoice(this, wxID_ANY);
        pickers->Add(m_project, wxSizerFlags().Expand().Center());

        pickers->Add(new wxStaticText(this, wxID_ANY, _("Language:")),
                     wxSizerFlags().Center().Right().BORDER_OSX(wxTOP, 1));
        m_language = new wxChoice(this, wxID_ANY);
        pickers->Add(m_language, wxSizerFlags().Expand().Center());

        pickers->AddSpacer(PX(5));
        pickers->AddSpacer(PX(5));

        pickers->Add(new wxStaticText(this, wxID_ANY, _("File:")),
                     wxSizerFlags().Center().Right().BORDER_OSX(wxTOP, 1));
        m_file = new wxChoice(this, wxID_ANY);
        pickers->Add(m_file, wxSizerFlags().Expand().Center());

        m_activity = new ActivityIndicator(this);
        topsizer->Add(m_activity, wxSizerFlags().Expand().PXDoubleBorder(wxLEFT|wxRIGHT|wxTOP));

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
    void EnableAllChoices(bool enable = true)
    {
        m_project->Enable(enable);
        m_file->Enable(enable);
        m_language->Enable(enable);
    }

    void FetchProjects()
    {
        m_activity->Start();
        CrowdinClient::Get().GetUserProjects(
            on_main_thread(this, &CrowdinOpenDialog::OnFetchedProjects),
            m_activity->HandleError
        );
    }

    void OnFetchedProjects(const std::vector<CrowdinClient::ProjectListing>& prjs)
    {
        m_projects = prjs;
        m_project->Append("");
        for (auto& p: prjs)
            m_project->Append(p.name);
        m_project->Enable();
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
            CrowdinClient::Get().GetProjectInfo(
                m_projects[sel-1].identifier,
                on_main_thread(this, &CrowdinOpenDialog::OnFetchedProjectInfo),
                m_activity->HandleError
            );
        }
    }

    void OnFetchedProjectInfo(const CrowdinClient::ProjectInfo& prj)
    {
        m_info = prj;

        m_language->Clear();
        m_language->Append("");
        for (auto& i: prj.languages)
            m_language->Append(i.DisplayName());

        m_file->Clear();
        m_file->Append("");
        for (auto& i: prj.po_files)
            m_file->Append(i);

        EnableAllChoices();
        m_activity->Stop();

        if (prj.languages.size() == 1)
            m_language->SetSelection(1);
        if (prj.po_files.size() == 1)
            m_file->SetSelection(1);
    }

    void OnUpdateOK(wxUpdateUIEvent& e)
    {
        e.Enable(!m_activity->IsRunning() &&
                 m_project->GetSelection() > 0 &&
                 m_file->GetSelection() > 0 &&
                 m_language->GetSelection() > 0);
    }

    void OnOK(wxCommandEvent&)
    {
        auto crowdin_prj = m_info.identifier;
        auto crowdin_file = m_info.po_files[m_file->GetSelection() - 1];
        auto crowdin_lang = m_info.languages[m_language->GetSelection() - 1];
        OutLocalFilename = CreateLocalFilename(crowdin_file, crowdin_lang);

        m_activity->Start(_(L"Downloading latest translations…"));

        auto outfile = std::make_shared<TempOutputFileFor>(OutLocalFilename);
        CrowdinClient::Get().DownloadFile(
            crowdin_prj, crowdin_file, crowdin_lang,
            outfile->FileName().ToStdWstring(),
            on_main_thread_for_window<>(this, [=]{
                outfile->Commit();
                AcceptAndClose();
            }),
            m_activity->HandleError
        );
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
