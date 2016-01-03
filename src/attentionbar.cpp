/*
 *  This file is part of Poedit (http://poedit.net)
 *
 *  Copyright (C) 2008-2015 Vaclav Slavik
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

#include "utility.h"

#include <wx/checkbox.h>
#include <wx/sizer.h>
#include <wx/settings.h>
#include <wx/artprov.h>
#include <wx/bmpbuttn.h>
#include <wx/stattext.h>
#include <wx/statbmp.h>
#include <wx/config.h>
#include <wx/dcclient.h>

#include "customcontrols.h"
#include "hidpi.h"

#ifdef __WXOSX__
#include "osx_helpers.h"
#endif

#ifdef __WXOSX__
    #define SMALL_BORDER   PX(7)
    #define BUTTONS_SPACE PX(10)
#else
    #define SMALL_BORDER   PX(3)
    #define BUTTONS_SPACE  PX(5)
#endif

BEGIN_EVENT_TABLE(AttentionBar, wxPanel)
    EVT_BUTTON(wxID_CLOSE, AttentionBar::OnClose)
    EVT_BUTTON(wxID_ANY, AttentionBar::OnAction)
END_EVENT_TABLE()

AttentionBar::AttentionBar(wxWindow *parent)
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
              wxTAB_TRAVERSAL | wxBORDER_NONE)
{
#ifdef __WXOSX__
    SetWindowVariant(wxWINDOW_VARIANT_SMALL);
#endif

#ifndef __WXGTK__
    m_icon = new wxStaticBitmap(this, wxID_ANY, wxNullBitmap);
#endif
    m_label = new AutoWrappingText(this, "");

    m_explanation = new AutoWrappingText(this, "");
    m_explanation->SetForegroundColour(GetBackgroundColour().ChangeLightness(40));

    m_buttons = new wxBoxSizer(wxHORIZONTAL);

    m_checkbox = new wxCheckBox(this, wxID_ANY, "");

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
    btnClose->SetBackgroundColour(GetBackgroundColour());
#endif

#if defined(__WXOSX__) || defined(__WXMSW__)
    wxFont boldFont = wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT);
    boldFont.SetWeight(wxFONTWEIGHT_BOLD);
    m_label->SetFont(boldFont);
#endif

    Bind(wxEVT_PAINT, &AttentionBar::OnPaint, this);

    wxSizer *sizer = new wxBoxSizer(wxHORIZONTAL);
    sizer->AddSpacer(PXDefaultBorder);
#ifndef __WXGTK__
    sizer->Add(m_icon, wxSizerFlags().Center().Border(wxALL, SMALL_BORDER));
#endif

    auto labelSizer = new wxBoxSizer(wxVERTICAL);
    labelSizer->Add(m_label, wxSizerFlags().Expand());
    labelSizer->Add(m_explanation, wxSizerFlags().Expand().Border(wxTOP|wxRIGHT, PX(4)));
    sizer->Add(labelSizer, wxSizerFlags(1).Center().PXDoubleBorder(wxALL));
    sizer->AddSpacer(PX(20));
    sizer->Add(m_buttons, wxSizerFlags().Center().Border(wxALL, SMALL_BORDER));
    sizer->Add(m_checkbox, wxSizerFlags().Center().Border(wxRIGHT, BUTTONS_SPACE));
    sizer->Add(btnClose, wxSizerFlags().Center().Border(wxALL, SMALL_BORDER));
#ifdef __WXMSW__
    sizer->AddSpacer(PX(4));
#endif

    SetSizer(sizer);

    // the bar should be initially hidden
    Show(false);
}


void AttentionBar::OnPaint(wxPaintEvent&)
{
    wxPaintDC dc(this);
    wxRect rect(dc.GetSize());

    auto bg = GetBackgroundColour();
    dc.SetBrush(*wxTRANSPARENT_BRUSH);
    dc.SetPen(bg.ChangeLightness(90));
#ifndef __WXOSX__
    dc.DrawLine(rect.GetTopLeft(), rect.GetTopRight());
#endif
    dc.DrawLine(rect.GetBottomLeft(), rect.GetBottomRight());
}


void AttentionBar::ShowMessage(const AttentionMessage& msg)
{
    if ( msg.IsBlacklisted() )
        return;

#ifdef __WXGTK__
    switch ( msg.m_kind )
    {
        case AttentionMessage::Info:
            SetBackgroundColour(wxColour(252,252,189));
            break;
        case AttentionMessage::Warning:
            SetBackgroundColour(wxColour(250,173,61));
            break;
        case AttentionMessage::Question:
            SetBackgroundColour(wxColour(138,173,212));
            break;
        case AttentionMessage::Error:
            SetBackgroundColour(wxColour(237,54,54));
            break;
    }
#else

    switch ( msg.m_kind )
    {
        case AttentionMessage::Question:
            SetBackgroundColour("#ABE887");
            break;
        default:
    #ifdef __WXMSW__
            SetBackgroundColour("#FFF499"); // match Visual Studio 2012+'s aesthetics
    #else
            SetBackgroundColour("#FCDE59");
    #endif
            break;
    }

#ifdef __WXMSW__
    auto bg = GetBackgroundColour();
    for (auto w : GetChildren())
        w->SetBackgroundColour(bg);
#endif

    wxString iconName;
    switch ( msg.m_kind )
    {
        case AttentionMessage::Info:
            iconName = wxART_INFORMATION;
            break;
        case AttentionMessage::Warning:
            iconName = wxART_WARNING;
            break;
        case AttentionMessage::Question:
            iconName = wxART_QUESTION;
            break;
        case AttentionMessage::Error:
            iconName = wxART_ERROR;
            break;
    }
    m_icon->SetBitmap(wxArtProvider::GetBitmap(iconName, wxART_MENU, wxSize(PX(16), PX(16))));
#endif

    m_label->SetAndWrapLabel(msg.m_text);
    m_explanation->SetAndWrapLabel(msg.m_explanation);
    m_explanation->GetContainingSizer()->Show(m_explanation, !msg.m_explanation.empty());
    m_checkbox->SetLabel(msg.m_checkbox);
    m_checkbox->GetContainingSizer()->Show(m_checkbox, !msg.m_checkbox.empty());

    m_buttons->Clear(true/*delete_windows*/);
    m_actions.clear();

    for ( AttentionMessage::Actions::const_iterator i = msg.m_actions.begin();
          i != msg.m_actions.end(); ++i )
    {
        wxButton *b = new wxButton(this, wxID_ANY, i->first);
#ifdef __WXOSX__
        MakeButtonRounded(b->GetHandle());
#endif
        m_buttons->Add(b, wxSizerFlags().Center().Border(wxRIGHT, BUTTONS_SPACE));
        m_actions[b] = i->second;
    }

    // we need to size the control correctly _and_ lay out the controls if this
    // is the first time it's being shown, otherwise we can get garbled look:
    SetSize(GetParent()->GetClientSize().x, GetBestSize().y);
    Layout();

    Refresh();
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

    // first perform the action...
    AttentionMessage::ActionInfo info;
    info.checkbox = m_checkbox->IsShown() && m_checkbox->IsChecked();
    i->second(info);

    // ...then hide the message
    HideMessage();
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
    auto id = m_id;
    AddAction(
        MSW_OR_OTHER(_("Don't show again"), _("Don't Show Again")), [id]{
        AddToBlacklist(id);
    });
}
