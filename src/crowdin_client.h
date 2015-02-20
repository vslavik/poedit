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

#ifndef Poedit_crowdin_client_h
#define Poedit_crowdin_client_h

#include <functional>
#include <memory>

#include <wx/panel.h>

class WXDLLIMPEXP_FWD_CORE wxBoxSizer;
class WXDLLIMPEXP_FWD_CORE wxButton;

/**
    Client to the Crowdin platform.
 */
class CrowdinClient
{
public:
    /// Return singleton instance of the client.
    static CrowdinClient& Get();

    /// Destroys the singleton, must be called (omly) on app shutdown.
    static void CleanUp();

    /// Is the user currently signed into Crowdin?
    bool IsSignedIn() const;

    /**
        Authenticate with Crowdin.
        
        This opens the browser to authenticate the app. The app must handle
        poedit:// URL and call HandleOAuthCallback. @a callback will be
        called after receiving the OAuth token.
     */
    void Authenticate(std::function<void()> callback);
    void HandleOAuthCallback(const std::string& uri);
    bool IsOAuthCallback(const std::string& uri);

    /// Sign out of Crowdin, forget the token
    void SignOut();

    /// Information about logged-in user
    struct UserInfo
    {
        std::wstring name;
        std::wstring login;
    };

    /// Retrieve information about the current user asynchronously
    void GetUserInfo(std::function<void(UserInfo)> callback);

private:
    CrowdinClient();
    ~CrowdinClient();

    class impl;
    std::unique_ptr<impl> m_impl;
    static CrowdinClient *ms_instance;
};


/**
    Panel used to sign in into Crowdin.
 */
class CrowdinLoginPanel : public wxPanel
{
public:
    CrowdinLoginPanel(wxWindow *parent);

protected:
    enum class State
    {
        Authenticating,
        SignedIn,
        SignedOut,
        UpdatingInfo
    };

    void ChangeState(State state);
    void CreateLoginInfoControls(State state);
    void UpdateUserInfo();

    void OnSignIn(wxCommandEvent&);
    void OnSignOut(wxCommandEvent&);

    State m_state;
    wxBoxSizer *m_loginInfo;
    wxButton *m_signIn, *m_signOut;
    wxString m_userName, m_userLogin;
};

#endif // Poedit_crowdin_client_h
