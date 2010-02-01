/*
 *  This file is part of Poedit (http://www.poedit.net)
 *
 *  Copyright (C) 2001-2008 Vaclav Slavik
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
 *  Search frame
 *
 */

#include <wx/wxprec.h>

#include <wx/xrc/xmlres.h>
#include <wx/config.h>
#include <wx/button.h>
#include <wx/textctrl.h>
#include <wx/checkbox.h>

#include "catalog.h"
#include "findframe.h"
#include "edlistctrl.h"

// The word separators used when doing a "Whole words only" search
// FIXME-ICU: use ICU to separate words
static const wxString SEPARATORS = wxT(" \t\r\n\\/:;.,?!\"'_|-+=(){}[]<>&#@");

wxString FindFrame::ms_text;

BEGIN_EVENT_TABLE(FindFrame, wxDialog)
   EVT_BUTTON(XRCID("find_next"), FindFrame::OnNext)
   EVT_BUTTON(XRCID("find_prev"), FindFrame::OnPrev)
   EVT_BUTTON(wxID_CLOSE, FindFrame::OnClose)
   EVT_TEXT(XRCID("string_to_find"), FindFrame::OnTextChange)
   EVT_CHECKBOX(-1, FindFrame::OnCheckbox)
END_EVENT_TABLE()

FindFrame::FindFrame(wxWindow *parent,
                     PoeditListCtrl *list,
                     Catalog *c,
                     wxTextCtrl *textCtrlOrig,
                     wxTextCtrl *textCtrlTrans,
                     wxTextCtrl *textCtrlComments,
                     wxTextCtrl *textCtrlAutoComments)
        : m_listCtrl(list),
          m_catalog(c),
          m_position(-1),
          m_textCtrlOrig(textCtrlOrig),
          m_textCtrlTrans(textCtrlTrans),
          m_textCtrlComments(textCtrlComments),
          m_textCtrlAutoComments(textCtrlAutoComments)
{
    wxXmlResource::Get()->LoadDialog(this, parent, _T("find_frame"));

    SetEscapeId(wxID_CLOSE);

#ifndef __WXGTK__
    wxPoint p(wxConfig::Get()->Read(_T("find_pos_x"), -1),
              wxConfig::Get()->Read(_T("find_pos_y"), -1));
    if (p.x != -1)
        Move(p);
#endif

    m_btnNext = XRCCTRL(*this, "find_next", wxButton);
    m_btnPrev = XRCCTRL(*this, "find_prev", wxButton);

    if ( !ms_text.empty() )
    {
        wxTextCtrl *t = XRCCTRL(*this, "string_to_find", wxTextCtrl);
        t->SetValue(ms_text);
        t->SelectAll();
    }

    Reset(c);

    XRCCTRL(*this, "in_orig", wxCheckBox)->SetValue(
        wxConfig::Get()->Read(_T("find_in_orig"), (long)true));
    XRCCTRL(*this, "in_trans", wxCheckBox)->SetValue(
        wxConfig::Get()->Read(_T("find_in_trans"), (long)true));
    XRCCTRL(*this, "in_comments", wxCheckBox)->SetValue(
        wxConfig::Get()->Read(_T("find_in_comments"), (long)true));
    XRCCTRL(*this, "in_auto_comments", wxCheckBox)->SetValue(
        wxConfig::Get()->Read(_T("find_in_auto_comments"), (long)true));
    XRCCTRL(*this, "case_sensitive", wxCheckBox)->SetValue(
        wxConfig::Get()->Read(_T("find_case_sensitive"), (long)false));
    XRCCTRL(*this, "from_first", wxCheckBox)->SetValue(
        wxConfig::Get()->Read(_T("find_from_first"), (long)true));
    XRCCTRL(*this, "whole_words", wxCheckBox)->SetValue(
        wxConfig::Get()->Read(_T("whole_words"), (long)false));
}


FindFrame::~FindFrame()
{
    wxConfig::Get()->Write(_T("find_pos_x"), (long)GetPosition().x);
    wxConfig::Get()->Write(_T("find_pos_y"), (long)GetPosition().y);

    wxConfig::Get()->Write(_T("find_in_orig"),
            XRCCTRL(*this, "in_orig", wxCheckBox)->GetValue());
    wxConfig::Get()->Write(_T("find_in_trans"),
                XRCCTRL(*this, "in_trans", wxCheckBox)->GetValue());
    wxConfig::Get()->Write(_T("find_in_comments"),
                XRCCTRL(*this, "in_comments", wxCheckBox)->GetValue());
    wxConfig::Get()->Write(_T("find_in_auto_comments"),
                XRCCTRL(*this, "in_auto_comments", wxCheckBox)->GetValue());
    wxConfig::Get()->Write(_T("find_case_sensitive"),
                XRCCTRL(*this, "case_sensitive", wxCheckBox)->GetValue());
    wxConfig::Get()->Write(_T("find_from_first"),
                XRCCTRL(*this, "from_first", wxCheckBox)->GetValue());
    wxConfig::Get()->Write(_T("whole_words"),
                XRCCTRL(*this, "whole_words", wxCheckBox)->GetValue());
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

    m_btnPrev->Enable(!ms_text.empty());
    m_btnNext->Enable(!ms_text.empty());
}


