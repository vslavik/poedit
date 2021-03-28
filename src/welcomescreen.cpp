/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 2013-2021 Vaclav Slavik
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

#include "welcomescreen.h"

#include "colorscheme.h"
#include "custom_buttons.h"
#include "customcontrols.h"
#include "crowdin_gui.h"
#include "edapp.h"
#include "edframe.h"
#include "hidpi.h"
#include "menus.h"
#include "recent_files.h"
#include "str_helpers.h"
#include "utility.h"

#include <wx/config.h>
#include <wx/dcbuffer.h>
#include <wx/statbmp.h>
#include <wx/stattext.h>
#include <wx/sizer.h>
#include <wx/artprov.h>
#include <wx/font.h>
#include <wx/button.h>
#include <wx/bmpbuttn.h>
#include <wx/settings.h>
#include <wx/hyperlink.h>
#include <wx/xrc/xmlres.h>

namespace
{

class HeaderStaticText : public wxStaticText
{
public:
    HeaderStaticText(wxWindow *parent, wxWindowID id, const wxString& text) : wxStaticText(parent, id, "")
    {
#ifdef __WXGTK__
        // Workaround sizing bug of wxStaticText with custom font by using markup instead.
        // See http://trac.wxwidgets.org/ticket/14374
        SetLabelMarkup("<span size='xx-large' weight='500'>" + text + "</span>");
#else
        SetLabel(text);
    #ifdef __WXOSX__
        if (@available(macOS 10.11, *))
        {
            SetFont([NSFont systemFontOfSize:30 weight:NSFontWeightRegular]);
        }
        else
    #endif
        {
            auto guiface = wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT).GetFaceName();
            wxFont f(wxFontInfo(22).FaceName(guiface).AntiAliased());
            SetFont(f);
        }
#endif
    }
};

#ifdef __WXMSW__

class SidebarHeader : public wxWindow
{
public:
    SidebarHeader(wxWindow* parent, const wxString& title) : wxWindow(parent, wxID_ANY)
    {
        ColorScheme::SetupWindowColors(this, [=]
        {
            SetBackgroundColour(ColorScheme::Get(Color::SidebarBackground));
        });

        auto label = new SecondaryLabel(this, title);
        auto sizer = new wxBoxSizer(wxVERTICAL);
        sizer->AddStretchSpacer();
        sizer->Add(label, wxSizerFlags().Left().Border(wxLEFT, PX(10)));
        //sizer->AddSpacer(PX(4));
        sizer->AddStretchSpacer();
        SetSizer(sizer);
    }
};

#endif // __WXMSW__

} // anonymous namespace


WelcomeScreenBase::WelcomeScreenBase(wxWindow *parent) : wxPanel(parent, wxID_ANY)
{
    ColorScheme::SetupWindowColors(this, [=]
    {
        switch (ColorScheme::GetWindowMode(this))
        {
            case ColorScheme::Light:
                SetBackgroundColour("#fdfdfd");
                break;
            case ColorScheme::Dark:
                SetBackgroundColour(ColorScheme::Get(Color::SidebarBackground));
                break;
        }
    });

    // Translate all button events to wxEVT_MENU and send them to the frame.
    Bind(wxEVT_BUTTON, [=](wxCommandEvent& e){
        wxCommandEvent event(wxEVT_MENU, e.GetId());
        event.SetEventObject(this);
        GetParent()->GetEventHandler()->AddPendingEvent(event);
    });
}


