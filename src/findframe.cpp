/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 2001-2017 Vaclav Slavik
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
#include <wx/notebook.h>

#ifdef __WXOSX__
#include <AppKit/AppKit.h>
#endif

#ifdef __WXGTK__
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#endif

#include "catalog.h"
#include "text_control.h"
#include "edframe.h"
#include "editing_area.h"
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

const int FRAME_STYLE = (wxDEFAULT_FRAME_STYLE | wxFRAME_TOOL_WINDOW | wxTAB_TRAVERSAL | wxFRAME_FLOAT_ON_PARENT)
                        & ~(wxRESIZE_BORDER | wxMAXIMIZE_BOX);

} // anonymous namespace


wxString FindFrame::ms_text;

FindFrame::FindFrame(PoeditFrame *owner,
                     PoeditListCtrl *list,
                     EditingArea *editingArea,
                     const CatalogPtr& c)
        : wxFrame(owner, wxID_ANY, _("Find"), wxDefaultPosition, wxDefaultSize, FRAME_STYLE),
          m_owner(owner),
          m_listCtrl(list),
          m_editingArea(editingArea),
          m_catalog(c),
          m_position(-1)
{
    auto panel = new wxPanel(this, wxID_ANY);
    wxBoxSizer *panelsizer = new wxBoxSizer(wxVERTICAL);

    wxBoxSizer *sizer = new wxBoxSizer(wxVERTICAL);
    panelsizer->Add(sizer, wxSizerFlags(1).Expand().PXDoubleBorderAll());

    auto entrySizer = new wxFlexGridSizer(2, wxSize(MSW_OR_OTHER(PX(5), PX(10)), PX(5)));
    m_mode = new wxChoice(panel, wxID_ANY);
#ifdef __WXOSX__
    [(NSPopUpButton*)m_mode->GetHandle() setBordered:NO];
#endif
    m_mode->Append(_("Find"));
    m_mode->Append(_("Replace"));
    m_mode->SetSelection(Mode_Find);

    m_searchField = new wxTextCtrl(panel, wxID_ANY, "", wxDefaultPosition, wxSize(PX(400), -1));
    m_replaceField = new wxTextCtrl(panel, wxID_ANY);

    entrySizer->Add(m_mode);
    entrySizer->Add(m_searchField, wxSizerFlags(1).Expand());
    entrySizer->AddSpacer(1);
    entrySizer->Add(m_replaceField, wxSizerFlags(1).Expand());
    sizer->Add(entrySizer, wxSizerFlags().Expand().PXBorderAll());

#ifdef __WXMSW__
    #define collPane panel
#else
    // TRANSLATORS: Expander in Find window for additional options (case sensitive etc.)
    auto coll = new wxCollapsiblePane(panel, wxID_ANY, _("Options"));
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

    m_btnClose = new wxButton(panel, wxID_CLOSE, _("Close"));
    m_btnReplaceAll = new wxButton(panel, wxID_ANY, MSW_OR_OTHER(_("Replace &all"), _("Replace &All")));
    m_btnReplace = new wxButton(panel, wxID_ANY, _("&Replace"));
    m_btnPrev = new wxButton(panel, wxID_ANY, _("< &Previous"));
    m_btnNext = new wxButton(panel, wxID_ANY, _("&Next >"));
    m_btnNext->SetDefault();

    wxBoxSizer *buttons = new wxBoxSizer(wxHORIZONTAL);
    sizer->Add(buttons, wxSizerFlags().Expand().PXBorderAll());
    buttons->Add(m_btnClose, wxSizerFlags().PXBorder(wxRIGHT));
    buttons->AddStretchSpacer();
    buttons->Add(m_btnReplaceAll, wxSizerFlags().PXBorder(wxRIGHT));
    buttons->Add(m_btnReplace, wxSizerFlags().PXBorder(wxRIGHT));
    buttons->Add(m_btnPrev, wxSizerFlags().PXBorder(wxRIGHT));
    buttons->Add(m_btnNext, wxSizerFlags());

    panel->SetSizer(panelsizer);
    auto topsizer = new wxBoxSizer(wxHORIZONTAL);
    topsizer->Add(panel, wxSizerFlags(1).Expand());
    SetSizerAndFit(topsizer);

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

    wxAcceleratorEntry entries[] = {
#ifndef __WXGTK__
        { wxACCEL_SHIFT, WXK_RETURN, m_btnPrev->GetId() },
#endif
#ifdef __WXOSX__
        { wxACCEL_CMD, 'W', wxID_CLOSE },
#endif
        { wxACCEL_NORMAL, WXK_ESCAPE, wxID_CLOSE }
    };
    wxAcceleratorTable accel(WXSIZEOF(entries), entries);
    SetAcceleratorTable(accel);

    m_searchField->Bind(wxEVT_TEXT, &FindFrame::OnTextChange, this);
    m_btnPrev->Bind(wxEVT_BUTTON, &FindFrame::OnPrev, this);
    m_btnNext->Bind(wxEVT_BUTTON, &FindFrame::OnNext, this);
    Bind(wxEVT_BUTTON, &FindFrame::OnClose, this, wxID_CLOSE);
    Bind(wxEVT_MENU, &FindFrame::OnClose, this, wxID_CLOSE);
    Bind(wxEVT_CHECKBOX, &FindFrame::OnCheckbox, this);

    // Set Shift+Return accelerator natively so that the button is animated.
    // (Can't be done on Windows where wxAcceleratorEntry above is used.)
#if defined(__WXGTK__)
    GtkAccelGroup *accelGroup = gtk_accel_group_new();
    gtk_window_add_accel_group(GTK_WINDOW(GetHandle()), accelGroup);
    gtk_widget_add_accelerator(m_btnPrev->GetHandle(), "activate", accelGroup, GDK_KEY_Return, GDK_SHIFT_MASK, GTK_ACCEL_VISIBLE);
#elif defined(__WXOSX__)
    // wx's code interferes with normal processing of Shift-Return and
    // setKeyEquivalent: @"\r" with setKeyEquivalentModifierMask: NSShiftKeyMask
    // wouldn't work. Emulate it in custom code instead, by handling the
    // event originating from the button and from the accelerator table above
    // differently. More than a bit of a hack, but it works.
    NSButton *macPrev = (NSButton*)m_btnPrev->GetHandle();
    Bind(wxEVT_MENU, [=](wxCommandEvent&){ [macPrev performClick:nil]; }, m_btnPrev->GetId());
#endif

    OnModeChanged();
    m_mode->Bind(wxEVT_CHOICE, [=](wxCommandEvent&){ OnModeChanged(); });

    m_btnReplace->Bind(wxEVT_BUTTON, &FindFrame::OnReplace, this);
    m_btnReplaceAll->Bind(wxEVT_BUTTON, &FindFrame::OnReplaceAll, this);
    m_btnReplace->Bind(wxEVT_UPDATE_UI, [=](wxUpdateUIEvent& e){ e.Enable((bool)m_lastItem); });
    m_btnReplaceAll->Bind(wxEVT_UPDATE_UI, [=](wxUpdateUIEvent& e){ e.Enable(!ms_text.empty()); });

    // SetHint() needs to be called *after* binding any event handlers, for
    // compatibility with its generic implementation:
    m_searchField->SetHint(_("String to find"));
    m_replaceField->SetHint(_("Replacement string"));

    // Create hidden, will be shown after setting it up
    Show(false);
}


