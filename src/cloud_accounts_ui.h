/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 2015-2023 Vaclav Slavik
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

#include <functional>
#include <vector>

#include <wx/panel.h>

class WXDLLIMPEXP_FWD_CORE wxBoxSizer;
class WXDLLIMPEXP_FWD_CORE wxSimplebook;
class WXDLLIMPEXP_FWD_CORE wxDataViewEvent;

class IconAndSubtitleListCtrl;


/// Base class for account login views (Crowdin etc.)
class AccountDetailPanel : public wxPanel
{
public:
    AccountDetailPanel(wxWindow *parent) : wxPanel(parent, wxID_ANY) {}

    // Get service name for UI (e.g. "Crowdin") and other metadata:
    virtual wxString GetServiceName() const = 0;
    virtual wxString GetServiceLogo() const = 0;
    virtual wxString GetServiceDescription() const = 0;
    virtual wxString GetServiceLearnMoreURL() const = 0;

    /// Call this when the window is first shown
    virtual void EnsureInitialized() = 0;

    /// Notification function called when content (e.g. login name, state) changes
    std::function<void()> NotifyContentChanged;

    /// Notification function called when content should be made visible to user (e.g. while signing in, after signing in finished)
    std::function<void()> NotifyShouldBeRaised;

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

private:
    wxBoxSizer *m_sizer;
};


/// Window showing all supported accounts in a list-detail view
class AccountsPanel : public wxPanel
{
public:
    AccountsPanel(wxWindow *parent);

    /**
        Call to initalize logged-in accounts.

        This can be a little bit lengthy and may prompt the user for permission,
        so should be called lazily.
     */
    void InitializeAfterShown();

protected:
    void AddAccount(const wxString& name, const wxString& iconId, AccountDetailPanel *panel);
    void OnSelectAccount(wxDataViewEvent& event);

private:
    IconAndSubtitleListCtrl *m_list;
    wxSimplebook *m_panelsBook;
    ServiceSelectionPanel *m_introPanel;
    std::vector<AccountDetailPanel*> m_panels;
};

#endif // !HAVE_HTTP_CLIENT

#endif // Poedit_cloud_accounts_ui_h