void FindFrame::OnClose(wxCommandEvent &event)
{
    Destroy();
}


void FindFrame::OnTextChange(wxCommandEvent &event)
{
    ms_text = XRCCTRL(*this, "string_to_find", wxTextCtrl)->GetValue();

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
    Found_InTrans,
    Found_InComments,
    Found_InAutoComments
};

bool TextInString(const wxString& str, const wxString& text, bool wholeWords)
{
    int index = str.Find(text);
    if (index >= 0)
    {
        if (wholeWords)
        {
            unsigned textLen = text.Length();

            bool result = true;
            if (index >0)
                result = result && SEPARATORS.Contains(str[index-1]);
            if (index+textLen < str.Length())
                result = result && SEPARATORS.Contains(str[index+textLen]);

            return result;
        }
        else
        {
            return true;
        }
    }
    else
    {
        return false;
    }
}

bool FindFrame::DoFind(int dir)
{
    int cnt = m_listCtrl->GetItemCount();
    bool inStr = XRCCTRL(*this, "in_orig", wxCheckBox)->GetValue();
    bool inTrans = XRCCTRL(*this, "in_trans", wxCheckBox)->GetValue();
    bool inComments = XRCCTRL(*this, "in_comments", wxCheckBox)->GetValue();
    bool inAutoComments = XRCCTRL(*this, "in_auto_comments", wxCheckBox)->GetValue();
    bool caseSens = XRCCTRL(*this, "case_sensitive", wxCheckBox)->GetValue();
    bool wholeWords = XRCCTRL(*this, "whole_words", wxCheckBox)->GetValue();
    int posOrig = m_position;

    FoundState found = Found_Not;
    wxString textc;
    wxString text(ms_text);

    if (!caseSens)
        text.MakeLower();

    m_position += dir;
    while (m_position >= 0 && m_position < cnt)
    {
        CatalogItem &dt = (*m_catalog)[m_listCtrl->GetIndexInCatalog(m_position)];

        if (inStr)
        {
            textc = dt.GetString();
            if (!caseSens)
                textc.MakeLower();
            if (TextInString(textc, text, wholeWords))
            {
                found = Found_InOrig;
                break;
            }
        }
        if (inTrans)
        {
            // concatenate all translations:
            size_t cnt = dt.GetNumberOfTranslations();
            textc = wxEmptyString;
            for (size_t i = 0; i < cnt; i++)
            {
                textc += dt.GetTranslation(i);
            }
            // and search for the substring in them:
            if (!caseSens)
                textc.MakeLower();

            if (TextInString(textc, text, wholeWords)) { found = Found_InTrans; break; }
        }
        if (inComments)
        {
            textc = dt.GetComment();
            if (!caseSens)
                textc.MakeLower();

            if (TextInString(textc, text, wholeWords)) { found = Found_InComments; break; }
        }
        if (inAutoComments)
        {
            wxArrayString autoComments = dt.GetAutoComments();
            textc = wxEmptyString;
            for (unsigned i = 0; i < autoComments.GetCount(); i++)
                textc += autoComments[i];

            if (!caseSens)
                textc.MakeLower();

            if (TextInString(textc, text, wholeWords)) { found = Found_InAutoComments; break; }
        }

        m_position += dir;
    }

    if (found != Found_Not)
    {
        m_listCtrl->EnsureVisible(m_position);
#ifdef __WXMAC__
        m_listCtrl->Refresh();
#endif
        m_listCtrl->Select(m_position);
        m_listCtrl->SetItemState(m_position,
                    wxLIST_STATE_FOCUSED, wxLIST_STATE_FOCUSED);

        // find the text on the control and select it:

        wxTextCtrl* txt;
        switch (found)
        {
            case Found_InOrig:
              txt = m_textCtrlOrig;
              break;
            case Found_InTrans:
              txt = m_textCtrlTrans;
              break;
            case Found_InComments:
              txt = m_textCtrlComments;
              break;
            case Found_InAutoComments:
              txt = m_textCtrlAutoComments;
              break;
            case Found_Not: // silence compiler warning, can't get here
              break;
        }

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
