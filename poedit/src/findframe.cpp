/*

    poedit, a wxWindows i18n catalogs editor

    ---------------
      findframe.cpp
    
      Find frame
    
      (c) Vaclav Slavik, 2001

*/

#include <wx/wxprec.h>

#include <wx/xrc/xmlres.h>
#include <wx/config.h>
#include <wx/button.h>
#include <wx/textctrl.h>
#include <wx/listctrl.h>
#include <wx/checkbox.h>

#include "catalog.h"
#include "findframe.h"


BEGIN_EVENT_TABLE(FindFrame, wxDialog)
   EVT_BUTTON(XRCID("find_next"), FindFrame::OnNext)
   EVT_BUTTON(XRCID("find_prev"), FindFrame::OnPrev)
   EVT_BUTTON(wxID_CANCEL, FindFrame::OnCancel)
   EVT_TEXT(XRCID("string_to_find"), FindFrame::OnTextChange)
   EVT_CHECKBOX(-1, FindFrame::OnCheckbox)
   EVT_CLOSE(FindFrame::OnClose)
END_EVENT_TABLE()

FindFrame::FindFrame(wxWindow *parent, wxListCtrl *list, Catalog *c,
                     wxTextCtrl *textCtrlOrig, wxTextCtrl *textCtrlTrans)
        : m_listCtrl(list), m_catalog(c), m_position(-1),
          m_textCtrlOrig(textCtrlOrig), m_textCtrlTrans(textCtrlTrans)
{
    wxPoint p(wxConfig::Get()->Read(_T("find_pos_x"), -1),
              wxConfig::Get()->Read(_T("find_pos_y"), -1));

    wxXmlResource::Get()->LoadDialog(this, parent, _T("find_frame"));
    if (p.x != -1) 
        Move(p);
        
    m_btnNext = XRCCTRL(*this, "find_next", wxButton);
    m_btnPrev = XRCCTRL(*this, "find_prev", wxButton);
    
    Reset(c);

    XRCCTRL(*this, "in_orig", wxCheckBox)->SetValue(
        wxConfig::Get()->Read(_T("find_in_orig"), (long)true));
    XRCCTRL(*this, "in_trans", wxCheckBox)->SetValue(
        wxConfig::Get()->Read(_T("find_in_trans"), (long)true));
    XRCCTRL(*this, "case_sensitive", wxCheckBox)->SetValue(
        wxConfig::Get()->Read(_T("find_case_sensitive"), (long)false));
    XRCCTRL(*this, "from_first", wxCheckBox)->SetValue(
        wxConfig::Get()->Read(_T("find_from_first"), (long)true));
}


FindFrame::~FindFrame()
{
    wxConfig::Get()->Write(_T("find_pos_x"), (long)GetPosition().x);
    wxConfig::Get()->Write(_T("find_pos_y"), (long)GetPosition().y);

    wxConfig::Get()->Write(_T("find_in_orig"),
            XRCCTRL(*this, "in_orig", wxCheckBox)->GetValue());
    wxConfig::Get()->Write(_T("find_in_trans"),
                XRCCTRL(*this, "in_trans", wxCheckBox)->GetValue());
    wxConfig::Get()->Write(_T("find_case_sensitive"),
                XRCCTRL(*this, "case_sensitive", wxCheckBox)->GetValue());
    wxConfig::Get()->Write(_T("find_from_first"),
                XRCCTRL(*this, "from_first", wxCheckBox)->GetValue());
}


void FindFrame::Reset(Catalog *c)
{
    bool fromFirst = XRCCTRL(*this, "from_first", wxCheckBox)->GetValue();

    m_catalog = c;
    m_position = -1;
    if (!fromFirst)
        m_position = m_listCtrl->GetNextItem(-1,
                                     wxLIST_NEXT_ALL,
                                     wxLIST_STATE_SELECTED);
    
    m_btnPrev->Enable(!!m_text);
    m_btnNext->Enable(!!m_text);
}


void FindFrame::OnClose(wxCloseEvent &event)
{
    Destroy();
}

void FindFrame::OnCancel(wxCommandEvent &event)
{
    Destroy();
}


void FindFrame::OnTextChange(wxCommandEvent &event)
{
    m_text = XRCCTRL(*this, "string_to_find", wxTextCtrl)->GetValue();
    Reset(m_catalog);
}


void FindFrame::OnCheckbox(wxCommandEvent &event)
{
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


enum FoundState
{
    Found_Not = 0,
    Found_InOrig,
    Found_InTrans
};

bool FindFrame::DoFind(int dir)
{
    int cnt = m_listCtrl->GetItemCount();
    bool inStr = XRCCTRL(*this, "in_orig", wxCheckBox)->GetValue();
    bool inTrans = XRCCTRL(*this, "in_trans", wxCheckBox)->GetValue();
    bool caseSens = XRCCTRL(*this, "case_sensitive", wxCheckBox)->GetValue();
    int posOrig = m_position;

    FoundState found = Found_Not;
    wxString textc;
    wxString text(m_text);

    if (!caseSens)
        text.MakeLower();
        
    m_position += dir;
    while (m_position >= 0 && m_position < cnt)
    {
        CatalogData &dt = (*m_catalog)[m_listCtrl->GetItemData(m_position)];
        
        if (inStr)
        {
            #if wxUSE_UNICODE
            textc = dt.GetString();
            #else
            textc = wxString(dt.GetString().wc_str(wxConvUTF8), wxConvLocal);
            #endif
            if (!caseSens)
                textc.MakeLower();
            if (textc.Contains(text)) { found = Found_InOrig; break; }
        }
        if (inTrans)
        {
            #if wxUSE_UNICODE
            textc = dt.GetTranslation();
            #else
            textc = wxString(dt.GetTranslation().wc_str(wxConvUTF8), wxConvLocal);
            #endif
            if (!caseSens)
                textc.MakeLower();

            if (textc.Contains(text)) { found = Found_InTrans; break; }
        }

        m_position += dir;
    }

    if (found != Found_Not)
    {
        m_listCtrl->SetItemState(m_position, 
                    wxLIST_STATE_FOCUSED, wxLIST_STATE_FOCUSED);
        m_listCtrl->SetItemState(m_position, 
                    wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
        m_listCtrl->EnsureVisible(m_position);

        // find the text on the control and select it:

        wxTextCtrl *txt =
            (found == Found_InOrig) ? m_textCtrlOrig : m_textCtrlTrans;

        textc = txt->GetValue();
        if (!caseSens)
            textc.MakeLower();
        int pos = textc.Find(text);
        if (pos != wxNOT_FOUND)
            txt->SetSelection(pos, pos + text.length());
        
        return true;
    }
    
    m_position = posOrig;
    return false;
}
