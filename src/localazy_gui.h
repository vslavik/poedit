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

#ifndef Poedit_localazy_gui_h
#define Poedit_localazy_gui_h

#include "catalog.h"

#ifdef HAVE_HTTP_CLIENT

#include "cloud_accounts_ui.h"
#include "cloud_sync.h"
#include "customcontrols.h"

#include <wx/panel.h>

class WXDLLIMPEXP_FWD_CORE wxBoxSizer;
class WXDLLIMPEXP_FWD_CORE wxButton;
class WXDLLIMPEXP_FWD_CORE wxDataViewListCtrl;


/**
    Panel used to sign in into Localazy.
 */
class LocalazyLoginPanel : public AccountDetailPanel
{
public:
    enum Flags
    {
        DialogButtons = 1
    };

    LocalazyLoginPanel(wxWindow *parent, int flags = 0);

    void EnsureInitialized() override;
    bool IsSignedIn() const override { return m_state == State::SignedIn; }
    wxString GetLoginName() const override { return m_userLogin; }

protected:
    enum class State
    {
        Uninitialized,
        Authenticating,
        SignedIn,
        SignedOut,
        UpdatingInfo
    };

    void ChangeState(State state);
    void CreateLoginInfoControls(State state);
    void UpdateUserInfo();

    void OnSignIn(wxCommandEvent&);
    void OnAddProject(wxCommandEvent&);
    void OnSignOut(wxCommandEvent&);
    virtual void OnUserSignedIn();

    State m_state;
    ActivityIndicator *m_activity;
    wxBoxSizer *m_loginInfo;
    wxButton *m_signIn, *m_signOut;
    wxDataViewListCtrl *m_projects;
    wxString m_userName, m_userLogin;
    std::string m_userAvatar;
};


/// Link to learn about Localazy
class LearnAboutLocalazyLink : public LearnMoreLink
{
public:
    LearnAboutLocalazyLink(wxWindow *parent, const wxString& text = "");
};

#endif // HAVE_HTTP_CLIENT

#endif // Poedit_localazy_gui_h
