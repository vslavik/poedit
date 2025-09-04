/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 2023-2025 Vaclav Slavik
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


#include "localazy_gui.h"

#include "localazy_client.h"

#include "catalog.h"
#include "cloud_sync.h"
#include "colorscheme.h"
#include "concurrency.h"
#include "configuration.h"
#include "customcontrols.h"
#include "errors.h"
#include "hidpi.h"
#include "http_client.h"
#include "languagectrl.h"
#include "str_helpers.h"
#include "utility.h"
#include "catalog_xliff.h"

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

#include <boost/algorithm/string.hpp>



LocalazyLoginPanel::LocalazyLoginPanel(wxWindow *parent, int flags)
    : AccountDetailPanel(parent, flags),
      m_state(State::Uninitialized),
      m_activity(nullptr)
{
    wxBoxSizer *topsizer = new wxBoxSizer(wxVERTICAL);
    SetSizer(topsizer);

    wxBoxSizer *sizer = new wxBoxSizer(wxVERTICAL);
    sizer->SetMinSize(PX(350), PX(320));
    topsizer->Add(sizer, wxSizerFlags(1).Expand().Border(wxALL, (flags & SlimBorders) ? PX(0) : PX(16)));

    auto logo = new StaticBitmap(this, GetServiceLogo());
    logo->SetCursor(wxCURSOR_HAND);
    logo->Bind(wxEVT_LEFT_UP, [this](wxMouseEvent&){ wxLaunchDefaultBrowser(GetServiceLearnMoreURL()); });
    sizer->Add(logo, wxSizerFlags().PXDoubleBorder(wxBOTTOM));
    auto explain = new ExplanationLabel(this, GetServiceDescription());
    sizer->Add(explain, wxSizerFlags().Expand());

    m_loginInfo = new wxBoxSizer(wxHORIZONTAL);
    auto loginInfoContainer = new wxBoxSizer(wxVERTICAL);
    loginInfoContainer->SetMinSize(-1, PX(50));
    loginInfoContainer->AddStretchSpacer();
    loginInfoContainer->Add(m_loginInfo, wxSizerFlags().Expand());
    loginInfoContainer->AddStretchSpacer();

    sizer->Add(loginInfoContainer, wxSizerFlags().Expand().ReserveSpaceEvenIfHidden().Border(wxTOP|wxBOTTOM, PX(16)));

    m_projects = new wxDataViewListCtrl(this, wxID_ANY, wxDefaultPosition, wxSize(-1, PX(100)), /*wxDV_NO_HEADER |*/ MSW_OR_OTHER(wxBORDER_SIMPLE, wxBORDER_SUNKEN));
#ifdef __WXOSX__
    [((NSTableView*)[((NSScrollView*)m_projects->GetHandle()) documentView]) setIntercellSpacing:NSMakeSize(0.0, 0.0)];
    if (@available(macOS 11.0, *))
        ((NSTableView*)[((NSScrollView*)m_projects->GetHandle()) documentView]).style = NSTableViewStyleFullWidth;
#endif
    sizer->Add(m_projects, wxSizerFlags(1).Expand().Border(wxBOTTOM, PX(16)));
    m_projects->AppendIconTextColumn(_("Projects"));

    m_signIn = new wxButton(this, wxID_ANY, MSW_OR_OTHER(_("Sign in"), _("Sign In")));
    m_signIn->Bind(wxEVT_BUTTON, &LocalazyLoginPanel::OnSignIn, this);
    m_signOut= new wxButton(this, wxID_ANY, MSW_OR_OTHER(_("Sign out"), _("Sign Out")));
    m_signOut->Bind(wxEVT_BUTTON, &LocalazyLoginPanel::OnSignOut, this);
#ifdef __WXMSW__
    m_signIn->SetBackgroundColour(GetBackgroundColour());
    m_signOut->SetBackgroundColour(GetBackgroundColour());
#endif

    // TRANSLATORS: %s is online service name, e.g. "Crowdin" or "Localazy"
    auto learnMore = new LearnMoreLink(this, GetServiceLearnMoreURL(), wxString::Format(_("Learn more about %s"), "Localazy"));

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

wxString LocalazyLoginPanel::GetServiceDescription() const
{
    return _("Localazy is a highly automated localization platform allowing anyone to translate their products and content into multiple languages easily.");
}

wxString LocalazyLoginPanel::GetServiceLearnMoreURL() const
{
    return LocalazyClient::AttributeLink("/");
}

void LocalazyLoginPanel::InitializeAfterShown()
{
    if (m_state != State::Uninitialized)
        return;

    if (IsSignedIn())
        UpdateUserInfo();
    else
        ChangeState(State::SignedOut);
}

bool LocalazyLoginPanel::IsSignedIn() const
{
    return LocalazyClient::Get().IsSignedIn();
}

void LocalazyLoginPanel::ChangeState(State state)
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

void LocalazyLoginPanel::CreateLoginInfoControls(State state)
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
            sizer->Add(m_activity, wxSizerFlags(1).Center());
            // delay so that the window is sized properly:
            m_activity->CallAfter([=]{ m_activity->Start(text); });
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

            sizer->AddStretchSpacer();
            auto addPrj = new wxButton(this, wxID_ANY, MSW_OR_OTHER(_("Add project"), _("Add Project")));
#ifdef __WXOSX__
            addPrj->SetWindowVariant(wxWINDOW_VARIANT_SMALL);
#endif
#ifdef __WXMSW__
            addPrj->SetBackgroundColour(GetBackgroundColour());
#endif
            addPrj->Bind(wxEVT_BUTTON, &LocalazyLoginPanel::OnAddProject, this);
            sizer->Add(addPrj, wxSizerFlags().Center().Border(wxALL, PX(6)));

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

void LocalazyLoginPanel::UpdateUserInfo()
{
    ChangeState(State::UpdatingInfo);

    LocalazyClient::Get().GetUserInfo()
        .then_on_window(this, [=](LocalazyClient::UserInfo u) {
            m_userName = u.name;
            m_userLogin = u.login;
            m_userAvatar = u.avatarUrl;
            ChangeState(State::SignedIn);
        })
        .catch_all(m_activity->HandleError);

    LocalazyClient::Get().GetUserProjects()
        .then_on_window(m_projects, [=](std::vector<LocalazyClient::ProjectInfo> projects){
            m_projects->DeleteAllItems();
        #ifdef __WXOSX__
            auto dummyIcon = wxArtProvider::GetIcon("AccountLocalazy");
        #else
            auto dummyIcon = wxArtProvider::GetIcon("AccountLocalazy", wxART_OTHER, wxSize(PX(16), PX(16)));
            dummyIcon.SetScaleFactor(HiDPIScalingFactor());
        #endif

            unsigned idx = 0;
            for (auto p : projects)
            {
                wxVariant data(wxDataViewIconText(p.name, dummyIcon));
                wxVector<wxVariant> datavec;
                datavec.push_back(data);
                m_projects->AppendItem(datavec);

                if (!p.avatarUrl.empty())
                {
                    http_client::download_from_anywhere(p.avatarUrl)
                    .then_on_window(m_projects, [=](downloaded_file f)
                    {
                        wxBitmap bitmap;
                    #ifdef __WXOSX__
                        NSString *path = str::to_NS(f.filename().GetFullPath());
                        NSImage *img = [[NSImage alloc] initWithContentsOfFile:path];
                        if (img != nil)
                            bitmap = wxBitmap(img);
                    #else
                        wxLogNull null;
                        wxImage img(f.filename().GetFullPath());
                        if (img.IsOk())
                        {
                            img.Rescale(PX(16), PX(16));
                            bitmap = wxBitmap(img);
                            bitmap.SetScaleFactor(HiDPIScalingFactor());                            
                        }
                    #endif
                        if (bitmap.IsOk())
                        {
                            wxVariant value;
                            m_projects->GetValue(value, idx, 0);
                            wxDataViewIconText iconText;
                            iconText << value;
                            wxIcon icon;
                            icon.CopyFromBitmap(bitmap);
                            iconText.SetIcon(icon);
                            value << iconText;
                            m_projects->SetValue(value, idx, 0);
                        }
                    });
                }

                idx++;
            }
        })
        .catch_all(m_activity->HandleError);
}

void LocalazyLoginPanel::SignIn()
{
    ChangeState(State::Authenticating);
    LocalazyClient::Get().Authenticate()
        .then_on_window(this, &LocalazyLoginPanel::OnUserSignedIn);
    if (NotifyShouldBeRaised)
        NotifyShouldBeRaised();
}

void LocalazyLoginPanel::OnSignIn(wxCommandEvent&)
{
    SignIn();
}

void LocalazyLoginPanel::OnAddProject(wxCommandEvent&)
{
    // don't change UI state unlike with OnSignIn() -- FIXME: do indicate waiting in some way
    LocalazyClient::Get().Authenticate()
        .then_on_window(this, &LocalazyLoginPanel::OnUserSignedIn);
}

void LocalazyLoginPanel::OnUserSignedIn()
{
    UpdateUserInfo();
    Raise();
    if (NotifyShouldBeRaised)
        NotifyShouldBeRaised();
}

void LocalazyLoginPanel::OnSignOut(wxCommandEvent&)
{
    LocalazyClient::Get().SignOut();
    m_projects->DeleteAllItems();
    ChangeState(State::SignedOut);
}
