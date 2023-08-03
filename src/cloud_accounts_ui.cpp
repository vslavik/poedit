/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 2023 Vaclav Slavik
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

#include "cloud_accounts_ui.h"

#ifdef HAVE_HTTP_CLIENT

#include "customcontrols.h"
#include "crowdin_gui.h"
#include "localazy_gui.h"
#include "utility.h"

#include <wx/simplebook.h>
#include <wx/sizer.h>
#include <wx/statline.h>


ServiceSelectionPanel::ServiceSelectionPanel(wxWindow *parent) : wxPanel(parent, wxID_ANY)
{
    auto topsizer = new wxBoxSizer(wxVERTICAL);
    SetSizer(topsizer);
    m_sizer = new wxBoxSizer(wxVERTICAL);
    topsizer->AddStretchSpacer();
    topsizer->Add(m_sizer, wxSizerFlags().Expand().Border(wxALL, PX(16)));
    topsizer->AddStretchSpacer();
}


void ServiceSelectionPanel::AddService(AccountDetailPanel *account)
{
    if (!GetChildren().empty())
        m_sizer->Add(new wxStaticLine(this, wxID_ANY), wxSizerFlags().Expand().Border(wxTOP|wxBOTTOM, PX(24)));

    auto logo = new StaticBitmap(this, account->GetServiceLogo());
    logo->SetCursor(wxCURSOR_HAND);
    logo->Bind(wxEVT_LEFT_UP, [=](wxMouseEvent&){ wxLaunchDefaultBrowser(account->GetServiceLearnMoreURL()); });
    m_sizer->Add(logo, wxSizerFlags().PXDoubleBorder(wxBOTTOM));
    auto explain = new ExplanationLabel(this, account->GetServiceDescription());
    m_sizer->Add(explain, wxSizerFlags().Expand());

    auto signIn = new wxButton(this, wxID_ANY, MSW_OR_OTHER(_("Add account"), _("Add Account")));
    signIn->Bind(wxEVT_BUTTON, [=](wxCommandEvent&){ account->SignIn(); });

    auto learnMore = new LearnMoreLink(this,
                                       account->GetServiceLearnMoreURL(),
                                       // TRANSLATORS: %s is online service name, e.g. "Crowdin" or "Localazy"
                                       wxString::Format(_("Learn more about %s"), account->GetServiceName()));

    auto buttons = new wxBoxSizer(wxHORIZONTAL);
    m_sizer->Add(buttons, wxSizerFlags().Expand().Border(wxTOP, PX(16)));
    buttons->Add(learnMore, wxSizerFlags().Center().Border(wxLEFT, PX(LearnMoreLink::EXTRA_INDENT)));
    buttons->AddStretchSpacer();
    buttons->Add(signIn, wxSizerFlags());
}


AccountsPanel::AccountsPanel(wxWindow *parent) : wxPanel(parent, wxID_ANY)
{
    wxSizer *sizer = new wxBoxSizer(wxHORIZONTAL);
    SetSizer(sizer);

    m_list = new IconAndSubtitleListCtrl(this, _("Account"), MSW_OR_OTHER(wxBORDER_SIMPLE, wxBORDER_SUNKEN));
    sizer->Add(m_list, wxSizerFlags().Expand().Border(wxRIGHT, PX(10)));

    m_panelsBook = new wxSimplebook(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL | MSW_OR_OTHER(wxBORDER_SIMPLE, wxBORDER_SUNKEN));
    ColorScheme::SetupWindowColors(m_panelsBook, [=]
    {
        if (ColorScheme::GetWindowMode(m_panelsBook) == ColorScheme::Light)
            m_panelsBook->SetBackgroundColour(*wxWHITE);
        else
            m_panelsBook->SetBackgroundColour(wxNullColour);
    });

    sizer->Add(m_panelsBook, wxSizerFlags(1).Expand());

    m_introPanel = new ServiceSelectionPanel(m_panelsBook);
    m_panelsBook->AddPage(m_introPanel, "");

    AddAccount("Crowdin", "AccountCrowdin", new CrowdinLoginPanel(m_panelsBook));
    AddAccount("Localazy", "AccountLocalazy", new LocalazyLoginPanel(m_panelsBook));

    m_list->SetMinSize(wxSize(PX(180), -1));
    m_panelsBook->SetMinSize(wxSize(PX(320), -1));

    m_list->Bind(wxEVT_DATAVIEW_SELECTION_CHANGED, &AccountsPanel::OnSelectAccount, this);
}


void AccountsPanel::InitializeAfterShown()
{
    for (auto& p: m_panels)
        p->EnsureInitialized();
    m_list->SetFocus();
}


void AccountsPanel::AddAccount(const wxString& name, const wxString& iconId, AccountDetailPanel *panel)
{
    auto pos = (unsigned)m_panels.size();
    m_panels.push_back(panel);
    m_panelsBook->AddPage(panel, "");

    m_introPanel->AddService(panel);

    m_list->AppendFormattedItem(wxArtProvider::GetBitmap(iconId), name, " ... ");

    panel->NotifyContentChanged = [=]{
        wxString desc = panel->IsSignedIn() ? panel->GetLoginName() : _("(not signed in)");
        m_list->UpdateFormattedItem(pos, name, desc);
        // select 1st available signed-in service if we can and hide the intro panel:
        if (m_list->GetSelectedRow() == wxNOT_FOUND && panel->IsSignedIn())
            m_list->SelectRow(pos);
    };
    panel->NotifyShouldBeRaised = [=]{
        m_list->SelectRow(pos);
    };
}


void AccountsPanel::OnSelectAccount(wxDataViewEvent& event)
{
    auto index = m_list->ItemToRow(event.GetItem());
    if (index == wxNOT_FOUND || index >= (int)m_panels.size())
        return;

    m_panelsBook->SetSelection(1 + index);
}


#endif // #ifdef HAVE_HTTP_CLIENT
