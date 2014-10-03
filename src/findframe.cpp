/*
 *  This file is part of Poedit (http://poedit.net)
 *
 *  Copyright (C) 2001-2014 Vaclav Slavik
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

#include <wx/xrc/xmlres.h>
#include <wx/accel.h>
#include <wx/config.h>
#include <wx/button.h>
#include <wx/textctrl.h>
#include <wx/checkbox.h>

#include "catalog.h"
#include "findframe.h"
#include "edlistctrl.h"
#include "utility.h"

// The word separators used when doing a "Whole words only" search
// FIXME-ICU: use ICU to separate words
static const wxString SEPARATORS = wxT(" \t\r\n\\/:;.,?!\"'_|-+=(){}[]<>&#@");

wxString FindFrame::ms_text;
FindFrame *FindFrame::ms_singleton = nullptr;

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
                     wxTextCtrl *textCtrlTrans)
        : m_listCtrl(list),
          m_catalog(c),
          m_position(-1),
          m_textCtrlOrig(textCtrlOrig),
          m_textCtrlTrans(textCtrlTrans)
{
    wxXmlResource::Get()->LoadDialog(this, parent, "find_frame");

    m_textField = XRCCTRL(*this, "string_to_find", wxTextCtrl);

    SetEscapeId(wxID_CLOSE);

    RestoreWindowState(this, wxDefaultSize, WinState_Pos);

    m_btnNext = XRCCTRL(*this, "find_next", wxButton);
    m_btnPrev = XRCCTRL(*this, "find_prev", wxButton);

    if ( !ms_text.empty() )
    {
        m_textField->SetValue(ms_text);
        m_textField->SelectAll();
    }

    Reset(c);

    XRCCTRL(*this, "in_orig", wxCheckBox)->SetValue(
        wxConfig::Get()->ReadBool("find_in_orig", true));
    XRCCTRL(*this, "in_trans", wxCheckBox)->SetValue(
        wxConfig::Get()->ReadBool("find_in_trans", true));
    XRCCTRL(*this, "in_comments", wxCheckBox)->SetValue(
        wxConfig::Get()->ReadBool("find_in_comments", true));
    XRCCTRL(*this, "in_auto_comments", wxCheckBox)->SetValue(
        wxConfig::Get()->ReadBool("find_in_auto_comments", true));
    XRCCTRL(*this, "case_sensitive", wxCheckBox)->SetValue(
        wxConfig::Get()->ReadBool("find_case_sensitive", false));
    XRCCTRL(*this, "from_first", wxCheckBox)->SetValue(
        wxConfig::Get()->ReadBool("find_from_first", true));
    XRCCTRL(*this, "whole_words", wxCheckBox)->SetValue(
        wxConfig::Get()->ReadBool("whole_words", false));

#ifdef __WXOSX__
    wxAcceleratorEntry entries[] = {
        { wxACCEL_CMD,  'W', wxID_CLOSE }
    };
    wxAcceleratorTable accel(WXSIZEOF(entries), entries);
    SetAcceleratorTable(accel);
#endif

    ms_singleton = this;
}


FindFrame::~FindFrame()
{
    ms_singleton = nullptr;

    SaveWindowState(this, WinState_Pos);

    wxConfig::Get()->Write("find_in_orig",
            XRCCTRL(*this, "in_orig", wxCheckBox)->GetValue());
    wxConfig::Get()->Write("find_in_trans",
                XRCCTRL(*this, "in_trans", wxCheckBox)->GetValue());
    wxConfig::Get()->Write("find_in_comments",
                XRCCTRL(*this, "in_comments", wxCheckBox)->GetValue());
    wxConfig::Get()->Write("find_in_auto_comments",
                XRCCTRL(*this, "in_auto_comments", wxCheckBox)->GetValue());
    wxConfig::Get()->Write("find_case_sensitive",
                XRCCTRL(*this, "case_sensitive", wxCheckBox)->GetValue());
    wxConfig::Get()->Write("find_from_first",
                XRCCTRL(*this, "from_first", wxCheckBox)->GetValue());
    wxConfig::Get()->Write("whole_words",
                XRCCTRL(*this, "whole_words", wxCheckBox)->GetValue());
}

FindFrame *FindFrame::Get(PoeditListCtrl *list, Catalog *forCatalog)
{
    if (!ms_singleton)
        return nullptr;
    if (ms_singleton->m_catalog != forCatalog)
    {
        ms_singleton->m_listCtrl = list;
        ms_singleton->Reset(forCatalog);
    }
    return ms_singleton;
}


void FindFrame::NotifyParentDestroyed(PoeditListCtrl *list, Catalog *forCatalog)
{
    if (!ms_singleton)
        return;
    if (ms_singleton->m_catalog == forCatalog || ms_singleton->m_listCtrl == list)
    {
        ms_singleton->Destroy();
        ms_singleton = nullptr;
    }
}



void FindFrame::Reset(Catalog *c)
{
    if (!m_listCtrl)
        return;

    bool fromFirst = XRCCTRL(*this, "from_first", wxCheckBox)->GetValue();

    m_catalog = c;
    m_position = -1;
    if (!fromFirst)
        m_position = (int)m_listCtrl->GetNextItem(-1,
                                                  wxLIST_NEXT_ALL,
                                                  wxLIST_STATE_SELECTED);

    m_btnPrev->Enable(!ms_text.empty());
    m_btnNext->Enable(!ms_text.empty());
}


void FindFrame::FocusSearchField()
{
    m_textField->SetFocus();
    m_textField->SelectAll();
}


void FindFrame::OnClose(wxCommandEvent&)
{
    Destroy();
    ms_singleton = nullptr;
}


void FindFrame::OnTextChange(wxCommandEvent&)
{
    ms_text = m_textField->GetValue();

    Reset(m_catalog);
}


void FindFrame::OnCheckbox(wxCommandEvent&)
{
    Reset(m_catalog);
}


void FindFrame::OnPrev(wxCommandEvent&)
{
    FindPrev();
}

void FindFrame::FindPrev()
{
    if (!DoFind(-1))
        m_btnPrev->Enable(false);
    else
        m_btnNext->Enable(true);
}


void FindFrame::OnNext(wxCommandEvent&)
{
    FindNext();
}

void FindFrame::FindNext()
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
            size_t textLen = text.Length();

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
    if (!m_listCtrl)
        return false;

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

    // Only ignore mnemonics when searching if the text being searched for
    // doesn't contain them. That's a reasonable heuristics: most of the time,
    // ignoring them is the right thing to do and provides better results. But
    // sometimes, people want to search for them.
    const bool ignoreMnemonicsAmp = (text.Find(_T('&')) == wxNOT_FOUND);
    const bool ignoreMnemonicsUnderscore = (text.Find(_T('_')) == wxNOT_FOUND);

    m_position += dir;
    while (m_position >= 0 && m_position < cnt)
    {
        CatalogItem &dt = (*m_catalog)[m_listCtrl->ListIndexToCatalog(m_position)];

        if (inStr)
        {
            textc = dt.GetString();
            if (!caseSens)
                textc.MakeLower();
            if (ignoreMnemonicsAmp)
                textc.Replace("&", "");
            if (ignoreMnemonicsUnderscore)
                textc.Replace("_", "");
            if (TextInString(textc, text, wholeWords))
            {
                found = Found_InOrig;
                break;
            }
        }
        if (inTrans)
        {
            // concatenate all translations:
            unsigned cntTrans = dt.GetNumberOfTranslations();
            textc = wxEmptyString;
            for (unsigned i = 0; i < cntTrans; i++)
            {
                textc += dt.GetTranslation(i);
            }
            // and search for the substring in them:
            if (!caseSens)
                textc.MakeLower();
            if (ignoreMnemonicsAmp)
                textc.Replace("&", "");
            if (ignoreMnemonicsUnderscore)
                textc.Replace("_", "");

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
#ifdef __WXOSX__
        m_listCtrl->Refresh();
#endif
        m_listCtrl->SelectAndFocus(m_position);

        // find the text on the control and select it:

        wxTextCtrl* txt = NULL;
        switch (found)
        {
            case Found_InOrig:
              txt = m_textCtrlOrig;
              break;
            case Found_InTrans:
              txt = m_textCtrlTrans;
              break;
            case Found_InComments:
            case Found_InAutoComments:
            case Found_Not:
              break;
        }

        if (txt)
        {
            textc = txt->GetValue();
            if (!caseSens)
                textc.MakeLower();
            int pos = textc.Find(text);
            if (pos != wxNOT_FOUND)
                txt->SetSelection(pos, pos + text.length());
        }

        return true;
    }

    m_position = posOrig;
    return false;
}
