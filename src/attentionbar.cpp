/*
 *  This file is part of Poedit (http://www.poedit.net)
 *
 *  Copyright (C) 2008 Vaclav Slavik
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

#if defined(__WXMSW__) && wxCHECK_VERSION(2,9,0)
    #define USE_SLIDE_EFFECT
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
    wxColour bg = wxSystemSettings::GetColour(wxSYS_COLOUR_INFOBK);
    SetBackgroundColour(bg);
    SetForegroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_INFOTEXT));

#ifdef __WXMAC__
    SetWindowVariant(wxWINDOW_VARIANT_SMALL);
#endif

    m_icon = new wxStaticBitmap(this, wxID_ANY, wxNullBitmap);
    m_label = new wxStaticText(this, wxID_ANY, wxEmptyString);

    m_buttons = new wxBoxSizer(wxHORIZONTAL);

    wxButton *btnClose =
            new wxBitmapButton
                (
                    this, wxID_CLOSE,
                    wxArtProvider::GetBitmap(_T("window-close"), wxART_MENU),
                    wxDefaultPosition, wxDefaultSize,
                    wxNO_BORDER
                );
    btnClose->SetToolTip(_("Hide this notification message"));
#ifndef __WXGTK__
    btnClose->SetBackgroundColour(bg);
#endif

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

    m_icon->SetBitmap(wxArtProvider::GetBitmap(iconName, wxART_MENU));
    m_label->SetLabelText(msg.m_text);

    m_buttons->Clear(true/*delete_windows*/);
    m_actions.clear();

    for ( AttentionMessage::Actions::const_iterator i = msg.m_actions.begin();
          i != msg.m_actions.end(); ++i )
    {
        wxButton *b = new wxButton(this, wxID_ANY, i->first);
        m_buttons->Add(b, wxSizerFlags().Center().Border(wxRIGHT, BUTTONS_SPACE));
        m_actions[b] = i->second;
    }

    // we need to size the control correctly _and_ lay out the controls if this
    // is the first time it's being shown, otherwise we can get garbled look:
    SetSize(GetParent()->GetClientSize().x,
            GetBestSize().y);
    Layout();

#ifdef USE_SLIDE_EFFECT
    ShowWithEffect(wxSHOW_EFFECT_SLIDE_TO_BOTTOM);
#else
    Show();
#endif
    GetParent()->Layout();
}

void AttentionBar::HideMessage()
{
#ifdef USE_SLIDE_EFFECT
    HideWithEffect(wxSHOW_EFFECT_SLIDE_TO_TOP);
#else
    Hide();
#endif
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
            wxString::Format(_T("/messages/dont_show/%s"), id.c_str()),
            (long)true
        );
}

/* static */
bool AttentionMessage::IsBlacklisted(const wxString& id)
{
    return wxConfig::Get()->Read
        (
            wxString::Format(_T("/messages/dont_show/%s"), id.c_str()),
            (long)false
        );
}

void AttentionMessage::AddDontShowAgain()
{
    AddAction(_("Don't show again"),
              boost::bind(&AttentionMessage::AddToBlacklist, m_id));
}
