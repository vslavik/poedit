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

class WXDLLIMPEXP_FWD_CORE wxSimplebook;
class WXDLLIMPEXP_FWD_CORE wxDataViewEvent;

class IconAndSubtitleListCtrl;


/// Base class for account login views (Crowdin etc.)
class AccountDetailPanel : public wxPanel
{
public:
    AccountDetailPanel(wxWindow *parent) : wxPanel(parent, wxID_ANY) {}

    /// Call this when the window is first shown
    virtual void EnsureInitialized() = 0;

    /// Notification function called when content (e.g. login name, state) changes
    std::function<void()> OnContentChanged;

    virtual bool IsSignedIn() const = 0;
    virtual wxString GetLoginName() const = 0;
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
    std::vector<AccountDetailPanel*> m_panels;
};

#endif // !HAVE_HTTP_CLIENT

#endif // Poedit_cloud_accounts_ui_h
