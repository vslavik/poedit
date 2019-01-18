/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 2014-2019 Vaclav Slavik
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

#ifdef __WXGTK__
#include <gtk/gtk.h>
#endif


class WXMainToolbar : public MainToolbar
{
public:
    WXMainToolbar(wxFrame *parent)
    {
        m_tb = wxXmlResource::Get()->LoadToolBar(parent, "toolbar");
        m_idUpdate = XRCID("toolbar_update");

#ifdef __WXGTK3__
        gtk_style_context_add_class(gtk_widget_get_style_context(GTK_WIDGET(Toolbar())), GTK_STYLE_CLASS_PRIMARY_TOOLBAR);
        SetIcon(0 , "document-open-symbolic");
        SetIcon(1 , "document-save-symbolic");
        SetIcon(3 , "poedit-validate-symbolic");
        SetIcon(4 , "poedit-update-symbolic");
        SetIcon(6 , "sidebar-symbolic");
#endif

#ifdef __WXMSW__
        // De-uglify the toolbar a bit on Windows 10:
        if (IsWindows10OrGreater())
        {
            if (wxUxThemeIsActive())
            {
                wxUxThemeHandle hTheme(m_tb, L"ExplorerMenu::Toolbar");
                m_tb->SetBackgroundColour(wxRGBToColour(::GetThemeSysColor(hTheme, COLOR_WINDOW)));
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
            #ifdef __WXGTK3__
            SetIcon(4 , "poedit-sync-symbolic");
            #else
            m_tb->SetToolNormalBitmap(m_idUpdate, wxArtProvider::GetBitmap("poedit-sync", wxART_TOOLBAR));
            #endif
            #ifdef __WXMSW__
            m_tb->SetToolDisabledBitmap(m_idUpdate, wxArtProvider::GetBitmap("poedit-sync@disabled", wxART_TOOLBAR));
            #endif
        }
        else
        {
            tool->SetLabel(MSW_OR_OTHER(_("Update from code"), _("Update from Code")));
            tool->SetShortHelp(_("Update catalog - synchronize it with sources"));
            #ifdef __WXGTK3__
            SetIcon(4 , "poedit-update-symbolic");
            #else
            m_tb->SetToolNormalBitmap(m_idUpdate, wxArtProvider::GetBitmap("poedit-update", wxART_TOOLBAR));
            #endif
            #ifdef __WXMSW__
            m_tb->SetToolDisabledBitmap(m_idUpdate, wxArtProvider::GetBitmap("poedit-update@disabled", wxART_TOOLBAR));
            #endif
        }
    }

#ifdef __WXGTK3__
private:
    GtkToolbar *Toolbar()
    {
    #ifdef __WXGTK4__
        return GTK_TOOLBAR(m_tb->GetHandle());
    #else
        return GTK_TOOLBAR(gtk_bin_get_child(GTK_BIN(m_tb->GetHandle())));
    #endif
    }

    void SetIcon(int index, const char *name)
    {
         GtkToolItem *i = gtk_toolbar_get_nth_item(Toolbar(), index);
         gtk_tool_button_set_icon_widget(GTK_TOOL_BUTTON(i), NULL);
         gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(i), name);
    } 
#endif

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
