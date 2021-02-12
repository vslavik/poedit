/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 2000-2020 Vaclav Slavik
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

#include "progressinfo.h"

#include "customcontrols.h"
#include "hidpi.h"
#include "titleless_window.h"
#include "utility.h"

#include <wx/app.h>
#include <wx/dialog.h>
#include <wx/evtloop.h>
#include <wx/log.h>
#include <wx/gauge.h>
#include <wx/stattext.h>
#include <wx/dialog.h>
#include <wx/sizer.h>
#include <wx/button.h>
#include <wx/config.h>


class ProgressWindow : public TitlelessDialog
{
public:
    ProgressWindow(wxWindow *parent, const wxString& title, bool *cancel);
    void UpdateMessage(const wxString& text);
    void OnCancel(wxCommandEvent&);

    wxStaticBitmap *m_image;
    wxStaticText *m_title;
    SecondaryLabel *m_message;
    wxGauge *m_gauge;

    bool *m_cancelFlag;
};


ProgressWindow::ProgressWindow(wxWindow *parent, const wxString& title, bool *cancel)
    : TitlelessDialog(parent, wxID_ANY, title, wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE & ~wxCLOSE_BOX)
{
    m_cancelFlag = cancel;

    auto sizer = new wxBoxSizer(wxVERTICAL);
    auto topsizer = new wxBoxSizer(wxHORIZONTAL);
    sizer->Add(topsizer, wxSizerFlags(1).Expand().Border(wxALL, PX(20)));

    wxSize logoSize(PX(64), PX(64));
#if defined(__WXMSW__)
    wxIcon logo;
    logo.LoadFile("appicon", wxBITMAP_TYPE_ICO_RESOURCE, 256, 256);
    {
        wxBitmap bmp;
        bmp.CopyFromIcon(logo);
        logo.CopyFromBitmap(wxBitmap(bmp.ConvertToImage().Scale(logoSize.x, logoSize.y, wxIMAGE_QUALITY_BICUBIC)));
    }
#elif defined(__WXGTK__)
    auto logo = wxArtProvider::GetIcon("net.poedit.Poedit", wxART_FRAME_ICON, logoSize);
#else
    auto logo = wxArtProvider::GetBitmap("Poedit");
#endif
    m_image = new wxStaticBitmap(this, wxID_ANY, logo, wxDefaultPosition, logoSize);
    m_image->SetMinSize(logoSize);
    topsizer->Add(m_image, wxSizerFlags().Center().Border(wxTOP, MSW_OR_OTHER(PX(10), 0)));

    auto infosizer = new wxBoxSizer(wxVERTICAL);
    topsizer->Add(infosizer, wxSizerFlags().Center().Border(wxLEFT, PX(10)));

    m_title = new wxStaticText(this, wxID_ANY, title);
#ifdef __WXMSW__
    auto titleFont = m_title->GetFont().Scaled(1.3f);
#else
    auto titleFont = m_title->GetFont().Bold();
#endif
    m_title->SetFont(titleFont);
    infosizer->Add(m_title, wxSizerFlags().Left().Border(wxBOTTOM, PX(3)));
    m_message = new SecondaryLabel(this, "");
    infosizer->Add(m_message, wxSizerFlags().Left().Border(wxBOTTOM, PX(2)));
    m_gauge = new wxGauge(this, wxID_ANY, 100, wxDefaultPosition, wxSize(PX(350), -1), wxGA_SMOOTH);
    m_gauge->Pulse();
    infosizer->Add(m_gauge, wxSizerFlags().Expand());

    auto cancelButton = new wxButton(this, wxID_CANCEL);
    cancelButton->Bind(wxEVT_BUTTON, &ProgressWindow::OnCancel, this);
    sizer->Add(cancelButton, wxSizerFlags().Right().Border(wxRIGHT|wxBOTTOM, PX(20)));

    SetSizerAndFit(sizer);
    if (parent)
        CenterOnParent();
}

void ProgressWindow::UpdateMessage(const wxString& text)
{
    if (*m_cancelFlag)
        return;

    m_message->SetLabel(text);
    m_message->Refresh();
    m_message->Update();
}

void ProgressWindow::OnCancel(wxCommandEvent&)
{
    ((wxButton*)FindWindow(wxID_CANCEL))->Enable(false);
    UpdateMessage(_(L"Cancelling…"));
    *m_cancelFlag = true;
}


ProgressInfo::ProgressInfo(wxWindow *parent, const wxString& title)
{
    m_cancelled = false;
    m_dlg = new ProgressWindow(parent, title, &m_cancelled);
    m_dlg->Show(true);
    m_disabler = new wxWindowDisabler(m_dlg);
}

ProgressInfo::~ProgressInfo()
{
    Done();
}

void ProgressInfo::Hide()
{
    m_dlg->Show(false);
    m_dlg->Refresh();
    wxEventLoop::GetActive()->YieldFor(wxEVT_CATEGORY_UI);
}


void ProgressInfo::Show()
{
    m_dlg->Show(true);
    m_dlg->Refresh();
    wxEventLoop::GetActive()->YieldFor(wxEVT_CATEGORY_UI);
}

void ProgressInfo::Done()
{
    if (m_disabler)
    {
        delete m_disabler;
        m_disabler = nullptr;
    }
    if (m_dlg)
    {
        m_dlg->Destroy();
        m_dlg = nullptr;
    }
}

void ProgressInfo::SetGaugeMax(int limit)
{
    m_dlg->m_gauge->SetRange(limit);
}

bool ProgressInfo::UpdateGauge(int increment)
{
    wxGauge *g = m_dlg->m_gauge;
    g->SetValue(g->GetValue() + increment);

#ifdef __WXOSX__
    // Set again the message to workaround a wxOSX bug
    auto txt = m_dlg->m_message;
    txt->SetLabel(txt->GetLabel());
    txt->Update();
#endif

    return !m_cancelled;
}

void ProgressInfo::ResetGauge(int value)
{
    m_dlg->m_gauge->SetValue(value);
}

void ProgressInfo::PulseGauge()
{
    m_dlg->m_gauge->Pulse();
}

void ProgressInfo::UpdateMessage(const wxString& text)
{
    m_dlg->UpdateMessage(text);
    m_dlg->Refresh();
    wxEventLoop::GetActive()->YieldFor(wxEVT_CATEGORY_UI);
}
