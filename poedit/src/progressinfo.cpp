
/*

    poedit, a wxWindows i18n catalogs editor

    ---------------
      progressinfo.cpp
    
      Shows progress of lengthy operation
    
      (c) Vaclav Slavik, 2000

*/


#ifdef __GNUG__
#pragma implementation
#endif

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
    wxTheXmlResource->LoadDialog(m_dlg, NULL, _T("parser_progress"));
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
    XMLCTRL(*m_dlg, "progress", wxGauge)->SetRange(limit);
}

void ProgressInfo::UpdateGauge(int increment)
{
    wxGauge *g = XMLCTRL(*m_dlg, "progress", wxGauge);
    g->SetValue(g->GetValue() + increment);
}

void ProgressInfo::ResetGauge(int value)
{
    XMLCTRL(*m_dlg, "progress", wxGauge)->SetValue(value);
}

void ProgressInfo::UpdateMessage(const wxString& text)
{
    XMLCTRL(*m_dlg, "info", wxStaticText)->SetLabel(text);
    m_dlg->Refresh();
    wxYield();
}
