/*
 *  This file is part of Poedit (http://www.poedit.net)
 *
 *  Copyright (C) 2008-2013 Vaclav Slavik
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

#include "attentionbar.h"

#include <wx/sizer.h>
#include <wx/settings.h>
#include <wx/artprov.h>
#include <wx/bmpbuttn.h>
#include <wx/stattext.h>
#include <wx/statbmp.h>
#include <wx/config.h>
#include <wx/dcclient.h>

#ifdef __WXMAC__
#include "osx_helpers.h"
#endif

#ifdef __WXMAC__
    #define SMALL_BORDER   5
    #define BUTTONS_SPACE 10
#else
    #define SMALL_BORDER   3
    #define BUTTONS_SPACE  3
#endif

BEGIN_EVENT_TABLE(AttentionBar, wxPanel)
    EVT_BUTTON(wxID_CLOSE, AttentionBar::OnClose)
    EVT_BUTTON(wxID_ANY, AttentionBar::OnAction)
END_EVENT_TABLE()

AttentionBar::AttentionBar(wxWindow *parent)
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
              wxTAB_TRAVERSAL | wxBORDER_NONE)
{
#ifdef __WXMSW__
    wxColour bg("#FFF499"); // match Visual Studio 2012+'s aesthetics
#else
    wxColour bg = wxSystemSettings::GetColour(wxSYS_COLOUR_INFOBK);
#endif
    SetBackgroundColour(bg);
    SetForegroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_INFOTEXT));

    m_icon = new wxStaticBitmap(this, wxID_ANY, wxNullBitmap);
    m_label = new wxStaticText(this, wxID_ANY, wxEmptyString);

    m_buttons = new wxBoxSizer(wxHORIZONTAL);

    wxButton *btnClose =
            new wxBitmapButton
                (
                    this, wxID_CLOSE,
                    wxArtProvider::GetBitmap("window-close", wxART_MENU),
                    wxDefaultPosition, wxDefaultSize,
                    wxNO_BORDER
                );
    btnClose->SetToolTip(_("Hide this notification message"));
#ifdef __WXMSW__
    btnClose->SetBackgroundColour(bg);
#endif
#ifdef __WXMAC__
    SetWindowVariant(wxWINDOW_VARIANT_SMALL);
#endif
#if defined(__WXMAC__) || defined(__WXMSW__)
    Bind(wxEVT_PAINT, &AttentionBar::OnPaint, this);
    wxFont boldFont = wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT);
    boldFont.SetWeight(wxFONTWEIGHT_BOLD);
    m_label->SetFont(boldFont);
#endif //

    wxSizer *sizer = new wxBoxSizer(wxHORIZONTAL);
    sizer->AddSpacer(wxSizerFlags::GetDefaultBorder());
    sizer->Add(m_icon, wxSizerFlags().Center().Border(wxRIGHT, SMALL_BORDER));
    sizer->Add(m_label, wxSizerFlags(1).Center().Border(wxALL, SMALL_BORDER));
    sizer->Add(m_buttons, wxSizerFlags().Center().Border(wxALL, SMALL_BORDER));
    sizer->Add(btnClose, wxSizerFlags().Center().Border(wxALL, SMALL_BORDER));

    SetSizer(sizer);

    // the bar should be initially hidden
    Show(false);
}


#ifdef __WXMAC__
void AttentionBar::OnPaint(wxPaintEvent&)
{
    wxPaintDC dc(this);

    wxRect rect(dc.GetSize());

    dc.SetPen(wxColour(254,230,93));
    dc.DrawLine(rect.GetTopLeft(), rect.GetTopRight());
    dc.SetPen(wxColour(176,120,7));
    dc.DrawLine(rect.GetBottomLeft(), rect.GetBottomRight());

    rect.Deflate(0,1);
    dc.GradientFillLinear
       (
         rect,
         wxColour(254,220,48),
         wxColour(253,188,11),
         wxDOWN
       );
}
#endif // __WXMAC__

#ifdef __WXMSW__
void AttentionBar::OnPaint(wxPaintEvent&)
{
    wxPaintDC dc(this);

    wxRect rect(dc.GetSize());
    dc.SetBrush(*wxTRANSPARENT_BRUSH);
    dc.SetPen(wxColour("#e5dd8c"));
    dc.DrawRoundedRectangle(rect, 1);
}
#endif // __WXMSW__


void AttentionBar::ShowMessage(const AttentionMessage& msg)
{
    if ( msg.IsBlacklisted() )
        return;

    wxString iconName;

    switch ( msg.m_kind )
    {
        case AttentionMessage::Info:
            iconName = wxART_INFORMATION;
            break;
        case AttentionMessage::Warning:
            iconName = wxART_WARNING;
            break;
        case AttentionMessage::Error:
            iconName = wxART_ERROR;
            break;
    }

    m_icon->SetBitmap(wxArtProvider::GetBitmap(iconName, wxART_MENU, wxSize(16, 16)));
    m_label->SetLabelText(msg.m_text);

    m_buttons->Clear(true/*delete_windows*/);
    m_actions.clear();

    for ( AttentionMessage::Actions::const_iterator i = msg.m_actions.begin();
          i != msg.m_actions.end(); ++i )
    {
        wxButton *b = new wxButton(this, wxID_ANY, i->first);
#ifdef __WXMAC__
        MakeButtonRounded(b->GetHandle());
#endif
        m_buttons->Add(b, wxSizerFlags().Center().Border(wxRIGHT, BUTTONS_SPACE));
        m_actions[b] = i->second;
    }

    // we need to size the control correctly _and_ lay out the controls if this
    // is the first time it's being shown, otherwise we can get garbled look:
    SetSize(GetParent()->GetClientSize().x,
            GetBestSize().y);
    Layout();

    Show();
    GetParent()->Layout();
}

void AttentionBar::HideMessage()
{
    Hide();
    GetParent()->Layout();
}

void AttentionBar::OnClose(wxCommandEvent& WXUNUSED(event))
{
    HideMessage();
}

void AttentionBar::OnAction(wxCommandEvent& event)
{
    ActionsMap::const_iterator i = m_actions.find(event.GetEventObject());
    if ( i == m_actions.end() )
    {
        event.Skip();
        return;
    }

    HideMessage();
    i->second();
}


/* static */
void AttentionMessage::AddToBlacklist(const wxString& id)
{
    wxConfig::Get()->Write
        (
            wxString::Format("/messages/dont_show/%s", id.c_str()),
            (long)true
        );
}

/* static */
bool AttentionMessage::IsBlacklisted(const wxString& id)
{
    return wxConfig::Get()->ReadBool
        (
            wxString::Format("/messages/dont_show/%s", id.c_str()),
            false
        );
}

void AttentionMessage::AddDontShowAgain()
{
    AddAction(_("Don't show again"),
              std::bind(&AttentionMessage::AddToBlacklist, m_id));
}
