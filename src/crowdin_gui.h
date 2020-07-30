/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 2015-2020 Vaclav Slavik
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

#ifndef Poedit_crowdin_gui_h
#define Poedit_crowdin_gui_h

#include "catalog.h"

#ifdef HAVE_HTTP_CLIENT

#include "cloud_sync.h"
#include "customcontrols.h"

#include <wx/panel.h>

class WXDLLIMPEXP_FWD_CORE wxBoxSizer;
class WXDLLIMPEXP_FWD_CORE wxButton;


/**
    Panel used to sign in into Crowdin.
 */
class CrowdinLoginPanel : public wxPanel
{
public:
    enum Flags
    {
        DialogButtons = 1
    };

    CrowdinLoginPanel(wxWindow *parent, int flags = 0);

    /// Call this when the window is first shown
    void EnsureInitialized();

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
    void OnSignOut(wxCommandEvent&);
    virtual void OnUserSignedIn();

    State m_state;
    wxBoxSizer *m_loginInfo;
    wxButton *m_signIn, *m_signOut;
    wxString m_userName, m_userLogin;
    std::string m_userAvatar;
};


class CrowdinSyncDestination : public CloudSyncDestination
{
public:
    wxString GetName() const override { return "Crowdin"; }
    bool AuthIfNeeded(wxWindow* parent) override;
    dispatch::future<void> Upload(CatalogPtr file) override;
};


/// Link to learn about Crowdin
class LearnAboutCrowdinLink : public LearnMoreLink
{
public:
    LearnAboutCrowdinLink(wxWindow *parent, const wxString& text = "");
};


/// Can given file by synced to Crowdin, i.e. does it come from Crowdin and does it have required metadata?
bool CanSyncWithCrowdin(CatalogPtr cat);

/// Was the file opened directly from Crowdin and should be synced when the user saves it?
bool ShouldSyncToCrowdinAutomatically(CatalogPtr cat);

/**
    Let the user choose a Crowdin file, download it and open in Poedit.
    
    @param parent    PoeditFrame the UI should be shown under.
    @param onLoaded  Called with the name of loaded PO file.
 */
void CrowdinOpenFile(wxWindow *parent, std::function<void(wxString)> onLoaded);

/**
    Synces the catalog with Crowdin, uploading and downloading translations.

    @param parent    PoeditFrame the UI should be shown under.
    @param catalog   Catalog to sync.
    @param onDone    Called with the (new) updated catalog instance.
 */
void CrowdinSyncFile(wxWindow *parent, std::shared_ptr<Catalog> catalog,
                     std::function<void(std::shared_ptr<Catalog>)> onDone);

#else // !HAVE_HTTP_CLIENT

// convenience stubs to avoid additional checks all over other code:
inline bool CanSyncWithCrowdin(CatalogPtr) { return false; }
inline bool ShouldSyncToCrowdinAutomatically(CatalogPtr) { return false; }

#endif // !HAVE_HTTP_CLIENT

#endif // Poedit_crowdin_gui_h
