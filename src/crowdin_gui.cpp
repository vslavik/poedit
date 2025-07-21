/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 2015-2025 Vaclav Slavik
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
#include "str_helpers.h"
#include "utility.h"
#include "catalog_xliff.h"

#include <wx/app.h>
#include <wx/artprov.h>
#include <wx/button.h>
#include <wx/choice.h>
#include <wx/config.h>
#include <wx/dataview.h>
#include <wx/dcclient.h>
#include <wx/dialog.h>
#include <wx/graphics.h>
#include <wx/msgdlg.h>
#include <wx/renderer.h>
#include <wx/sizer.h>
#include <wx/statbmp.h>
#include <wx/stdpaths.h>
#include <wx/weakref.h>
#include <wx/windowptr.h>

#include <regex>


namespace
{

class RecommendedLabel : public wxPanel
{
public:
    RecommendedLabel(wxWindow *parent) : wxPanel(parent, wxID_ANY)
    {
        auto label = new wxStaticText(this, wxID_ANY, _("Recommended"));
#ifdef __WXOSX__
        label->SetWindowVariant(wxWINDOW_VARIANT_SMALL);
#endif

        auto sizer = new wxBoxSizer(wxHORIZONTAL);
        sizer->AddSpacer(PX(4));
        sizer->Add(label, wxSizerFlags(1).Center().Border(wxALL, PX(2)));
        sizer->AddSpacer(PX(4));
        SetSizer(sizer);

        Bind(wxEVT_PAINT, [=](wxPaintEvent&)
        {
            wxPaintDC dc(this);
            std::unique_ptr<wxGraphicsContext> gc(wxGraphicsContext::Create(dc));
            gc->SetBrush(m_bg);
            gc->SetPen(*wxTRANSPARENT_PEN);

            auto rect = GetClientRect();
            if (!rect.IsEmpty())
            {
                gc->DrawRoundedRectangle(rect.x, rect.y, rect.width, rect.height, PX(2));
            }
        });

        ColorScheme::SetupWindowColors(this, [=]
        {
            auto fg = ColorScheme::GetBlendedOn(Color::TagWarningLineFg, this, Color::TagWarningLineBg);
            m_bg = ColorScheme::GetBlendedOn(Color::TagWarningLineBg, this);
            label->SetForegroundColour(fg);
#ifdef __WXMSW__
            for (auto c : GetChildren())
                c->SetBackgroundColour(m_bg);
#endif
        });

        m_container.DisableSelfFocus();
    }

    bool AcceptsFocus() const override { return false; }

private:
    wxColour m_bg;
};

} // anonymous namespace


CrowdinLoginPanel::CrowdinLoginPanel(wxWindow *parent, int flags)
    : AccountDetailPanel(parent, flags),
      m_state(State::Uninitialized),
      m_activity(nullptr)
{
    wxBoxSizer *topsizer = new wxBoxSizer(wxVERTICAL);
    SetSizer(topsizer);

    wxBoxSizer *sizer = new wxBoxSizer(wxVERTICAL);
    sizer->SetMinSize(PX(300), -1);
    topsizer->Add(sizer, wxSizerFlags(1).Expand().Border(wxALL, (flags & SlimBorders) ? PX(0) : PX(16)));

    auto logo = new StaticBitmap(this, GetServiceLogo());
    logo->SetCursor(wxCURSOR_HAND);
    logo->Bind(wxEVT_LEFT_UP, [this](wxMouseEvent&){ wxLaunchDefaultBrowser(GetServiceLearnMoreURL()); });

    auto logosizer = new wxBoxSizer(wxHORIZONTAL);
    logosizer->Add(logo, wxSizerFlags().Center());
    logosizer->AddStretchSpacer();
    logosizer->Add(new RecommendedLabel(this), wxSizerFlags().Center());
    sizer->Add(logosizer, wxSizerFlags().Expand().PXDoubleBorder(wxBOTTOM));

    auto explain = new ExplanationLabel(this, GetServiceDescription());
    sizer->Add(explain, wxSizerFlags().Expand());

    m_loginInfo = new wxBoxSizer(wxHORIZONTAL);
    auto loginInfoContainer = new wxBoxSizer(wxVERTICAL);
    loginInfoContainer->SetMinSize(-1, PX(50));
    loginInfoContainer->AddStretchSpacer();
    loginInfoContainer->Add(m_loginInfo, wxSizerFlags().Center());
    loginInfoContainer->AddStretchSpacer();

    sizer->Add(loginInfoContainer, wxSizerFlags().Expand().ReserveSpaceEvenIfHidden().Border(wxTOP|wxBOTTOM, PX(16)));
    sizer->AddStretchSpacer();

    m_signIn = new wxButton(this, wxID_ANY, MSW_OR_OTHER(_("Sign in"), _("Sign In")));
    m_signIn->Bind(wxEVT_BUTTON, &CrowdinLoginPanel::OnSignIn, this);
    m_signOut= new wxButton(this, wxID_ANY, MSW_OR_OTHER(_("Sign out"), _("Sign Out")));
    m_signOut->Bind(wxEVT_BUTTON, &CrowdinLoginPanel::OnSignOut, this);
#ifdef __WXMSW__
    m_signIn->SetBackgroundColour(GetBackgroundColour());
    m_signOut->SetBackgroundColour(GetBackgroundColour());
#endif

    auto learnMore = new LearnMoreLink(this, GetServiceLearnMoreURL(), _("Learn more about Crowdin"));

    auto buttons = new wxBoxSizer(wxHORIZONTAL);
    sizer->Add(buttons, wxSizerFlags().Expand().Border(wxBOTTOM, 1));
    buttons->Add(learnMore, wxSizerFlags().Center());
    buttons->AddSpacer(PX(60));
    buttons->AddStretchSpacer();
    buttons->Add(m_signIn, wxSizerFlags());
    buttons->Add(m_signOut, wxSizerFlags());

    if (flags & AddCancelButton)
    {
        auto cancel = new wxButton(this, wxID_CANCEL);
#ifdef __WXMSW__
        buttons->Add(cancel, wxSizerFlags().Border(wxLEFT, PX(3)));
#else
        buttons->Insert(3, cancel, wxSizerFlags().Border(wxRIGHT, PX(6)));
#endif
        m_signIn->SetDefault();
        m_signIn->SetFocus();
    }

    ChangeState(State::Uninitialized);
}

