/*

    poedit, a wxWindows i18n catalogs editor

    ---------------
      findframe.cpp
    
      Find frame
    
      (c) Vaclav Slavik, 2001

*/


#ifdef __GNUG__
#pragma implementation
#endif

#include <wx/wxprec.h>

#include <wx/xml/xmlres.h>
#include <wx/config.h>
#include <wx/button.h>
#include <wx/listctrl.h>
#include <wx/checkbox.h>

#include "catalog.h"
#include "findframe.h"


BEGIN_EVENT_TABLE(FindFrame, wxDialog)
   EVT_BUTTON(XMLID("find_next"), FindFrame::OnNext)
   EVT_BUTTON(XMLID("find_prev"), FindFrame::OnPrev)
   EVT_BUTTON(wxID_CANCEL, FindFrame::OnCancel)
   EVT_TEXT(XMLID("string_to_find"), FindFrame::OnTextChange)
   EVT_CLOSE(FindFrame::OnCancel)
END_EVENT_TABLE()

FindFrame::FindFrame(wxWindow *parent, wxListCtrl *list, Catalog *c)
        : m_listCtrl(list), m_catalog(c), m_position(-1)
{
    wxPoint p(wxConfig::Get()->Read("find_pos_x", -1),
              wxConfig::Get()->Read("find_pos_y", -1));

    wxTheXmlResource->LoadDialog(this, parent, "find_frame");
    if (p.x != -1) 
        Move(p);
        
    m_btnNext = XMLCTRL(*this, "find_next", wxButton);
    m_btnPrev = XMLCTRL(*this, "find_prev", wxButton);

    XMLCTRL(*this, "in_orig", wxCheckBox)->SetValue(
        wxConfig::Get()->Read("find_in_orig", true));
    XMLCTRL(*this, "in_trans", wxCheckBox)->SetValue(
        wxConfig::Get()->Read("find_in_trans", true));
    XMLCTRL(*this, "case_sensitive", wxCheckBox)->SetValue(
        wxConfig::Get()->Read("find_case_sensitive", false));
}


FindFrame::~FindFrame()
{
    wxConfig::Get()->Write("find_pos_x", (long)GetPosition().x);
    wxConfig::Get()->Write("find_pos_y", (long)GetPosition().y);

    wxConfig::Get()->Write("find_in_orig",
            XMLCTRL(*this, "in_orig", wxCheckBox)->GetValue());
    wxConfig::Get()->Write("find_in_trans",
                XMLCTRL(*this, "in_trans", wxCheckBox)->GetValue());
    wxConfig::Get()->Write("find_case_sensitive",
                XMLCTRL(*this, "case_sensitive", wxCheckBox)->GetValue());
}


void FindFrame::Reset(Catalog *c)
{
    m_catalog = c;
    m_position = -1;
    m_btnPrev->Enable(!!m_text);
    m_btnNext->Enable(!!m_text);
}


void FindFrame::OnCancel(wxCommandEvent &event)
{
    Destroy();
}


void FindFrame::OnTextChange(wxCommandEvent &event)
{
    m_text = XMLCTRL(*this, "string_to_find", wxTextCtrl)->GetValue();
    Reset(m_catalog);
}


void FindFrame::OnPrev(wxCommandEvent &event)
{
    if (!DoFind(-1))
        m_btnPrev->Enable(false);
    else
        m_btnNext->Enable(true);
}


void FindFrame::OnNext(wxCommandEvent &event)
{
    if (!DoFind(+1))
        m_btnNext->Enable(false);
    else
        m_btnPrev->Enable(true);
}


bool FindFrame::DoFind(int dir)
{
    #define CMP_S(s) \
        ((caseSens && s.Contains(text)) || \
         (!caseSens && s.Lower().Contains(text)))
         
    wxString text = wxString(m_text.mb_str(wxConvLocal), wxConvUTF8);
    int cnt = m_listCtrl->GetItemCount();
    bool inStr = XMLCTRL(*this, "in_orig", wxCheckBox)->GetValue();
    bool inTrans = XMLCTRL(*this, "in_trans", wxCheckBox)->GetValue();
    bool caseSens = XMLCTRL(*this, "case_sensitive", wxCheckBox)->GetValue();
    int posOrig = m_position;
    
    if (!caseSens)
        text = text.Lower();
    
    m_position += dir;
    while (m_position >= 0 && m_position < cnt)
    {
        CatalogData &dt = (*m_catalog)[m_listCtrl->GetItemData(m_position)];
        if ((inStr && CMP_S(dt.GetString())) ||
            (inTrans && CMP_S(dt.GetTranslation())))
        {
            m_listCtrl->SetItemState(m_position, 
                        wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
            m_listCtrl->EnsureVisible(m_position);
            return TRUE;
        }
        else        
            m_position += dir;
    }
    m_position = posOrig;
    return FALSE;
    
    #undef CMP_S
}
