/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 2013-2020 Vaclav Slavik
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
#include "customcontrols.h"
#include "crowdin_gui.h"
#include "edapp.h"
#include "edframe.h"
#include "hidpi.h"
#include "str_helpers.h"
#include "utility.h"

#include <wx/dcbuffer.h>
#include <wx/statbmp.h>
#include <wx/stattext.h>
#include <wx/sizer.h>
#include <wx/artprov.h>
#include <wx/font.h>
#include <wx/button.h>
#include <wx/settings.h>
#include <wx/hyperlink.h>
#include <wx/xrc/xmlres.h>

#ifdef __WXOSX__
    #include "StyleKit.h"
    #include <wx/nativewin.h>
    #if !wxCHECK_VERSION(3,1,0)
        #include "wx_backports/nativewin.h"
    #endif
#else
    #include <wx/commandlinkbutton.h>
#endif

#ifdef __WXOSX__

@interface POWelcomeButton : NSButton

@property wxWindow *parent;
@property NSString *heading;

@end

@implementation POWelcomeButton

- (id)initWithLabel:(NSString*)label heading:(NSString*)heading
{
    self = [super init];
    if (self)
    {
        self.title = label;
        self.heading = heading;
    }
    return self;
}

- (void)sizeToFit
{
    [super sizeToFit];
    NSSize size = self.frame.size;
    size.height = 64;
    [self setFrameSize:size];
}

- (void)drawRect:(NSRect)dirtyRect
{
    #pragma unused(dirtyRect)
    [StyleKit drawWelcomeButtonWithFrame:self.bounds
                                    icon:self.image
                                   label:self.heading
                             description:self.title
                              isDarkMode:(ColorScheme::GetWindowMode(self.parent) == ColorScheme::Dark)
                                 pressed:[self isHighlighted]];
}

- (void)controlAction:(id)sender
{
    #pragma unused(sender)
    wxCommandEvent event(wxEVT_BUTTON, _parent->GetId());
    event.SetEventObject(_parent);
    _parent->ProcessWindowEvent(event);
}

@end

#endif // __WXOSX__

namespace
{

#ifdef __WXOSX__

class ActionButton : public wxNativeWindow
{
public:
    ActionButton(wxWindow *parent, wxWindowID winid, const wxString& label, const wxString& note, const wxString& image = wxString())
    {
        SetMinSize(wxSize(510, -1));

        POWelcomeButton *view = [[POWelcomeButton alloc] initWithLabel:str::to_NS(note) heading:str::to_NS(label)];
        if (!image.empty())
            view.image = [NSImage imageNamed:str::to_NS(image)];
        view.parent = this;
        wxNativeWindow::Create(parent, winid, view);
    }
};

#elif defined(__WXGTK__)

class ActionButton : public wxButton
{
public:
    ActionButton(wxWindow *parent, wxWindowID winid, const wxString& label, const wxString& note)
        : wxButton(parent, winid, label, wxDefaultPosition, wxSize(500, 50), wxBU_LEFT)
    {
        SetLabelMarkup(wxString::Format("<b>%s</b>\n<small>%s</small>", label, note));
    }
};

#else

typedef wxCommandLinkButton ActionButton;

#endif


#ifdef __WXGTK__

// Workaround sizing bug of wxStaticText with custom font by using markup instead.
// See http://trac.wxwidgets.org/ticket/14374
class HeaderStaticText : public wxStaticText
{
public:
    HeaderStaticText(wxWindow *parent, wxWindowID id, const wxString& text)
        : wxStaticText(parent, id, "")
    {
        SetLabelMarkup("<span size='xx-large' weight='500'>" + text + "</span>");
    }

    virtual bool SetFont(const wxFont&) { return true; }
};

#else

typedef wxStaticText HeaderStaticText;

#endif

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

    auto guiface = wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT).GetFaceName();
#if defined(__WXOSX__)
    m_fntHeader = wxFont(wxFontInfo(30).FaceName(guiface));//.Light());
#else
    m_fntHeader = wxFont(wxFontInfo(22).FaceName(guiface).AntiAliased());
#endif

    // Translate all button events to wxEVT_MENU and send them to the frame.
    Bind(wxEVT_BUTTON, [=](wxCommandEvent& e){
        wxCommandEvent event(wxEVT_MENU, e.GetId());
        event.SetEventObject(this);
        GetParent()->GetEventHandler()->AddPendingEvent(event);
    });
}


