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


#include "crowdin_client.h"

#include "http_client.h"

#include "customcontrols.h"
#include "hidpi.h"
#include "utility.h"

#include <functional>
#include <mutex>
#include <boost/algorithm/string.hpp>

#include <wx/app.h>
#include <wx/artprov.h>
#include <wx/button.h>
#include <wx/choice.h>
#include <wx/config.h>
#include <wx/dialog.h>
#include <wx/sizer.h>
#include <wx/statbmp.h>
#include <wx/weakref.h>
#include <wx/windowptr.h>

#ifndef __WXOSX__
    #define NEEDS_IN_APP_BROWSER
    #include <wx/frame.h>
    #include <wx/webview.h>
#endif

// GCC's libstdc++ didn't have functional std::regex implementation until 4.9
#if (defined(__GNUC__) && !defined(__clang__) && !wxCHECK_GCC_VERSION(4,9))
    #include <boost/regex.hpp>
    using boost::regex;
    using boost::regex_search;
    using boost::smatch;
#else
    #include <regex>
    using std::regex;
    using std::regex_search;
    using std::smatch;
#endif

// ----------------------------------------------------------------
// Implementation
// ----------------------------------------------------------------

#define OAUTH_CLIENT_ID     "poedit-y1aBMmDbm3s164dtJ4Ur150e2"
#define OAUTH_AUTHORIZE_URL "/oauth2/authorize?response_type=token&client_id=" OAUTH_CLIENT_ID
#define OAUTH_URI_PREFIX    "poedit://auth/crowdin/"

namespace
{

std::string WrapCrowdinLink(const std::string& page)
{
    return "https://secure.payproglobal.com/r.ashx?s=12569&a=62761&u=" +
            http_client::url_encode("https://crowdin.com" + page);
}


// Recursive extract files from /api/project/*/info response
void ExtractFilesFromInfo(std::vector<std::wstring>& out, const json_dict& r, const std::wstring& prefix)
{
    r.iterate_array("files",[&out,prefix](const json_dict& i){
        auto name = prefix + i.wstring("name");
        auto node_type = i.utf8_string("node_type");
        if (node_type == "file")
        {
            if (boost::ends_with(name, ".po") || boost::ends_with(name, ".pot"))
                out.push_back(name);
        }
        else if (node_type == "directory")
        {
            ExtractFilesFromInfo(out, i, name + L"/");
        }
    });
}


} // anonymous namespace


class CrowdinClient::impl
{
public:
    impl() : m_api("https://api.crowdin.com")
    {
        SignInIfAuthorized();
    }

    void Authenticate(std::function<void()> callback)
    {
        auto url = WrapCrowdinLink(OAUTH_AUTHORIZE_URL);
        m_authCallback = callback;

#ifdef NEEDS_IN_APP_BROWSER
        auto win = new wxDialog(nullptr, wxID_ANY, _("Sign in"), wxDefaultPosition, wxSize(PX(800), PX(600)), wxDIALOG_NO_PARENT | wxDEFAULT_DIALOG_STYLE);
        auto web = wxWebView::New(win, wxID_ANY);

        // Protocol handler that simply calls a callback and doesn't return any data.
        // We have to use this hack, because wxEVT_WEBVIEW_NAVIGATING is unreliable
        // with IE on Windows 8 and is not sent at all or sent one navigation event too
        // late, possibly due to the custom protocol used, possibly not.
        class PoeditURIHandler : public wxWebViewHandler
        {
        public:
            PoeditURIHandler(std::function<void(std::string)> cb) : wxWebViewHandler("poedit"), m_cb(cb) {}
            wxFSFile* GetFile(const wxString &uri) override
            {
                m_cb(uri.ToStdString());
                return nullptr;
            }
            std::function<void(std::string)> m_cb;
        };

        web->RegisterHandler(wxSharedPtr<wxWebViewHandler>(new PoeditURIHandler([=](std::string uri){
            if (!this->IsOAuthCallback(uri))
                return;
            win->CallAfter([=]{
                win->EndModal(wxID_OK);
                this->HandleOAuthCallback(uri);
            });
        })));

        win->CallAfter([=]{ web->LoadURL(url); });
        win->ShowModal();
        win->Destroy();
#else
        wxLaunchDefaultBrowser(url);
#endif
    }

