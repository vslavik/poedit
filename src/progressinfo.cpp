/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 2000-2017 Vaclav Slavik
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

#include <wx/app.h>
#include <wx/dialog.h>
#include <wx/evtloop.h>
#include <wx/log.h>
#include <wx/xrc/xmlres.h>
#include <wx/gauge.h>
#include <wx/stattext.h>
#include <wx/dialog.h>
#include <wx/button.h>
#include <wx/config.h>

class ProgressDlg : public wxDialog
{
    public:
        ProgressDlg(bool *cancel) : wxDialog(), m_cancelFlag(cancel) {}
        
    private:
        bool *m_cancelFlag;
    
        DECLARE_EVENT_TABLE()

        void OnCancel(wxCommandEvent&)
        {
            ((wxButton*)FindWindow(wxID_CANCEL))->Enable(false);
            *m_cancelFlag = true;
        }
};

BEGIN_EVENT_TABLE(ProgressDlg, wxDialog)
   EVT_BUTTON(wxID_CANCEL, ProgressDlg::OnCancel)
END_EVENT_TABLE()

ProgressInfo::ProgressInfo(wxWindow *parent, const wxString& title)
{
    m_cancelled = false;
    m_dlg = new ProgressDlg(&m_cancelled);
    wxXmlResource::Get()->LoadDialog(m_dlg, parent, "extractor_progress");
    m_dlg->SetTitle(title);
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
    XRCCTRL(*m_dlg, "progress", wxGauge)->SetRange(limit);
}

bool ProgressInfo::UpdateGauge(int increment)
{
    wxGauge *g = XRCCTRL(*m_dlg, "progress", wxGauge);
    g->SetValue(g->GetValue() + increment);

#ifdef __WXOSX__
    // Set again the message to workaround a wxOSX bug
    wxStaticText *txt = XRCCTRL(*m_dlg, "info", wxStaticText);
    txt->SetLabel(txt->GetLabel());
    txt->Update();
#endif

    return !m_cancelled;
}

void ProgressInfo::ResetGauge(int value)
{
    XRCCTRL(*m_dlg, "progress", wxGauge)->SetValue(value);
}

void ProgressInfo::PulseGauge()
{
    XRCCTRL(*m_dlg, "progress", wxGauge)->Pulse();
}

void ProgressInfo::UpdateMessage(const wxString& text)
{
    wxStaticText *txt = XRCCTRL(*m_dlg, "info", wxStaticText);
    txt->SetLabel(text);
    txt->Refresh();
    txt->Update();
    m_dlg->Refresh();
    wxEventLoop::GetActive()->YieldFor(wxEVT_CATEGORY_UI);
}