wxString CrowdinLoginPanel::GetServiceDescription() const
{
    return _("Crowdin is an online translation management platform and collaborative translation tool. We use Crowdin ourselves to translate Poedit into many languages, and we love it.");
}

wxString CrowdinLoginPanel::GetServiceLearnMoreURL() const
{
    return CrowdinClient::AttributeLink("/");
}

void CrowdinLoginPanel::InitializeAfterShown()
{
    if (m_state != State::Uninitialized)
        return;

    if (IsSignedIn())
        UpdateUserInfo();
    else
        ChangeState(State::SignedOut);
}

bool CrowdinLoginPanel::IsSignedIn() const
{
    return CrowdinClient::Get().IsSignedIn();
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

    switch (state)
    {
        case State::SignedIn:
        case State::SignedOut:
            if (NotifyContentChanged)
                NotifyContentChanged();
            break;

        case State::Authenticating:
        case State::UpdatingInfo:
        case State::Uninitialized:
            // not relevant for UI changes
            break;
    }
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
            m_activity = new ActivityIndicator(this, ActivityIndicator::Centered);
            sizer->Add(m_activity, wxSizerFlags().Expand());
            m_activity->Start(text);
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
            m_userAvatar = u.avatarUrl;
            ChangeState(State::SignedIn);
        })
        .catch_all(m_activity->HandleError);
}

void CrowdinLoginPanel::SignIn()
{
    ChangeState(State::Authenticating);
    CrowdinClient::Get().Authenticate()
        .then_on_window(this, &CrowdinLoginPanel::OnUserSignedIn);
    if (NotifyShouldBeRaised)
        NotifyShouldBeRaised();
}

void CrowdinLoginPanel::OnSignIn(wxCommandEvent&)
{
    SignIn();
}

void CrowdinLoginPanel::OnUserSignedIn()
{
    UpdateUserInfo();
    Raise();
    if (NotifyShouldBeRaised)
        NotifyShouldBeRaised();
}

void CrowdinLoginPanel::OnSignOut(wxCommandEvent&)
{
    CrowdinClient::Get().SignOut();
    ChangeState(State::SignedOut);
}


namespace
{

typedef CloudLoginDialog<CrowdinLoginPanel> CrowdinLoginDialog;

} // anonymous namespace


bool CanSyncWithCrowdin(CatalogPtr cat)
{
    return (bool)CrowdinClient::DoExtractSyncMetadata(*cat);
}


void CrowdinSyncFile(wxWindow *parent, std::shared_ptr<Catalog> catalog,
                     std::function<void(std::shared_ptr<Catalog>)> onDone)
{
    if (!CrowdinClient::Get().IsSignedIn())
    {
        wxWindowPtr<CrowdinLoginDialog> login(new CrowdinLoginDialog(parent, _("Sign in to Crowdin")));
        login->ShowWindowModalThenDo([login,parent,catalog,onDone](int retval){
            if (retval == wxID_OK)
                CrowdinSyncFile(parent, catalog, onDone);
        });
        return;
    }

    wxLogTrace("poedit.crowdin", "Crowdin syncing file ...");

    wxWindowPtr<CloudSyncProgressWindow> dlg(new CloudSyncProgressWindow(parent));

    auto meta = CrowdinClient::Get().ExtractSyncMetadata(*catalog);

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
        CrowdinClient::Get().UploadFile(catalog->SaveToBuffer(), meta)
        .then([=]
        {
            auto tmpdir = std::make_shared<TempDirectory>();
            auto outfile = tmpdir->CreateFileName("crowdin." + wxFileName(catalog->GetFileName()).GetExt());

            dispatch::on_main([=]{
                dlg->Activity->Start(_(L"Downloading latest translations…"));
            });

            return CrowdinClient::Get().DownloadFile(outfile.ToStdWstring(), meta)
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

    dlg->ShowWindowModal();
}