EmptyPOScreenPanel::EmptyPOScreenPanel(PoeditFrame *parent, bool isGettext)
    : WelcomeScreenBase(parent)
{
    auto sizer = new wxBoxSizer(wxVERTICAL);
    auto uberSizer = new wxBoxSizer(wxHORIZONTAL);
    uberSizer->AddStretchSpacer();
    uberSizer->Add(sizer, wxSizerFlags().Center().Border(wxALL, PX(100)));
    uberSizer->AddStretchSpacer();
    SetSizer(uberSizer);

    auto header = new HeaderStaticText(this, wxID_ANY, _(L"There are no translations. That’s unusual."));
    ColorScheme::SetupWindowColors(this, [=]
    {
        header->SetForegroundColour(ColorScheme::Get(Color::Label));
    });
    sizer->Add(header, wxSizerFlags().Center().PXBorderAll());

    if (isGettext)
    {
        auto explain = new AutoWrappingText(this, _(L"Translatable entries aren’t added manually in the Gettext system, but are automatically extracted\nfrom source code. This way, they stay up to date and accurate.\nTranslators typically use PO template files (POTs) prepared for them by the developer."));
        sizer->Add(explain, wxSizerFlags().Expand().Border(wxTOP, PX(10)));

        auto learnMore = new LearnMoreLink(this, "http://www.gnu.org/software/gettext/manual/html_node/", _("(Learn more about GNU gettext)"));
        sizer->Add(learnMore, wxSizerFlags().Border(wxBOTTOM, PX(15)).Align(wxALIGN_RIGHT));

        auto explain2 = new wxStaticText(this, wxID_ANY, _("The simplest way to fill this file with translations is to update it from a POT:"));
        sizer->Add(explain2, wxSizerFlags().Expand().Border(wxTOP|wxBOTTOM, PX(10)));

        sizer->Add(new ActionButton(
                           this, XRCID("menu_update_from_pot"), "UpdateFromPOT",
                           _("Update from POT"),
                           _("Take translatable strings from an existing POT template.")),
                   wxSizerFlags().Expand());
        sizer->AddSpacer(PX(20));

        auto explain3 = new wxStaticText(this, wxID_ANY, _("You can also extract translatable strings directly from the source code:"));
        sizer->Add(explain3, wxSizerFlags().Expand().Border(wxTOP|wxBOTTOM, PX(10)));

        auto btnSources = new ActionButton(
                           this, wxID_ANY, "ExtractFromSources",
                           _("Extract from sources"),
                           _("Configure source code extraction in Properties."));
        sizer->Add(btnSources, wxSizerFlags().Expand());
        sizer->AddSpacer(PX(20));

        ColorScheme::SetupWindowColors(this, [=]
        {
            explain->SetForegroundColour(ColorScheme::Get(Color::SecondaryLabel));
            explain2->SetForegroundColour(ColorScheme::Get(Color::SecondaryLabel));
            explain3->SetForegroundColour(ColorScheme::Get(Color::SecondaryLabel));
        });

        btnSources->Bind(wxEVT_MENU, [=](wxCommandEvent&){
            parent->EditCatalogPropertiesAndUpdateFromSources();
        });
    }

    Layout();
}



WelcomeWindow *WelcomeWindow::ms_instance = nullptr;

WelcomeWindow *WelcomeWindow::GetAndActivate()
{
    if (!ms_instance)
        ms_instance = new WelcomeWindow();
    ms_instance->Show();
    if (ms_instance->IsIconized())
        ms_instance->Iconize(false);
    ms_instance->Raise();
    return ms_instance;
}

bool WelcomeWindow::HideActive()
{
    bool retval = ms_instance && ms_instance->IsShown();
    if (ms_instance)
        ms_instance->Hide();
    return retval;
}


WelcomeWindow::~WelcomeWindow()
{
    ms_instance = nullptr;
}

