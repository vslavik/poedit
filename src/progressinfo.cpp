/*
 *  This file is part of poEdit (http://www.poedit.net)
 *
 *  Copyright (C) 2000-2005 Vaclav Slavik
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
 *  $Id$
 *
 *  Shows progress of lengthy operation
 *
 */

#include <wx/wxprec.h>

#include "progressinfo.h"

#include <wx/dialog.h>
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

        void OnCancel(wxCommandEvent& event)
        {
            ((wxButton*)FindWindow(wxID_CANCEL))->Enable(false);
            *m_cancelFlag = true;
        }
};

BEGIN_EVENT_TABLE(ProgressDlg, wxDialog)
   EVT_BUTTON(wxID_CANCEL, ProgressDlg::OnCancel)
END_EVENT_TABLE()

ProgressInfo::ProgressInfo()
{
    m_cancelled = false;
    m_dlg = new ProgressDlg(&m_cancelled);
    wxXmlResource::Get()->LoadDialog(m_dlg, NULL, _T("parser_progress"));
    wxPoint pos(wxConfig::Get()->Read(_T("progress_pos_x"), -1),
               wxConfig::Get()->Read(_T("progress_pos_y"), -1));
    if (pos.x != -1 && pos.y != -1) m_dlg->Move(pos);
    m_dlg->Show(true);
    m_disabler = new wxWindowDisabler(m_dlg);
}

ProgressInfo::~ProgressInfo()
{
    delete m_disabler;
    wxConfig::Get()->Write(_T("progress_pos_x"), (long)m_dlg->GetPosition().x);
    wxConfig::Get()->Write(_T("progress_pos_y"), (long)m_dlg->GetPosition().y);
    m_dlg->Destroy();
}

void ProgressInfo::SetTitle(const wxString& text)
{
    m_dlg->SetTitle(text);
    wxYield();
}
           
void ProgressInfo::SetGaugeMax(int limit)
{
    XRCCTRL(*m_dlg, "progress", wxGauge)->SetRange(limit);
}

void ProgressInfo::UpdateGauge(int increment)
{
    wxGauge *g = XRCCTRL(*m_dlg, "progress", wxGauge);
    g->SetValue(g->GetValue() + increment);
}

void ProgressInfo::ResetGauge(int value)
{
    XRCCTRL(*m_dlg, "progress", wxGauge)->SetValue(value);
}

void ProgressInfo::UpdateMessage(const wxString& text)
{
    XRCCTRL(*m_dlg, "info", wxStaticText)->SetLabel(text);
    m_dlg->Refresh();
    wxYield();
}
