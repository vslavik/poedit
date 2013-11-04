/*
 *  This file is part of Poedit (http://www.poedit.net)
 *
 *  Copyright (C) 2013 Vaclav Slavik
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

#include "edapp.h"

#include <wx/dcbuffer.h>
#include <wx/statbmp.h>
#include <wx/stattext.h>
#include <wx/sizer.h>
#include <wx/artprov.h>
#include <wx/font.h>
#include <wx/button.h>
#include <wx/settings.h>
#include <wx/xrc/xmlres.h>

#ifdef __WXOSX__
#include <wx/cocoa/string.h>
#else
#include <wx/commandlinkbutton.h>
#endif

#ifdef __WXMSW__
// wxStatic{Bitmap,Text} is not transparent on Windows, use generic variants
#include <wx/generic/statbmpg.h>
#include <wx/generic/stattextg.h>
#define wxStaticText   wxGenericStaticText
#define wxStaticBitmap wxGenericStaticBitmap
#endif

namespace
{

#ifdef __WXOSX__

class ActionButton : public wxButton
{
public:
    ActionButton(wxWindow *parent, wxWindowID winid, const wxString& label, const wxString& note)
        : wxButton(parent, winid, "", wxDefaultPosition, wxSize(350, 50))
    {
        NSButton *btn = (NSButton*)GetHandle();

        NSString *str = wxNSStringWithWxString(label + "\n" + note);
        NSMutableAttributedString *s = [[NSMutableAttributedString alloc] initWithString:str];
        [s addAttribute:NSFontAttributeName
                  value:[NSFont boldSystemFontOfSize:0]
                  range:NSMakeRange(0, label.length())];
        [s addAttribute:NSFontAttributeName
                  value:[NSFont systemFontOfSize:[NSFont systemFontSize]-1.5]
                  range:NSMakeRange(label.length()+1, note.length())];
        [btn setAttributedTitle:s];
        [btn setShowsBorderOnlyWhileMouseInside:YES];
        [btn setBezelStyle:NSSmallSquareBezelStyle];
        [btn setButtonType:NSMomentaryPushInButton];
    }
};

#else

#if 0
class ActionButton : public wxCommandLinkButton
{
public:
    ActionButton(wxWindow *parent, wxWindowID winid, const wxString& label, const wxString& note)
        : wxCommandLinkButton(parent, winid, label, note, wxDefaultPosition, wxSize(350, -1), wxTRANSPARENT_WINDOW)
    {
        SetSize(350, -1);
    }
};
#endif
typedef wxCommandLinkButton ActionButton;

#endif

} // anonymous namespace


WelcomeScreenPanel::WelcomeScreenPanel(wxWindow *parent)
    : wxPanel(parent, wxID_ANY)
{
#if 1//ndef __WXMSW__
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    Bind(wxEVT_PAINT, &WelcomeScreenPanel::OnPaint, this);
#endif

    auto sizer = new wxBoxSizer(wxVERTICAL);
    auto uberSizer = new wxBoxSizer(wxHORIZONTAL);
    uberSizer->AddStretchSpacer();
    uberSizer->Add(sizer, wxSizerFlags().Center().Border(wxALL, 50));
    uberSizer->AddStretchSpacer();
    SetSizer(uberSizer);

    auto hdr = new wxStaticBitmap(this, wxID_ANY, wxArtProvider::GetBitmap("PoeditWelcome"), wxDefaultPosition, wxDefaultSize, wxTRANSPARENT_WINDOW);
    sizer->Add(hdr, wxSizerFlags().Center());

#if defined(__WXMAC__)
    #define HEADER_FACE "Helvetica Neue"
    wxFont fntHeader(wxFontInfo(30).FaceName(HEADER_FACE).Light());
    wxFont fntSub(wxFontInfo(11).FaceName(HEADER_FACE).Light());
#elif defined(__WXMSW__)
    #define HEADER_FACE "Segoe UI Light"
    wxFont fntHeader(wxFontInfo(24).FaceName(HEADER_FACE).Light());
    wxFont fntSub(wxFontInfo(10).FaceName(HEADER_FACE).Light());
#else
    #define HEADER_FACE "sans serif"
    wxFont fntHeader(wxFontInfo(30).FaceName(HEADER_FACE).Light());
    wxFont fntSub(wxFontInfo(11).FaceName(HEADER_FACE).Light());
#endif

    auto header = new wxStaticText(this, wxID_ANY, _("Welcome to Poedit"), wxDefaultPosition, wxDefaultSize, wxTRANSPARENT_WINDOW);
    header->SetFont(fntHeader);
    header->SetForegroundColour("#555555");
    sizer->Add(header, wxSizerFlags().Center().Border(wxTOP, 10));

    auto version = new wxStaticText(this, wxID_ANY, wxString::Format(_("Version %s"), wxGetApp().GetAppVersion()), wxDefaultPosition, wxDefaultSize, wxTRANSPARENT_WINDOW);
    version->SetFont(fntSub);
    version->SetForegroundColour("#aaaaaa");
    sizer->Add(version, wxSizerFlags().Center());

    sizer->AddSpacer(20);

    sizer->Add(new ActionButton(
                       this, wxID_OPEN,
                       _("Edit a translation"),
                       _("Open an existing PO file and edit the translation.")),
               wxSizerFlags().Border().Expand());

    sizer->Add(new ActionButton(
                       this, XRCID("menu_new_from_pot"),
                       _("Create new translation"),
                       _("Take an existing PO file or POT template and create a new translation from it.")),
               wxSizerFlags().Border().Expand());

    sizer->AddSpacer(50);

    // Translate all button events to wxEVT_MENU and send them to the frame.
    Bind(wxEVT_BUTTON, [=](wxCommandEvent& e){
        wxCommandEvent event(wxEVT_MENU, e.GetId());
        event.SetEventObject(this);
        GetParent()->GetEventHandler()->AddPendingEvent(event);
    });
}


void WelcomeScreenPanel::OnPaint(wxPaintEvent&)
{
    wxAutoBufferedPaintDC dc(this);
    wxRect rect(dc.GetSize());

    dc.GradientFillLinear(rect, wxColour("#ffffff"), wxColour("#fffceb"), wxBOTTOM);
}