WelcomeWindow::WelcomeWindow()
    : WelcomeWindowBase(nullptr, wxID_ANY, _("Welcome to Poedit"),
                        wxDefaultPosition, wxDefaultSize,
                        wxSYSTEM_MENU | wxCLOSE_BOX | wxCAPTION | wxCLIP_CHILDREN)
{
    ColorScheme::SetupWindowColors(this, [=]
    {
        if (ColorScheme::GetWindowMode(this) == ColorScheme::Light)
            SetBackgroundColour(*wxWHITE);
        else
            SetBackgroundColour(GetDefaultAttributes().colBg);
    });

#ifdef __WXOSX__
    NSWindow *wnd = (NSWindow*)GetWXWindow();
    wnd.excludedFromWindowsMenu = YES;
#endif

#ifdef __WXMSW__
    SetIcons(wxIconBundle(wxStandardPaths::Get().GetResourcesDir() + "\\Resources\\Poedit.ico"));
#endif

#ifndef __WXOSX__
    SetMenuBar(wxGetApp().CreateMenu(Menu::WelcomeWindow));
#endif

    auto topsizer = new wxBoxSizer(wxHORIZONTAL);
    auto leftoutersizer = new wxBoxSizer(wxVERTICAL);
    auto leftsizer = new wxBoxSizer(wxVERTICAL);

#ifdef __WXMSW__
    if (GetMenuWindow())
    {
        leftoutersizer->Add(GetMenuWindow(), wxSizerFlags().Left());
    }
#endif

#if defined(__WXMSW__)
    wxIcon logo;
    if (HiDPIScalingFactor() == 1.0)
    {
        logo.LoadFile("appicon", wxBITMAP_TYPE_ICO_RESOURCE, 128, 128);
    }
    else
    {
        logo.LoadFile("appicon", wxBITMAP_TYPE_ICO_RESOURCE, 256, 256);
        if (HiDPIScalingFactor() != 2.0)
        {
            wxBitmap bmp;
            bmp.CopyFromIcon(logo);
            logo.CopyFromBitmap(wxBitmap(bmp.ConvertToImage().Scale(PX(128), PX(128), wxIMAGE_QUALITY_BICUBIC)));
        }
    }
#elif defined(__WXGTK__)
    auto logo = wxArtProvider::GetIcon("net.poedit.Poedit", wxART_FRAME_ICON, wxSize(128, 128));
#else
    auto logo = wxArtProvider::GetBitmap("Poedit");
#endif
    auto logoWindow = new wxStaticBitmap(this, wxID_ANY, logo, wxDefaultPosition, wxSize(PX(128),PX(128)));
    leftsizer->Add(logoWindow, wxSizerFlags().Center().Border(wxALL, PX(5)));

    auto header = new HeaderStaticText(this, wxID_ANY, _("Welcome to Poedit"));
    leftsizer->Add(header, wxSizerFlags().Center());

    auto version = new wxStaticText(this, wxID_ANY, wxString::Format(_("Version %s"), wxGetApp().GetAppVersion()));
    leftsizer->Add(version, wxSizerFlags().Center().Border(wxTOP, PX(5)));

    leftsizer->AddSpacer(PX(30));

    leftsizer->Add(new ActionButton(
                       this, XRCID("menu_new_from_pot"), "CreateTranslation",
                       _(L"Create new…"),
                       _("Create new translation from POT template.")),
               wxSizerFlags().Border(wxTOP, PX(2)).Expand());

    leftsizer->Add(new ActionButton(
                       this, wxID_OPEN, "EditTranslation",
                       _("Browse files"),
                       _("Open and edit translation files.")),
               wxSizerFlags().Border(wxTOP, PX(2)).Expand());

#ifdef HAVE_HTTP_CLIENT
    leftsizer->Add(new ActionButton(
                       this, XRCID("menu_open_crowdin"), "Collaborate",
                       _("Translate Crowdin project"),
                       _("Collaborate with others in a Crowdin project.")),
               wxSizerFlags().Border(wxTOP|wxBOTTOM, PX(2)).Expand());
#endif // HAVE_HTTP_CLIENT

    leftoutersizer->Add(leftsizer, wxSizerFlags().Center().Border(wxALL, PX(50)));
    topsizer->Add(leftoutersizer, wxSizerFlags(1).Expand());

#ifndef __WXGTK__
    auto rightsizer = new wxBoxSizer(wxVERTICAL);
    topsizer->Add(rightsizer, wxSizerFlags().Expand());

#ifdef __WXMSW__
    // wx doesn't like close button overlapping the recents list (or any overlapping at all),
    // so add some space at the top of the list to improve the situation
    auto closeButton = GetCloseButton();
    if (closeButton)
    {
        auto label = new SidebarHeader(this, _("Recent files"));
        label->SetMinSize(wxSize(-1, closeButton->GetSize().y));
        rightsizer->Add(label, wxSizerFlags().Expand().Border(wxRIGHT, closeButton->GetSize().x));
    }
#endif // __WXMSW__

    auto recentFiles = new RecentFilesCtrl(this);
    recentFiles->SetMinSize(wxSize(PX(320), -1));
    rightsizer->Add(recentFiles, wxSizerFlags(1).Expand());
#endif

    SetSizerAndFit(topsizer);

    ColorScheme::SetupWindowColors(this, [=]
    {
        header->SetForegroundColour(ColorScheme::Get(Color::Label));
        version->SetForegroundColour(ColorScheme::Get(Color::SecondaryLabel));

#ifdef __WXMSW__
        if (GetCloseButton())
            GetCloseButton()->SetBackgroundColour(ColorScheme::Get(Color::SidebarBackground));
        for (auto& w : GetChildren())
        {
            if (dynamic_cast<ActionButton*>(w))
                w->SetBackgroundColour(GetBackgroundColour());
        }
#endif
    });
}
