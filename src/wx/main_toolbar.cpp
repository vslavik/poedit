/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 2014-2025 Vaclav Slavik
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

#include <wx/bmpbuttn.h>
#include <wx/dcmemory.h>
#include <wx/stattext.h>
#include <wx/intl.h>
#include <wx/settings.h>
#include <wx/toolbar.h>
#include <wx/xrc/xmlres.h>
#include <wx/graphics.h>
#include <wx/scopedptr.h>

#ifdef __WXMSW__
#include <wx/msw/uxtheme.h>
#endif

#ifdef __WXGTK3__
#include <gtk/gtk.h>
#endif


class WXMainToolbar : public MainToolbar
{
public:
    WXMainToolbar(wxFrame *parent)
    {
        long style = wxTB_HORIZONTAL | wxTB_FLAT | wxTB_HORZ_TEXT | wxBORDER_NONE;
#ifdef __WXMSW__
        style |= wxTB_NODIVIDER;
#endif
        m_tb = new wxToolBar(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, style, "toolbar");
        m_tb->SetMargins(3, 3);

#ifdef __WXGTK3__
        GtkToolbar *gtb = Toolbar();
        gtk_toolbar_set_icon_size(gtb, GTK_ICON_SIZE_SMALL_TOOLBAR);
        gtk_style_context_add_class(gtk_widget_get_style_context(GTK_WIDGET(gtb)), GTK_STYLE_CLASS_PRIMARY_TOOLBAR);
#endif

        CreateTools();
        m_idUpdate = XRCID("toolbar_update");

        m_tb->Realize();
        parent->SetToolBar(m_tb);
     
#ifdef __WXMSW__
        // De-uglify the toolbar a bit on Windows 10:
        if (wxUxThemeIsActive())
        {
            wxUxThemeHandle hTheme(m_tb, L"ExplorerMenu::Toolbar");
            m_tb->SetBackgroundColour(wxRGBToColour(::GetThemeSysColor(hTheme, COLOR_WINDOW)));
        }

        unsigned padding = PX(10);
        ::SendMessage((HWND) m_tb->GetHWND(), TB_SETPADDING, 0, MAKELPARAM(padding, padding));
        
         m_tb->SetDoubleBuffered(true);
#endif
    }

    void EnableSyncWithCrowdin(bool on) override
    {
        auto tool = m_tb->FindById(m_idUpdate);

        if (on)
        {
            tool->SetLabel(_("Sync"));
            m_tb->SetToolShortHelp(m_idUpdate, _("Synchronize the translation with Crowdin"));
            #ifdef __WXGTK3__
            SetIcon(m_idUpdate, "sync");
            #else
            m_tb->SetToolNormalBitmap(m_idUpdate, wxArtProvider::GetBitmap("sync", wxART_TOOLBAR));
            #endif
            #ifdef __WXMSW__
            m_tb->SetToolDisabledBitmap(m_idUpdate, wxArtProvider::GetBitmap("sync@disabled", wxART_TOOLBAR));
            #endif
        }
        else
        {
            tool->SetLabel(MSW_OR_OTHER(_("Update from code"), _("Update from Code")));
            m_tb->SetToolShortHelp(m_idUpdate, _("Update from source code"));
            #ifdef __WXGTK3__
            SetIcon(m_idUpdate , "update");
            #else
            m_tb->SetToolNormalBitmap(m_idUpdate, wxArtProvider::GetBitmap("update", wxART_TOOLBAR));
            #endif
            #ifdef __WXMSW__
            m_tb->SetToolDisabledBitmap(m_idUpdate, wxArtProvider::GetBitmap("update@disabled", wxART_TOOLBAR));
            #endif
        }
    }

private:
    void CreateTools()
    {
        AddTool(wxID_OPEN, wxEmptyString, "document-open", _("Open file"));
        AddTool(wxID_SAVE, wxEmptyString, "document-save", _("Save file"));

        m_tb->AddSeparator();

        AddTool(XRCID("menu_validate"), _("Validate"), "validate", _("Check for errors in the translation"));

        AddTool(XRCID("menu_pretranslate"), _("Pre-translate"), "pretranslate", _("Pre-translate strings that don't have a translation yet"));
        AddTool(XRCID("toolbar_update"), MSW_OR_OTHER(_("Update from code"), _("Update from Code")), "update", _("Update from source code"));

        m_tb->AddStretchableSpace();

        AddTool(XRCID("show_sidebar"), wxEmptyString, "sidebar", _("Show or hide the sidebar"));
    }

    wxToolBarToolBase *AddTool(int id, const wxString& label, const wxString& icon, const wxString& shortHelp)
    {
        auto tool = m_tb->AddTool
        (
            id,
            label,
            wxArtProvider::GetBitmap(icon, wxART_TOOLBAR),
#ifdef __WXMSW__
            wxArtProvider::GetBitmap(icon + "@disabled", wxART_TOOLBAR),
#else
            wxNullBitmap,
#endif
            wxITEM_NORMAL,
            shortHelp
        );

#ifdef __WXGTK3__
        SetIcon(id, icon);
#endif

        return tool;
    }

#ifdef __WXGTK3__
    GtkToolbar *Toolbar()
    {
    #ifdef __WXGTK4__
        return GTK_TOOLBAR(m_tb->GetHandle());
    #else
        return GTK_TOOLBAR(gtk_bin_get_child(GTK_BIN(m_tb->GetHandle())));
    #endif
    }

    void SetIcon(int toolId, const wxString& name)
    {
        auto icon = name.StartsWith("document-")
                    ? wxString::Format("%s-symbolic", name)
                    : wxString::Format("poedit-%s-symbolic", name);

        GtkToolItem* i = gtk_toolbar_get_nth_item(Toolbar(), m_tb->GetToolPos(toolId));
        gtk_tool_button_set_icon_widget(GTK_TOOL_BUTTON(i), NULL);
        gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(i), icon.utf8_str());
    }
#endif

private:
    wxToolBar *m_tb;
    int m_idUpdate;
};


std::unique_ptr<MainToolbar> MainToolbar::Create(wxFrame *parent)
{
    return std::unique_ptr<MainToolbar>(new WXMainToolbar(parent));
}
