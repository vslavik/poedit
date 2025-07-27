/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 1999-2025 Vaclav Slavik
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

#ifndef Poedit_editing_area_h
#define Poedit_editing_area_h

#include "catalog.h"

#include <wx/panel.h>

#include <functional>
#include <vector>

class PoeditListCtrl;
class SourceTextCtrl;
class TranslationTextCtrl;
class SwitchButton;

class WXDLLIMPEXP_FWD_CORE wxBoxSizer;
class WXDLLIMPEXP_FWD_CORE wxBookCtrlBase;
class WXDLLIMPEXP_FWD_CORE wxStaticText;


/**
    Bottom area of the main screen where editing takes place.
 */
class EditingArea : public wxPanel
{
public:
    /// Control's operation mode
    enum Mode
    {
        Editing,
        POT
    };

    /// Flags for UpdateToTextCtrl()
    enum UpdateToTextCtrlFlags
    {
        /// Change to textctrl should be undoable by the user
        UndoableEdit = 0x01,
        /// Change is due to item change, discard undo buffer
        ItemChanged = 0x02,
        /// Only update non-text information (auxiliary, fuzzy etc.)
        DontTouchText = 0x04
    };

    /// Constructor
    EditingArea(wxWindow *parent, PoeditListCtrl *associatedList, Mode mode);
    ~EditingArea();

    // Hooked-up signals:

    /// Called from UpdateFromTextCtrl() after filling item with data
    std::function<void(CatalogItemPtr, bool /*statsChanged*/)> OnUpdatedFromTextCtrl;

    void SetCustomFont(const wxFont& font);
    bool InitSpellchecker(bool enabled, Language lang);

    void SetLanguage(Language lang);
    void UpdateEditingUIForCatalog(CatalogPtr catalog);

    void SetSingleSelectionMode();
    void SetMultipleSelectionMode();

    void SetTextFocus();
    bool HasTextFocus();
    bool HasTextFocusInPlurals();
    bool IsShowingPlurals();

    void CopyFromSingular();

    /// Puts text from catalog & listctrl to textctrls.
    void UpdateToTextCtrl(CatalogItemPtr item, int flags);

    /// Puts text from textctrls to catalog & listctrl.
    void UpdateFromTextCtrl();

    void DontAutoclearFuzzyStatus() { m_dontAutoclearFuzzyStatus = true; }
    bool ShouldNotAutoclearFuzzyStatus() { return m_dontAutoclearFuzzyStatus; }

    /// Move focused tab to prev(-1) or next(+1)
    void ChangeFocusedPluralTab(int offset);

    /// Returns height of the source line at the top with issues shown
    int GetTopRowHeight() const;

    // Semi-private use (TODO: get rid of them)
    SourceTextCtrl *Ctrl_Original() const { return m_textOrig; }
    SourceTextCtrl *Ctrl_OriginalPlural() const { return m_textOrigPlural; }
    TranslationTextCtrl *Ctrl_Translation() const { return m_textTrans; }
    wxBookCtrlBase *Ctrl_PluralNotebook() const { return m_pluralNotebook; }
    TranslationTextCtrl *Ctrl_PluralTranslation(size_t index) const { return m_textTransPlural[index]; }

private:
    void RecreatePluralTextCtrls(CatalogPtr catalog);

    void UpdateAuxiliaryInfo(CatalogItemPtr item);
    void UpdateCharCounter(CatalogItemPtr item);

    void CreateEditControls(wxBoxSizer *sizer);
    void CreateTemplateControls(wxBoxSizer *sizer);
    wxBoxSizer *CreatePlaceholderControls();

    void SetupTextCtrlSizes();

    void ShowPluralFormUI(bool show);

    void ShowPart(wxWindow *part, bool show);

    void OnPaint(wxPaintEvent&);

private:
    class TagLabel;
    class IssueLabel;
    class CharCounter;

    PoeditListCtrl *m_associatedList;

    bool m_isSingleSelection = true;
    bool m_fuzzyToggleNeeded = true;

    bool m_dontAutoclearFuzzyStatus;

    wxBoxSizer *m_controlsSizer, *m_placeholderSizer;

    SourceTextCtrl *m_textOrig, *m_textOrigPlural;

    SwitchButton *m_fuzzy;
    TranslationTextCtrl *m_textTrans;
    std::vector<TranslationTextCtrl*> m_textTransPlural;
    TranslationTextCtrl *m_textTransSingularForm;

    wxBookCtrlBase *m_pluralNotebook;
    wxStaticText *m_labelSingular, *m_labelPlural;
    wxStaticText *m_labelSource, *m_labelTrans;
    wxStaticText *m_labelPlaceholder;

    TagLabel *m_tagIdOrContext;
    TagLabel *m_tagFormat;
    TagLabel *m_tagPretranslated;

    IssueLabel *m_issueLine;

    CharCounter *m_charCounter;
};

#endif // Poedit_editing_area_h
