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

#ifndef Poedit_cloud_accounts_ui_h
#define Poedit_cloud_accounts_ui_h

#ifdef HAVE_HTTP_CLIENT

#include "catalog.h"
#include "cloud_accounts.h"
#include "hidpi.h"

#include <functional>
#include <vector>

#include <wx/dialog.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/panel.h>

class WXDLLIMPEXP_FWD_CORE wxBoxSizer;
class WXDLLIMPEXP_FWD_CORE wxSimplebook;
class WXDLLIMPEXP_FWD_CORE wxDataViewEvent;

class IconAndSubtitleListCtrl;


// Abstract base class with unified interface both for single-account panels and
// the one for picking from multiple accounts
class AnyAccountPanelBase
{
public:
    virtual ~AnyAccountPanelBase() {}

    /// Constructor flags
    enum Flags
    {
        /// Add wxID_CANCEL dialog button to the panel
        AddCancelButton = 1,
        SlimBorders = 2
    };

    /**
        Call to initalize logged-in accounts.

        This can be a little bit lengthy and may prompt the user for permission,
        so should be called lazily.
     */
    virtual void InitializeAfterShown() = 0;

    /// Notification function called when content (e.g. login name, state) changes
    std::function<void()> NotifyContentChanged;

    /// Notification function called when content should be made visible to user (e.g. while signing in, after signing in finished)
    std::function<void()> NotifyShouldBeRaised;
};


/// Base class for account login views (Crowdin etc.)
class AccountDetailPanel : public wxPanel, public AnyAccountPanelBase
{
public:
    // flags is unused, it is there to force derived classes to implement it
    AccountDetailPanel(wxWindow *parent, int /*flags*/) : wxPanel(parent, wxID_ANY) {}

    // Get service name for UI (e.g. "Crowdin") and other metadata:
    virtual wxString GetServiceName() const = 0;
    virtual wxString GetServiceLogo() const = 0;
    virtual wxString GetServiceDescription() const = 0;
    virtual wxString GetServiceLearnMoreURL() const = 0;

    virtual bool IsSignedIn() const = 0;
    virtual wxString GetLoginName() const = 0;

    /// Perform signing-in action, including any UI changes; directly corresponds to pressing "Sign in" button
    virtual void SignIn() = 0;
};


/// Panel for choosing a service if the user doesn't have any yet
class ServiceSelectionPanel : public wxPanel
{
public:
    ServiceSelectionPanel(wxWindow *parent);

    /// Add service information
    void AddService(AccountDetailPanel *account);

protected:
    wxSizer *CreateServiceContent(AccountDetailPanel *account);

private:
    wxBoxSizer *m_sizer;
};


/// Window showing all supported accounts in a list-detail view
class AccountsPanel : public wxPanel, public AnyAccountPanelBase
{
public:
    AccountsPanel(wxWindow *parent, int flags = 0);

    /**
        Call to initalize logged-in accounts.

        This can be a little bit lengthy and may prompt the user for permission,
        so should be called lazily.
     */
    void InitializeAfterShown() override;

    /// Is at least one account signed in?
    bool IsSignedIn() const;

protected:
    void AddAccount(const wxString& name, const wxString& iconId, AccountDetailPanel *panel);
    void OnSelectAccount(wxDataViewEvent& event);
    void SelectAccount(unsigned index);

private:
    IconAndSubtitleListCtrl *m_list;
    wxSimplebook *m_panelsBook;
    ServiceSelectionPanel *m_introPanel;
    std::vector<AccountDetailPanel*> m_panels;
};


/// See CloudLoginDialog, except this one doesn't close automatically
template<typename T>
class CloudEditLoginDialog : public wxDialog
{
public:
    typedef T LoginPanel;

    CloudEditLoginDialog(wxWindow *parent, const wxString& title) : wxDialog(parent, wxID_ANY, title)
    {
        auto topsizer = new wxBoxSizer(wxVERTICAL);

#ifdef __WXOSX__
        auto titleLabel = new wxStaticText(this, wxID_ANY, title);
        titleLabel->SetFont(titleLabel->GetFont().Bold());
        topsizer->AddSpacer(PX(4));
        topsizer->Add(titleLabel, wxSizerFlags().Border(wxTOP|wxLEFT|wxRIGHT, PX(16)));
        topsizer->AddSpacer(PX(10));
#else
        topsizer->AddSpacer(PX(16));
#endif

        m_panel = new LoginPanel(this, LoginPanel::AddCancelButton | LoginPanel::SlimBorders);
        m_panel->SetClientSize(m_panel->GetBestSize());
        topsizer->Add(m_panel, wxSizerFlags(1).Expand().Border(wxBOTTOM|wxLEFT|wxRIGHT, PX(16)));
        SetSizerAndFit(topsizer);
        CenterOnParent();

        m_panel->InitializeAfterShown();

        m_panel->NotifyShouldBeRaised = [=]{
            Raise();
        };
    }

protected:
    LoginPanel *m_panel;
};

/**
    A dialog for logging into cloud accounts.

    It can be used either for logging into any account (T=AccountsPanel, for initial setup)
    or just into a single provider (e.g. T=CrowdinLoginPanel) e.g. when syncing a file and
    credentials expired.

    Unlike CloudEditLoginDialog, closes automatically upon successful login.
 */
template<typename T>
class CloudLoginDialog : public CloudEditLoginDialog<T>
{
public:
    CloudLoginDialog(wxWindow *parent, const wxString& title) : CloudEditLoginDialog<T>(parent, title)
    {
        this->m_panel->NotifyContentChanged = [=]{
            if (this->m_panel->IsSignedIn())
            {
                this->Raise();
                this->EndModal(wxID_OK);
            }
        };
    }
};


/**
    Let the user choose a remote cloud file, download it and open in Poedit.

    @param parent    PoeditFrame the UI should be shown under.
    @param project   Optional project to preselect, otherwise nullptr
    @param onDone    Called with the dialog return value (wxID_OK/CANCEL) and name of loaded PO file.
 */
void CloudOpenFile(wxWindow *parent, std::shared_ptr<CloudAccountClient::ProjectInfo> project, std::function<void(int, wxString)> onDone);

/// Was the file opened directly from a cloud account and should be synced when the user saves it?
bool ShouldSyncToCloudAutomatically(CatalogPtr catalog);

/// Configure file, if it was opened directly from a cloud account, to be sync when the user saves is.
void SetupCloudSyncIfShouldBeDoneAutomatically(CatalogPtr catalog);


#endif // !HAVE_HTTP_CLIENT

#endif // Poedit_cloud_accounts_ui_h
