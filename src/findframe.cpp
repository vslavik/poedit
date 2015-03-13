/*
 *  This file is part of Poedit (http://poedit.net)
 *
 *  Copyright (C) 2001-2015 Vaclav Slavik
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

#include <wx/accel.h>
#include <wx/choice.h>
#include <wx/collpane.h>
#include <wx/config.h>
#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/checkbox.h>

#ifdef __WXOSX__
#include <AppKit/AppKit.h>
#endif

#include "catalog.h"
#include "text_control.h"
#include "edframe.h"
#include "edlistctrl.h"
#include "findframe.h"
#include "hidpi.h"
#include "utility.h"

namespace
{

// The word separators used when doing a "Whole words only" search
// FIXME-ICU: use ICU to separate words
const wxString SEPARATORS = wxT(" \t\r\n\\/:;.,?!\"'_|-+=(){}[]<>&#@");

enum
{
    Mode_Find,
    Mode_Replace
};

} // anonymous namespace


wxString FindFrame::ms_text;

FindFrame::FindFrame(PoeditFrame *owner,
                     PoeditListCtrl *list,
                     const CatalogPtr& c,
                     CustomizedTextCtrl *textCtrlOrig,
                     CustomizedTextCtrl *textCtrlTrans)
        : wxDialog(owner, wxID_ANY, _("Find")),
          m_owner(owner),
          m_listCtrl(list),
          m_catalog(c),
          m_position(-1),
          m_textCtrlOrig(textCtrlOrig),
          m_textCtrlTrans(textCtrlTrans)
{
    wxBoxSizer *topsizer = new wxBoxSizer(wxVERTICAL);

    wxBoxSizer *sizer = new wxBoxSizer(wxVERTICAL);
    topsizer->Add(sizer, wxSizerFlags(1).Expand().PXDoubleBorderAll());

    auto entrySizer = new wxFlexGridSizer(2, wxSize(MSW_OR_OTHER(PX(5), PX(10)), PX(5)));
    m_mode = new wxChoice(this, wxID_ANY);
#ifdef __WXOSX__
    [(NSPopUpButton*)m_mode->GetHandle() setBordered:NO];
#endif
    m_mode->Append(_("Find"));
    m_mode->Append(_("Replace"));
    m_mode->SetSelection(Mode_Find);

    m_searchField = new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition, wxSize(PX(400), -1));
    m_searchField->SetHint(_("String to find"));
    m_replaceField = new wxTextCtrl(this, wxID_ANY);
    m_replaceField->SetHint(_("Replacement string"));

    entrySizer->Add(m_mode);
    entrySizer->Add(m_searchField, wxSizerFlags(1).Expand());
    entrySizer->AddSpacer(1);
    entrySizer->Add(m_replaceField, wxSizerFlags(1).Expand());
    sizer->Add(entrySizer, wxSizerFlags().Expand().PXBorderAll());

#ifdef __WXMSW__
    #define collPane this
#else
    // TRANSLATORS: Expander in Find window for additional options (case sensitive etc.)
    auto coll = new wxCollapsiblePane(this, wxID_ANY, _("Options"));
    auto collPane = coll->GetPane();
#endif

    m_ignoreCase = new wxCheckBox(collPane, wxID_ANY, _("Ignore case"));
    m_wrapAround = new wxCheckBox(collPane, wxID_ANY, _("Wrap around"));
    m_wholeWords = new wxCheckBox(collPane, wxID_ANY, _("Whole words only"));
    m_findInOrig = new wxCheckBox(collPane, wxID_ANY, _("Find in source texts"));
    m_findInTrans = new wxCheckBox(collPane, wxID_ANY, _("Find in translations"));
    m_findInComments = new wxCheckBox(collPane, wxID_ANY, _("Find in comments"));

    wxBoxSizer *options = new wxBoxSizer(wxHORIZONTAL);
    wxBoxSizer *optionsL = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer *optionsR = new wxBoxSizer(wxVERTICAL);
    options->Add(optionsL, wxSizerFlags(1).Expand().PXBorder(wxRIGHT));
    options->Add(optionsR, wxSizerFlags(1).Expand());
    optionsL->Add(m_ignoreCase, wxSizerFlags().Expand());
    optionsL->Add(m_wrapAround, wxSizerFlags().Expand().Border(wxTOP, PX(2)));
    optionsL->Add(m_wholeWords, wxSizerFlags().Expand().Border(wxTOP, PX(2)));
    optionsR->Add(m_findInOrig, wxSizerFlags().Expand().Border(wxTOP, PX(2)));
    optionsR->Add(m_findInTrans, wxSizerFlags().Expand().Border(wxTOP, PX(2)));
    optionsR->Add(m_findInComments, wxSizerFlags().Expand().Border(wxTOP, PX(2)));

#ifdef __WXMSW__
    sizer->Add(options, wxSizerFlags().Expand().PXBorderAll());
#else
    collPane->SetSizer(options);
    sizer->Add(coll, wxSizerFlags().Expand().PXBorderAll());
#endif

    m_btnClose = new wxButton(this, wxID_CLOSE, _("Close"));
    m_btnReplaceAll = new wxButton(this, wxID_ANY, MSW_OR_OTHER(_("Replace all"), _("Replace All")));
    m_btnReplace = new wxButton(this, wxID_ANY, _("Replace"));
    m_btnPrev = new wxButton(this, wxID_ANY, _("< &Previous"));
    m_btnNext = new wxButton(this, wxID_ANY, _("&Next >"));
    m_btnNext->SetDefault();

    wxBoxSizer *buttons = new wxBoxSizer(wxHORIZONTAL);
    sizer->Add(buttons, wxSizerFlags().Expand().PXBorderAll());
    buttons->Add(m_btnClose, wxSizerFlags().PXBorder(wxRIGHT));
    buttons->AddStretchSpacer();
    buttons->Add(m_btnReplaceAll, wxSizerFlags().PXBorder(wxRIGHT));
    buttons->Add(m_btnReplace, wxSizerFlags().PXBorder(wxRIGHT));
    buttons->Add(m_btnPrev, wxSizerFlags().PXBorder(wxRIGHT));
    buttons->Add(m_btnNext, wxSizerFlags());

    SetSizerAndFit(topsizer);

    SetEscapeId(wxID_CLOSE);

    RestoreWindowState(this, wxDefaultSize, WinState_Pos);

    if ( !ms_text.empty() )
    {
        m_searchField->SetValue(ms_text);
        m_searchField->SelectAll();
    }

    Reset(c);

    m_findInOrig->SetValue(wxConfig::Get()->ReadBool("find_in_orig", true));
    m_findInTrans->SetValue(wxConfig::Get()->ReadBool("find_in_trans", true));
    m_findInComments->SetValue(wxConfig::Get()->ReadBool("find_in_comments", true));
    m_ignoreCase->SetValue(!wxConfig::Get()->ReadBool("find_case_sensitive", false));
    m_wrapAround->SetValue(wxConfig::Get()->ReadBool("find_wrap_around", true));
    m_wholeWords->SetValue(wxConfig::Get()->ReadBool("whole_words", false));

#ifdef __WXOSX__
    wxAcceleratorEntry entries[] = {
        { wxACCEL_CMD,  'W', wxID_CLOSE }
    };
    wxAcceleratorTable accel(WXSIZEOF(entries), entries);
    SetAcceleratorTable(accel);
#endif

    m_searchField->Bind(wxEVT_TEXT, &FindFrame::OnTextChange, this);
    m_btnPrev->Bind(wxEVT_BUTTON, &FindFrame::OnPrev, this);
    m_btnNext->Bind(wxEVT_BUTTON, &FindFrame::OnNext, this);
    Bind(wxEVT_BUTTON, &FindFrame::OnClose, this, wxID_CLOSE);
    Bind(wxEVT_CHECKBOX, &FindFrame::OnCheckbox, this);

    OnModeChanged();
    m_mode->Bind(wxEVT_CHOICE, [=](wxCommandEvent&){ OnModeChanged(); });

    m_btnReplace->Bind(wxEVT_BUTTON, &FindFrame::OnReplace, this);
    m_btnReplaceAll->Bind(wxEVT_BUTTON, &FindFrame::OnReplaceAll, this);
    m_btnReplace->Bind(wxEVT_UPDATE_UI, [=](wxUpdateUIEvent& e){ e.Enable((bool)m_lastItem); });
    m_btnReplaceAll->Bind(wxEVT_UPDATE_UI, [=](wxUpdateUIEvent& e){ e.Enable(!ms_text.empty()); });
}


FindFrame::~FindFrame()
{
    SaveWindowState(this, WinState_Pos);
}


void FindFrame::Reset(const CatalogPtr& c)
{
    if (!m_listCtrl)
        return;

    m_catalog = c;
    m_position = -1;

    m_btnPrev->Enable(!ms_text.empty());
    m_btnNext->Enable(!ms_text.empty());
}


void FindFrame::ShowForFind()
{
    DoShowFor(Mode_Find);
}

void FindFrame::ShowForReplace()
{
    DoShowFor(Mode_Replace);
}

void FindFrame::DoShowFor(int mode)
{
    m_mode->SetSelection(mode);
    OnModeChanged();

    Show(true);
    Raise();

    m_searchField->SetFocus();
    m_searchField->SelectAll();
}


void FindFrame::OnClose(wxCommandEvent&)
{
    Destroy();
}


void FindFrame::OnModeChanged()
{
    bool isReplace = m_mode->GetSelection() == Mode_Replace;

    SetTitle(isReplace ? _("Replace") : _("Find"));

    m_btnReplace->Show(isReplace);
    m_btnReplaceAll->Show(isReplace);
    m_replaceField->GetContainingSizer()->Show(m_replaceField, isReplace);

    m_findInOrig->Enable(!isReplace);
    m_findInTrans->Enable(!isReplace);
    m_findInComments->Enable(!isReplace);
    m_ignoreCase->Enable(!isReplace);

    Layout();
    GetSizer()->SetSizeHints(this);
}


void FindFrame::OnTextChange(wxCommandEvent&)
{
    ms_text = m_searchField->GetValue();
    m_lastItem.reset();

    Reset(m_catalog);
}


void FindFrame::OnCheckbox(wxCommandEvent&)
{
    wxConfig::Get()->Write("find_in_orig", m_findInOrig->GetValue());
    wxConfig::Get()->Write("find_in_trans", m_findInTrans->GetValue());
    wxConfig::Get()->Write("find_in_comments", m_findInComments->GetValue());
    wxConfig::Get()->Write("find_case_sensitive", !m_ignoreCase->GetValue());
    wxConfig::Get()->Write("find_wrap_around", m_wrapAround->GetValue());
    wxConfig::Get()->Write("whole_words", m_wholeWords->GetValue());
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


namespace
{

template<typename S, typename F>
bool FindTextInStringAndDo(S& str, const wxString& text, bool wholeWords, F&& handler)
{
    auto textLen = text.Length();

    bool found = false;
    size_t start = 0;
    while (start != wxString::npos)
    {
        auto index = str.find(text, start);
        if (index == wxString::npos)
            break;

        if (wholeWords)
        {
            bool result = true;
            if (index >0)
                result = result && SEPARATORS.Contains(str[index-1]);
            if (index+textLen < str.Length())
                result = result && SEPARATORS.Contains(str[index+textLen]);

            if (!result)
            {
                start = index + textLen;
                continue;
            }
        }

        found = true;
        start = handler(str, index, textLen);
    }

    return found;
}

bool IsTextInString(const wxString& str, const wxString& text, bool wholeWords)
{
    return FindTextInStringAndDo(str, text, wholeWords,
                                 [=](const wxString&,size_t,size_t){ return wxString::npos;/*just 1 hit*/ });
}

