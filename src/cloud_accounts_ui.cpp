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

    // FIXME: introductory panel
    m_panelsBook->AddPage(new wxPanel(m_panelsBook, wxID_ANY, wxDefaultPosition, wxDefaultSize), "");

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
}


void AccountsPanel::AddAccount(const wxString& name, const wxString& iconId, AccountDetailPanel *panel)
{
    auto pos = (unsigned)m_panels.size();
    m_panels.push_back(panel);
    m_panelsBook->AddPage(panel, "");

    m_list->AppendFormattedItem(wxArtProvider::GetBitmap(iconId), name, " ... ");

    panel->OnContentChanged = [=]{
        wxString desc = panel->IsSignedIn() ? panel->GetLoginName() : _("(not signed in)");
        m_list->UpdateFormattedItem(pos, name, desc);
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
