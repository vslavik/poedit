
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
#include <wx/xml/xmlres.h>
#include <wx/gauge.h>
#include <wx/stattext.h>
#include <wx/dialog.h>
#include <wx/button.h>
#include <wx/config.h>

class ProgressDlg : public wxDialog
{
    public:
        ProgressDlg(bool *cancel) : wxDialog(), m_CancelFlag(cancel) {}
        
    private:
        bool *m_CancelFlag;
    
        DECLARE_EVENT_TABLE()

        void OnCancel(wxCommandEvent& event)
        {
            ((wxButton*)FindWindow(wxID_CANCEL))->Enable(false);
            *m_CancelFlag = true;
        }
};

BEGIN_EVENT_TABLE(ProgressDlg, wxDialog)
   EVT_BUTTON(wxID_CANCEL, ProgressDlg::OnCancel)
END_EVENT_TABLE()


ProgressInfo::ProgressInfo()
{
    m_Cancelled = false;
    m_Dlg = new ProgressDlg(&m_Cancelled);
    wxTheXmlResource->LoadDialog(m_Dlg, NULL, "parser_progress");
    wxPoint pos(wxConfig::Get()->Read("progress_pos_x", -1),
               wxConfig::Get()->Read("progress_pos_y", -1));
    if (pos.x != -1 && pos.y != -1) m_Dlg->Move(pos);
    m_Dlg->Show(true);
    m_Disabler = new wxWindowDisabler(m_Dlg);
}



ProgressInfo::~ProgressInfo()
{
    delete m_Disabler;
    wxConfig::Get()->Write("progress_pos_x", (long)m_Dlg->GetPosition().x);
    wxConfig::Get()->Write("progress_pos_y", (long)m_Dlg->GetPosition().y);
    m_Dlg->Destroy();
}



void ProgressInfo::SetTitle(const wxString& text)
{
    m_Dlg->SetTitle(text);
    wxYield();
}


            
void ProgressInfo::SetGaugeMax(int limit)
{
    XMLCTRL(*m_Dlg, "progress", wxGauge)->SetRange(limit);
}



void ProgressInfo::UpdateGauge(int increment)
{
    wxGauge *g = XMLCTRL(*m_Dlg, "progress", wxGauge);
    g->SetValue(g->GetValue() + increment);
}



void ProgressInfo::UpdateMessage(const wxString& text)
{
    XMLCTRL(*m_Dlg, "info", wxStaticText)->SetLabel(text);
    m_Dlg->Refresh();
    wxYield();
}
            