FindFrame::~FindFrame()
{
    SaveWindowState(this, WinState_Pos);
}


void FindFrame::Reset(const CatalogPtr& c)
{
    m_catalog = c;
    m_position = -1;
    m_lastItem.reset();

    UpdateButtons();
}

void FindFrame::UpdateButtons()
{
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
    m_position = m_listCtrl->GetCurrentItemListIndex();

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

    wxString title = isReplace ? _("Replace") : _("Find");
    if (PoeditFrame::GetOpenWindowsCount() > 1)
    {
        auto filename = m_owner->GetFileNamePartOfTitle();
        if (!filename.empty())
            title += wxString::Format(L" — %s", filename);
    }
    SetTitle(title);

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


void FindFrame::OnTextChange(wxCommandEvent& e)
{
    ms_text = m_searchField->GetValue();
    UpdateButtons();
    e.Skip();
}


void FindFrame::OnCheckbox(wxCommandEvent&)
{
    Reset(m_catalog);
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

bool IsTextInString(wxString str, const wxString& text,
                    bool ignoreCase, bool wholeWords, bool ignoreAmp, bool ignoreUnderscore)
{
    if (ignoreCase)
        str.MakeLower();
    if (ignoreAmp)
        str.Replace("&", "");
    if (ignoreUnderscore)
        str.Replace("_", "");

    return FindTextInStringAndDo(str, text, wholeWords,
                                 [=](const wxString&,size_t,size_t){ return wxString::npos;/*just 1 hit*/ });
}

size_t IsTextInStrings(const wxArrayString& strs, const wxString& text,
                     bool ignoreCase, bool wholeWords, bool ignoreAmp, bool ignoreUnderscore)
{
    // loop through all strings and search for the substring in them
    for (size_t i = 0; i < strs.GetCount(); i++)
    {
        if (IsTextInString(strs[i], text, ignoreCase, wholeWords, ignoreAmp, ignoreUnderscore))
            return i;
    }

    return -1;
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
    Found_InOrigPlural,
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
    bool inTrans = m_findInTrans->GetValue() && (m_catalog->HasCapability(Catalog::Cap::Translations));
    bool inSource = (mode == Mode_Find) && m_findInOrig->GetValue();
    bool inComments = (mode == Mode_Find) && m_findInComments->GetValue();
    bool ignoreCase = (mode == Mode_Find) && m_ignoreCase->GetValue();
    bool wholeWords = m_wholeWords->GetValue();
    bool wrapAround = m_wrapAround->GetValue();
    int posOrig = m_position;
    size_t trans;

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
    const bool ignoreAmp = (mode == Mode_Find) && (text.Find(_T('&')) == wxNOT_FOUND);
    const bool ignoreUnderscore = (mode == Mode_Find) && (text.Find(_T('_')) == wxNOT_FOUND);

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
            trans = IsTextInStrings(dt->GetTranslations(), text, ignoreCase, wholeWords, ignoreAmp, ignoreUnderscore);
            if (trans != (size_t)-1)
            {
                found = Found_InTrans;
                break;
            }
        }
        if (inSource)
        {
            if (IsTextInString(dt->GetString(), text, ignoreCase, wholeWords, ignoreAmp, ignoreUnderscore))
            {
                found = Found_InOrig;
                break;
            }
            if (dt->HasPlural() && IsTextInString(dt->GetPluralString(), text, ignoreCase, wholeWords, ignoreAmp, ignoreUnderscore))
            {
                found = Found_InOrigPlural;
                break;
            }
        }
        if (inComments)
        {
            if (IsTextInString(dt->GetComment(), text, ignoreCase, wholeWords, false, false))
            {
                found = Found_InComments;
                break;
            }
            if (IsTextInStrings(dt->GetExtractedComments(), text, ignoreCase, wholeWords, false, false) != (size_t)-1)
            {
                found = Found_InExtractedComments;
                break;
            }
        }
    }

    if (found != Found_Not)
    {
        m_lastItem = lastItem;

        m_listCtrl->EnsureVisible(m_listCtrl->ListIndexToListItem(m_position));
        m_listCtrl->SelectAndFocus(m_position);

        // find the text on the control and select it:

        CustomizedTextCtrl* txt = nullptr;
        switch (found)
        {
            case Found_InOrig:
              txt = m_editingArea->Ctrl_Original();
              break;
            case Found_InOrigPlural:
              txt = m_editingArea->Ctrl_OriginalPlural();
              break;
            case Found_InTrans:
              if (lastItem->GetNumberOfTranslations() == 1)
              {
                  txt = m_editingArea->Ctrl_Translation();
              }
              else
              {
                  m_editingArea->Ctrl_PluralNotebook()->SetSelection(trans);
                  txt = m_editingArea->Ctrl_PluralTranslation(trans);
              }
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
            FindTextInStringAndDo
            (
                textc, text, wholeWords,
                [=](const wxString&,size_t pos, size_t len)
                {
                    txt->ShowFindIndicator((int)pos, (int)len);
                    return wxString::npos;
                }
            );
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

    if (replaced)
    {
        item->SetModified(true);
        m_owner->MarkAsModified();
        if (item == m_owner->GetCurrentItem())
            m_owner->UpdateToTextCtrl(EditingArea::UndoableEdit);
    }

    return replaced;
}

void FindFrame::OnReplace(wxCommandEvent&)
{
    if (!m_lastItem)
        return;
    if (DoReplaceInItem(m_lastItem))
    {
        // FIXME: Only refresh affected items
        m_listCtrl->RefreshAllItems();
    }
}

void FindFrame::OnReplaceAll(wxCommandEvent&)
{
    bool replaced = false;
    for (auto& item: m_catalog->items())
    {
        if (DoReplaceInItem(item))
            replaced = true;
    }

    if (replaced)
    {
        // FIXME: Only refresh affected items
        m_listCtrl->RefreshAllItems();
    }
}