WelcomeScreenPanel::WelcomeScreenPanel(wxWindow *parent)
    : WelcomeScreenBase(parent)
{
    auto sizer = new wxBoxSizer(wxVERTICAL);
    auto uberSizer = new wxBoxSizer(wxHORIZONTAL);
    uberSizer->AddStretchSpacer();
    uberSizer->Add(sizer, wxSizerFlags().Center().Border(wxALL, PX(50)));
    uberSizer->AddStretchSpacer();
    SetSizer(uberSizer);

    auto headerSizer = new wxBoxSizer(wxVERTICAL);

    auto hdr = new wxStaticBitmap(this, wxID_ANY, wxArtProvider::GetBitmap("PoeditWelcome"));
    headerSizer->Add(hdr, wxSizerFlags().Center());

    auto header = new HeaderStaticText(this, wxID_ANY, _("Welcome to Poedit"));
    header->SetFont(m_fntHeader);
    headerSizer->Add(header, wxSizerFlags().Center().Border(wxTOP, PX(10)));

    auto version = new wxStaticText(this, wxID_ANY, wxString::Format(_("Version %s"), wxGetApp().GetAppVersion()));
    headerSizer->Add(version, wxSizerFlags().Center());

    headerSizer->AddSpacer(PX(20));

    sizer->Add(headerSizer, wxSizerFlags().Expand());

    sizer->Add(new ActionButton(
                       this, wxID_OPEN,
                       MSW_OR_OTHER(_("Edit a translation"), _("Edit a Translation")),
                       _("Open an existing PO file and edit the translation.")),
               wxSizerFlags().PXBorderAll().Expand());

    sizer->Add(new ActionButton(
                       this, XRCID("menu_new_from_pot"),
                       MSW_OR_OTHER(_("Create new translation"), _("Create New Translation")),
                       _("Take an existing PO file or POT template and create a new translation from it.")),
               wxSizerFlags().PXBorderAll().Expand());

#ifdef HAVE_HTTP_CLIENT
    sizer->Add(new ActionButton(
                       this, XRCID("menu_open_crowdin"),
                       MSW_OR_OTHER(_("Collaborate on a translation with others"), _("Collaborate on a Translation with Others")),
                       _("Download a file from Crowdin project, translate and sync your changes back.")),
               wxSizerFlags().PXBorderAll().Expand());
    sizer->Add(new LearnAboutCrowdinLink(this, _("What is Crowdin?")), wxSizerFlags().Right().Border(wxRIGHT, PX(8)));
#endif // HAVE_HTTP_CLIENT

    sizer->AddSpacer(PX(50));

    ColorScheme::SetupWindowColors(this, [=]
    {
        header->SetForegroundColour(ColorScheme::Get(Color::Label));
        version->SetForegroundColour(ColorScheme::Get(Color::SecondaryLabel));
    });

    // Hide the cosmetic logo part if the screen is too small:
    auto minFullSize = sizer->GetMinSize().y + PX(50);
    Bind(wxEVT_SIZE, [=](wxSizeEvent& e){
        sizer->Show((size_t)0, e.GetSize().y >= minFullSize);
        e.Skip();
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
    header->SetFont(m_fntHeader);
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

        auto explain2 = new wxStaticText(this, wxID_ANY, _("The simplest way to fill this catalog is to update it from a POT:"));
        sizer->Add(explain2, wxSizerFlags().Expand().Border(wxTOP|wxBOTTOM, PX(10)));

        sizer->Add(new ActionButton(
                           this, XRCID("menu_update_from_pot"),
                           _("Update from POT"),
                           _("Take translatable strings from an existing POT template.")),
                   wxSizerFlags().Expand());
        sizer->AddSpacer(PX(20));

        auto explain3 = new wxStaticText(this, wxID_ANY, _("You can also extract translatable strings directly from the source code:"));
        sizer->Add(explain3, wxSizerFlags().Expand().Border(wxTOP|wxBOTTOM, PX(10)));

        auto btnSources = new ActionButton(
                           this, wxID_ANY,
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

        btnSources->Bind(wxEVT_BUTTON, [=](wxCommandEvent&){
            parent->EditCatalogPropertiesAndUpdateFromSources();
        });
    }

    Layout();
}