    void HandleOAuthCallback(const std::string& uri)
    {
        if (!m_authCallback)
            return;

        const regex re("access_token=([^&]+)&");
        smatch m;
        if (!regex_search(uri, m, re))
            return;

        SaveAndSetToken(m.str(1));

        m_authCallback();
        m_authCallback = nullptr;
    }

    bool IsOAuthCallback(const std::string& uri)
    {
        return boost::starts_with(uri, OAUTH_URI_PREFIX);
    }

    void GetUserInfo(std::function<void(UserInfo)> callback)
    {
        request("/api/account/profile?json=",
                // OK:
                [callback](const json_dict& r){
                    auto profile = r.subdict("profile");
                    UserInfo u;
                    u.name = profile.wstring("name");
                    u.login = profile.wstring("login");
                    callback(u);
                },
                // error:
                [](std::exception_ptr) {}
        );
    }

    void GetUserProjects(std::function<void(std::vector<ProjectListing>)> onResult,
                         CrowdinClient::error_func_t onError)
    {
        request("/api/account/get-projects?json=&role=all",
                // OK:
                [onResult](const json_dict& r){
                    std::vector<CrowdinClient::ProjectListing> all;
                    r.iterate_array("projects",[&all](const json_dict& i){
                        all.push_back({i.wstring("name"),
                                       i.utf8_string("identifier"),
                                       (bool)i.number("downloadable")});
                    });
                    onResult(all);
                },
                onError
        );
    }

    void GetProjectInfo(const std::string& project_id,
                        std::function<void(CrowdinClient::ProjectInfo)> onResult,
                        CrowdinClient::error_func_t onError)
    {
        auto url = "/api/project/" + project_id + "/info?json=&project-identifier=" + project_id;
        request(url,
                // OK:
                [onResult](const json_dict& r){
                    CrowdinClient::ProjectInfo prj;
                    auto details = r.subdict("details");
                    prj.name = details.wstring("name");
                    prj.identifier = details.utf8_string("identifier");
                    r.iterate_array("languages",[&prj](const json_dict& i){
                        if (i.number("can_translate") != 0)
                            prj.languages.push_back(Language::TryParse(i.wstring("code")));
                    });
                    ExtractFilesFromInfo(prj.po_files, r, L"/");
                    onResult(prj);
                },
                onError
        );
    }

    bool IsSignedIn() const
    {
        return wxConfig::Get()->ReadBool("/crowdin/signed_in", false);
    }

    void SignInIfAuthorized()
    {
        if (IsSignedIn())
        {
            // TODO: move to secure storage
            SetToken(wxConfig::Get()->Read("/crowdin/oauth_token").ToStdString());
        }
    }

    void SetToken(const std::string& token)
    {
        m_api.set_authorization(!token.empty() ? "Bearer " + token : "");
    }

    void SaveAndSetToken(const std::string& token)
    {
        SetToken(token);
        auto *cfg = wxConfig::Get();
        cfg->Write("/crowdin/signed_in", true);
        cfg->Write("/crowdin/oauth_token", wxString(token));
        cfg->Flush();
    }

    void SignOut()
    {
        m_api.set_authorization("");
        wxConfig::Get()->DeleteGroup("/crowdin");
    }

private:
    template <typename T1, typename T2>
    void request(const std::string& url, const T1& onResult, const T2& onError)
    {
        m_api.request(url, [onResult,onError](const http_response& r){
            try
            {
                onResult(r.json());
            }
            catch (...)
            {
                onError(std::current_exception());
            }
        });
    }

    http_client m_api;
    std::function<void()> m_authCallback;
};



// ----------------------------------------------------------------
// Singleton management
// ----------------------------------------------------------------

static std::once_flag initializationFlag;
CrowdinClient* CrowdinClient::ms_instance = nullptr;

CrowdinClient& CrowdinClient::Get()
{
    std::call_once(initializationFlag, []() {
        ms_instance = new CrowdinClient;
    });
    return *ms_instance;
}

void CrowdinClient::CleanUp()
{
    if (ms_instance)
    {
        delete ms_instance;
        ms_instance = nullptr;
    }
}

CrowdinClient::CrowdinClient() : m_impl(new impl) {}
CrowdinClient::~CrowdinClient() {}


// ----------------------------------------------------------------
// Public API
// ----------------------------------------------------------------

bool CrowdinClient::IsSignedIn() const
{
    return m_impl->IsSignedIn();
}

