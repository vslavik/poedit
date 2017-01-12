/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 2014-2017 Vaclav Slavik
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

#include "main_toolbar.h"

#include "hidpi.h"
#include "utility.h"
#include "unicode_helpers.h"

#include <wx/intl.h>
#include <wx/settings.h>
#include <wx/toolbar.h>
#include <wx/xrc/xmlres.h>

#ifdef __WXMSW__
#include <wx/msw/uxtheme.h>
#endif


class WXMainToolbar : public MainToolbar
{
public:
    WXMainToolbar(wxFrame *parent)
    {
        m_tb = wxXmlResource::Get()->LoadToolBar(parent, "toolbar");
        m_idUpdate = XRCID("toolbar_update");

#ifdef __WXMSW__
        // De-uglify the toolbar a bit on Windows 10:
        if (IsWindows10OrGreater())
        {
            const wxUxThemeEngine* theme = wxUxThemeEngine::GetIfActive();
            if (theme)
            {
                wxUxThemeHandle hTheme(m_tb, L"ExplorerMenu::Toolbar");
                m_tb->SetBackgroundColour(wxRGBToColour(theme->GetThemeSysColor(hTheme, COLOR_WINDOW)));
            }

            unsigned padding = PX(4);
            ::SendMessage((HWND) m_tb->GetHWND(), TB_SETPADDING, 0, MAKELPARAM(padding, padding));
        }
        m_tb->SetDoubleBuffered(true);
#endif
    }

    void EnableSyncWithCrowdin(bool on) override
    {
        auto tool = m_tb->FindById(m_idUpdate);

        if (on)
        {
            tool->SetLabel(_("Sync"));
            tool->SetShortHelp(_("Synchronize the translation with Crowdin"));
            m_tb->SetToolNormalBitmap(m_idUpdate, wxArtProvider::GetBitmap("poedit-sync", wxART_TOOLBAR));
            #ifdef __WXMSW__
            m_tb->SetToolDisabledBitmap(m_idUpdate, wxArtProvider::GetBitmap("poedit-sync@disabled", wxART_TOOLBAR));
            #endif
        }
        else
        {
            tool->SetLabel(_("Update"));
            tool->SetShortHelp(_("Update catalog - synchronize it with sources"));
            m_tb->SetToolNormalBitmap(m_idUpdate, wxArtProvider::GetBitmap("poedit-update", wxART_TOOLBAR));
            #ifdef __WXMSW__
            m_tb->SetToolDisabledBitmap(m_idUpdate, wxArtProvider::GetBitmap("poedit-update@disabled", wxART_TOOLBAR));
            #endif
        }
    }

private:
    wxToolBar *m_tb;
    int m_idUpdate;
};


std::unique_ptr<MainToolbar> MainToolbar::CreateWX(wxFrame *parent)
{
    return std::unique_ptr<MainToolbar>(new WXMainToolbar(parent));
}


std::unique_ptr<MainToolbar> MainToolbar::Create(wxFrame *parent)
{
    return CreateWX(parent);
}
