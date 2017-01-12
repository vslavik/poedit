/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 2013-2017 Vaclav Slavik
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

#ifndef __WXOSX__
#include <wx/commandlinkbutton.h>
#endif

namespace
{

#ifdef __WXOSX__

class ActionButton : public wxWindow
{
public:
    ActionButton(wxWindow *parent, wxWindowID winid, const wxString& label, const wxString& note)
        : wxWindow(parent, wxID_ANY, wxDefaultPosition, wxSize(500, 65))
    {
        m_button = new wxButton(this, winid, "");
        auto sizer = new wxBoxSizer(wxHORIZONTAL);
        sizer->Add(m_button, wxSizerFlags(1).Expand());
        SetSizer(sizer);

        NSButton *btn = (NSButton*)m_button->GetHandle();

        NSString *str = str::to_NS(label + "\n" + note);
        NSRange topLine = NSMakeRange(0, label.length() + 1);
        NSRange bottomLine = NSMakeRange(label.length() + 1, note.length());

        NSMutableAttributedString *s = [[NSMutableAttributedString alloc] initWithString:str];
        [s addAttribute:NSFontAttributeName
                  value:[NSFont systemFontOfSize:[NSFont systemFontSize]+1]
                  range:topLine];
        [s addAttribute:NSFontAttributeName
                  value:[NSFont systemFontOfSize:[NSFont systemFontSizeForControlSize:NSSmallControlSize]]
                  range:bottomLine];
        [s addAttribute:NSForegroundColorAttributeName
                  value:[NSColor grayColor]
                  range:bottomLine];

        NSMutableParagraphStyle *paragraphStyle = [[NSMutableParagraphStyle alloc] init];
        paragraphStyle.headIndent = 10.0;
        paragraphStyle.firstLineHeadIndent = 10.0;
        paragraphStyle.tailIndent = -10.0;
        paragraphStyle.lineSpacing = 2.0;
        [s addAttribute:NSParagraphStyleAttributeName
                  value:paragraphStyle
                  range:NSMakeRange(0, [s length])];

        [btn setAttributedTitle:s];
        [btn setShowsBorderOnlyWhileMouseInside:YES];
        [btn setBezelStyle:NSSmallSquareBezelStyle];
        [btn setButtonType:NSMomentaryPushInButton];

        SetBackgroundColour(wxColour("#F2FCE2"));
        Bind(wxEVT_PAINT, &ActionButton::OnPaint, this);
    }

private:
    void OnPaint(wxPaintEvent&)
    {
        wxPaintDC dc(this);
        wxRect rect(dc.GetSize());
        auto bg = GetBackgroundColour();
        dc.SetBrush(bg);
        dc.SetPen(bg.ChangeLightness(90));
        dc.DrawRoundedRectangle(rect, 2);
        dc.DrawRectangle(rect.x+1, rect.y+rect.height-2, rect.width-2, 2);
    }

    wxButton *m_button;
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
        SetLabelMarkup("<big><b>" + text + "</b></big>");
    }

    virtual bool SetFont(const wxFont&) { return true; }
};

#else

typedef wxStaticText HeaderStaticText;

#endif

} // anonymous namespace


WelcomeScreenBase::WelcomeScreenBase(wxWindow *parent)
    : wxPanel(parent, wxID_ANY),
      m_clrHeader("#555555"),
      m_clrNorm("#444444"),
      m_clrSub("#aaaaaa")
{
    SetBackgroundColour(wxColour("#fffcf5"));

#if defined(__WXOSX__)
    auto guiface = wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT).GetFaceName();
    m_fntHeader = wxFont(wxFontInfo(30).FaceName(guiface).Light());
    m_fntNorm = wxFont(wxFontInfo(13).FaceName(guiface).Light());
    m_fntSub = wxFont(wxFontInfo(11).FaceName(guiface).Light());
#elif defined(__WXMSW__)
    #define HEADER_FACE "Segoe UI"
    m_fntHeader = wxFont(wxFontInfo(20).FaceName("Segoe UI Light").AntiAliased());
    m_fntNorm = wxFont(wxFontInfo(10).FaceName(HEADER_FACE));
    m_fntSub = wxFont(wxFontInfo(9).FaceName(HEADER_FACE));
#else
    #define HEADER_FACE "sans serif"
    m_fntHeader = wxFont(wxFontInfo(16).FaceName(HEADER_FACE).Light());
    m_fntNorm = wxFont(wxFontInfo(10).FaceName(HEADER_FACE).Light());
    m_fntSub = wxFont(wxFontInfo(9).FaceName(HEADER_FACE).Light());
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
    header->SetForegroundColour(m_clrHeader);
    headerSizer->Add(header, wxSizerFlags().Center().Border(wxTOP, PX(10)));

    auto version = new wxStaticText(this, wxID_ANY, wxString::Format(_("Version %s"), wxGetApp().GetAppVersion()));
    version->SetFont(m_fntSub);
    version->SetForegroundColour(m_clrSub);
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

    // Hide the cosmetic logo part if the screen is too small:
    auto minFullSize = sizer->GetMinSize().y + PX(50);
    Bind(wxEVT_SIZE, [=](wxSizeEvent& e){
        sizer->Show((size_t)0, e.GetSize().y >= minFullSize);
        e.Skip();
    });
}




EmptyPOScreenPanel::EmptyPOScreenPanel(PoeditFrame *parent)
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
    header->SetForegroundColour(m_clrHeader);
    sizer->Add(header, wxSizerFlags().Center().PXBorderAll());

    auto explain = new wxStaticText(this, wxID_ANY, _("Translatable entries aren't added manually in the Gettext system, but are automatically extracted\nfrom source code. This way, they stay up to date and accurate.\nTranslators typically use PO template files (POTs) prepared for them by the developer."));
    explain->SetFont(m_fntNorm);
    explain->SetForegroundColour(m_clrNorm);
    sizer->Add(explain, wxSizerFlags());

    auto learnMore = new wxHyperlinkCtrl(this, wxID_ANY, _("(Learn more about GNU gettext)"), "http://www.gnu.org/software/gettext/manual/html_node/");
    learnMore->SetFont(m_fntNorm);
    sizer->Add(learnMore, wxSizerFlags().PXBorder(wxTOP|wxBOTTOM).Align(wxALIGN_RIGHT));

    auto explain2 = new wxStaticText(this, wxID_ANY, _("The simplest way to fill this catalog is to update it from a POT:"));
    explain2->SetFont(m_fntNorm);
    explain2->SetForegroundColour(m_clrNorm);
    sizer->Add(explain2, wxSizerFlags().PXDoubleBorder(wxTOP));

    sizer->Add(new ActionButton(
                       this, XRCID("menu_update_from_pot"),
                       _("Update from POT"),
                       _("Take translatable strings from an existing POT template.")),
               wxSizerFlags().PXBorderAll().Expand());

    auto explain3 = new wxStaticText(this, wxID_ANY, _("You can also extract translatable strings directly from the source code:"));
    explain3->SetFont(m_fntNorm);
    explain3->SetForegroundColour(m_clrNorm);
    sizer->Add(explain3, wxSizerFlags().PXDoubleBorder(wxTOP));

    auto btnSources = new ActionButton(
                       this, wxID_ANY,
                       _("Extract from sources"),
                       _("Configure source code extraction in Properties."));
    sizer->Add(btnSources, wxSizerFlags().PXBorderAll().Expand());

    btnSources->Bind(wxEVT_BUTTON, [=](wxCommandEvent&){
        parent->EditCatalogPropertiesAndUpdateFromSources();
    });
}