void CrowdinClient::Authenticate(std::function<void()> callback)
{
    return m_impl->Authenticate(callback);
}

void CrowdinClient::HandleOAuthCallback(const std::string& uri)
{
    return m_impl->HandleOAuthCallback(uri);
}

bool CrowdinClient::IsOAuthCallback(const std::string& uri)
{
    return m_impl->IsOAuthCallback(uri);
}

void CrowdinClient::SignOut()
{
    m_impl->SignOut();
}

void CrowdinClient::GetUserInfo(std::function<void(UserInfo)> callback)
{
    m_impl->GetUserInfo(callback);
}

void CrowdinClient::GetUserProjects(std::function<void(std::vector<ProjectListing>)> onResult,
                                    error_func_t onError)
{
    m_impl->GetUserProjects(onResult, onError);
}

void CrowdinClient::GetProjectInfo(const std::string& project_id,
                                   std::function<void(ProjectInfo)> onResult,
                                   error_func_t onError)
{
    m_impl->GetProjectInfo(project_id, onResult, onError);
}


// ----------------------------------------------------------------
// GUI Components
// ----------------------------------------------------------------


CrowdinLoginPanel::CrowdinLoginPanel(wxWindow *parent)
    : wxPanel(parent, wxID_ANY),
      m_state(State::SignedOut)
{
    wxBoxSizer *sizer = new wxBoxSizer(wxVERTICAL);
    sizer->SetMinSize(PX(400), -1);
    SetSizer(sizer);

    sizer->AddSpacer(PX(10));
    auto logo = new wxStaticBitmap(this, wxID_ANY, wxArtProvider::GetBitmap("CrowdinLogo"));
    logo->SetCursor(wxCURSOR_HAND);
    logo->Bind(wxEVT_LEFT_UP, [](wxMouseEvent&){ wxLaunchDefaultBrowser(WrapCrowdinLink("/")); });
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

    wxWeakRef<CrowdinLoginPanel> self(this);
    CrowdinClient::Get().GetUserInfo([self](CrowdinClient::UserInfo u){
        wxTheApp->CallAfter([self,u]{
            if (self)
            {
                self->m_userName = u.name;
                self->m_userLogin = u.login;
                self->ChangeState(State::SignedIn);
            }
        });
    });
}

void CrowdinLoginPanel::OnSignIn(wxCommandEvent&)
{
    ChangeState(State::Authenticating);

    wxWeakRef<CrowdinLoginPanel> self(this);
    CrowdinClient::Get().Authenticate([self]{
        wxTheApp->CallAfter([self]{
            if (self)
            {
                self->UpdateUserInfo();
                self->Raise();
            }
        });
    });
}

void CrowdinLoginPanel::OnSignOut(wxCommandEvent&)
{
    CrowdinClient::Get().SignOut();
    ChangeState(State::SignedOut);
}


LearnAboutCrowdinLink::LearnAboutCrowdinLink(wxWindow *parent, const wxString& text)
    : LearnMoreLink(parent,
                    WrapCrowdinLink("/"),
                    text.empty() ? (MSW_OR_OTHER(_("Learn more about Crowdin"), _("Learn More About Crowdin"))) : text)
{
}



namespace
{

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

        m_project->Bind(wxEVT_CHOICE, [=](wxCommandEvent&){ OnProjectSelected(); });
        ok->Bind(wxEVT_UPDATE_UI, &CrowdinOpenDialog::OnUpdateOK, this);

        ok->Disable();
        EnableAllChoices(false);

        FetchProjects();
    }

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
        e.Enable(m_project->GetSelection() > 0 &&
                 m_file->GetSelection() > 0 &&
                 m_language->GetSelection() > 0);
    }

private:
    wxChoice *m_project, *m_file, *m_language;
    ActivityIndicator *m_activity;

    std::vector<CrowdinClient::ProjectListing> m_projects;
    CrowdinClient::ProjectInfo m_info;
};

} // anonymous namespace


void CrowdinOpenFile(wxWindow *parent, std::function<void(wxString)> onLoaded)
{
    wxWindowPtr<CrowdinOpenDialog> dlg(new CrowdinOpenDialog(parent));
    dlg->CenterOnParent();

    dlg->ShowWindowModalThenDo([dlg,onLoaded](int retval) {
        dlg->Hide();

        // TODO: download the file
        // TODO: call onLoaded() on it
    });
}