bool ReplaceTextInString(wxString& str, const wxString& text, bool wholeWords, const wxString& replacement)
{
    return FindTextInStringAndDo(str, text, wholeWords,
                                 [=](wxString& s, size_t pos, size_t len){
                                     s.replace(pos, len, replacement);
                                     return pos + replacement.length();
                                 });
}

enum FoundState
{
    Found_Not = 0,
    Found_InOrig,
    Found_InTrans,
    Found_InComments,
    Found_InExtractedComments
};

} // anonymous space

bool FindFrame::DoFind(int dir)
{
    wxASSERT( dir == +1 || dir == -1 );

    if (!m_listCtrl)
        return false;

    int mode = m_mode->GetSelection();
    int cnt = m_listCtrl->GetItemCount();
    bool inTrans = m_findInTrans->GetValue();
    bool inSource = (mode == Mode_Find) && m_findInOrig->GetValue();
    bool inComments = (mode == Mode_Find) && m_findInComments->GetValue();
    bool ignoreCase = (mode == Mode_Find) && m_ignoreCase->GetValue();
    bool wholeWords = m_wholeWords->GetValue();
    bool wrapAround = m_wrapAround->GetValue();
    int posOrig = m_position;

    FoundState found = Found_Not;
    CatalogItemPtr lastItem;

    wxString textc;
    wxString text(ms_text);

    if (ignoreCase)
        text.MakeLower();

    // Only ignore mnemonics when searching if the text being searched for
    // doesn't contain them. That's a reasonable heuristics: most of the time,
    // ignoring them is the right thing to do and provides better results. But
    // sometimes, people want to search for them.
    const bool ignoreMnemonicsAmp = (mode == Mode_Find) && (text.Find(_T('&')) == wxNOT_FOUND);
    const bool ignoreMnemonicsUnderscore = (mode == Mode_Find) && (text.Find(_T('_')) == wxNOT_FOUND);

    int oldPosition = m_position;
    m_position = oldPosition + dir;
    for (int tested = 0; tested < cnt; ++tested, m_position += dir)
    {
        if (m_position < 0)
        {
            if (wrapAround)
                m_position += cnt;
            else
                break;
        }
        else if (m_position >= cnt)
        {
            if (wrapAround)
                m_position -= cnt;
            else
                break;
        }

        auto dt = lastItem = (*m_catalog)[m_listCtrl->ListIndexToCatalog(m_position)];

        if (inTrans)
        {
            // concatenate all translations:
            unsigned cntTrans = dt->GetNumberOfTranslations();
            textc = wxEmptyString;
            for (unsigned i = 0; i < cntTrans; i++)
            {
                textc += dt->GetTranslation(i);
            }
            // and search for the substring in them:
            if (ignoreCase)
                textc.MakeLower();
            if (ignoreMnemonicsAmp)
                textc.Replace("&", "");
            if (ignoreMnemonicsUnderscore)
                textc.Replace("_", "");

            if (IsTextInString(textc, text, wholeWords))
            {
                found = Found_InTrans;
                break;
            }
        }
        if (inSource)
        {
            textc = dt->GetString();
            if (ignoreCase)
                textc.MakeLower();
            if (ignoreMnemonicsAmp)
                textc.Replace("&", "");
            if (ignoreMnemonicsUnderscore)
                textc.Replace("_", "");
            if (IsTextInString(textc, text, wholeWords))
            {
                found = Found_InOrig;
                break;
            }
        }
        if (inComments)
        {
            textc = dt->GetComment();
            if (ignoreCase)
                textc.MakeLower();

            if (IsTextInString(textc, text, wholeWords))
            {
                found = Found_InComments;
                break;
            }

            wxArrayString extractedComments = dt->GetExtractedComments();
            textc = wxEmptyString;
            for (unsigned i = 0; i < extractedComments.GetCount(); i++)
                textc += extractedComments[i];

            if (ignoreCase)
                textc.MakeLower();

            if (IsTextInString(textc, text, wholeWords))
            {
                found = Found_InExtractedComments;
                break;
            }
        }
    }

    if (found != Found_Not)
    {
        m_lastItem = lastItem;

        m_listCtrl->EnsureVisible(m_position);
        m_listCtrl->SelectAndFocus(m_position);

        // find the text on the control and select it:

        CustomizedTextCtrl* txt = nullptr;
        switch (found)
        {
            case Found_InOrig:
              txt = m_textCtrlOrig;
              break;
            case Found_InTrans:
              txt = m_textCtrlTrans;
              break;
            case Found_InComments:
            case Found_InExtractedComments:
            case Found_Not:
              break;
        }

        if (txt)
        {
            textc = txt->GetValue();
            if (ignoreCase)
                textc.MakeLower();
            int pos = textc.Find(text);
            if (pos != wxNOT_FOUND)
                txt->ShowFindIndicator(pos, (int)text.length());
        }

        return true;
    }

    m_position = posOrig;
    return false;
}

bool FindFrame::DoReplaceInItem(CatalogItemPtr item)
{
    bool wholeWords = m_wholeWords->GetValue();
    auto search = m_searchField->GetValue();
    auto replace = m_replaceField->GetValue();

    bool replaced = false;
    for (auto& t: item->GetTranslations())
    {
        if (ReplaceTextInString(t, search, wholeWords, replace))
            replaced = true;
    }

    if (replaced && item == m_owner->GetCurrentItem())
        m_owner->UpdateToTextCtrl(PoeditFrame::UndoableEdit);

    return replaced;
}

void FindFrame::OnReplace(wxCommandEvent&)
{
    DoReplaceInItem(m_lastItem);
    m_listCtrl->Refresh();
}

void FindFrame::OnReplaceAll(wxCommandEvent&)
{
    for (auto& item: m_catalog->items())
        DoReplaceInItem(item);
    m_listCtrl->Refresh();
}
