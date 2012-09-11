/*
 *  This file is part of Poedit (http://www.poedit.net)
 *
 *  Copyright (C) 1999-2012 Vaclav Slavik
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

#ifndef _EDFRAME_H_
#define _EDFRAME_H_

#if defined(__WXMSW__)
  #define CAN_MODIFY_DEFAULT_FONT
#endif

#include <set>

#include <wx/frame.h>
#include <wx/process.h>

class WXDLLIMPEXP_FWD_CORE wxSplitterWindow;
class WXDLLIMPEXP_FWD_CORE wxTextCtrl;
class WXDLLIMPEXP_FWD_CORE wxGauge;
class WXDLLIMPEXP_FWD_CORE wxNotebook;
class WXDLLIMPEXP_FWD_CORE wxStaticText;

#include "catalog.h"
#include "gexecute.h"
#include "edlistctrl.h"
#include "edapp.h"

class ListHandler;
class TextctrlHandler;
class TransTextctrlHandler;

class TranslationMemory;
class PoeditFrame;
class AttentionBar;
class ErrorBar;

WX_DECLARE_LIST(PoeditFrame, PoeditFramesList);

/** This class provides main editing frame. It handles user's input
    and provides frontend to catalog editing engine. Nothing fancy.
 */
class PoeditFrame : public wxFrame
{
    public:
        /** Public constructor functions. Creates and shows frame
            (and optionally opens \a catalog). If \a catalog is not
            empty and is already opened in another Poedit frame,
            then this function won't create new frame but instead
            return pointer to existing one.

            \param catalog filename of catalog to open. If empty, starts
                           w/o opened file.
         */
        static PoeditFrame *Create(const wxString& catalog = wxEmptyString);

        /// Opens given file in this frame. Asks user for permission first
        /// if there's unsaved document.
        void OpenFile(const wxString& filename);

        /** Returns pointer to existing instance of PoeditFrame that currently
            exists and edits \a catalog. If no such frame exists, returns NULL.
         */
        static PoeditFrame *Find(const wxString& catalog);

        /// Returns active PoeditFrame, if it is unused (i.e. not showing
        /// content, not having catalog loaded); NULL otherwise.
        static PoeditFrame *UnusedActiveWindow();

        ~PoeditFrame();

        /// Reads catalog, refreshes controls.
        void ReadCatalog(const wxString& catalog);
        /// Writes catalog.
        bool WriteCatalog(const wxString& catalog);
        /// Did the user modify the catalog?
        bool IsModified() const { return m_modified; }
        /** Updates catalog and sets m_modified flag. Updates from POT
            if \a pot_file is not empty and from sources otherwise.
         */
        void UpdateCatalog(const wxString& pot_file = wxEmptyString);


        virtual void DoGiveHelp(const wxString& text, bool show);

        void UpdateAfterPreferencesChange();
        static void UpdateAllAfterPreferencesChange();

    private:
        /** Ctor.
            \param catalog filename of catalog to open. If empty, starts
                           w/o opened file.
         */
        PoeditFrame();

        static PoeditFramesList ms_instances;

    private:
        /// Refreshes controls.
        void RefreshControls();
        /// Sets controls custom fonts.
        void SetCustomFonts();
        void SetAccelerators();

        CatalogItem *GetCurrentItem() const;

        // if there's modified catalog, ask user to save it; return true
        // if it's save to discard m_catalog and load new data
        bool CanDiscardCurrentDoc();
        // implements opening of files, without asking user
        void DoOpenFile(const wxString& filename);

        /// Puts text from textctrls to catalog & listctrl.
        void UpdateFromTextCtrl();
        /// Puts text from catalog & listctrl to textctrls.
        void UpdateToTextCtrl();

        /// Updates statistics in statusbar.
        void UpdateStatusBar();
        /// Updates frame title.
        void UpdateTitle();
        /// Updates menu -- disables and enables items.
        void UpdateMenu();

        /// Updates the editable nature of the comment window
        void UpdateCommentWindowEditable();

        /// Returns popup menu for given catalog entry.
        wxMenu *GetPopupMenu(size_t item);

#ifdef USE_TRANSMEM
        /// Initializes translation memory, if enabled
        TranslationMemory *GetTransMem();
#endif

        // (Re)initializes spellchecker, if needed
        void InitSpellchecker();

        void EditCatalogProperties();

        // navigation to another item in the list
        typedef bool (*NavigatePredicate)(const CatalogItem& item);
        void Navigate(int step, NavigatePredicate predicate, bool wrap);
        void OnDoneAndNext(wxCommandEvent&);
        void OnPrev(wxCommandEvent&);
        void OnNext(wxCommandEvent&);
        void OnPrevPage(wxCommandEvent&);
        void OnNextPage(wxCommandEvent&);
        void OnPrevUnfinished(wxCommandEvent&);
        void OnNextUnfinished(wxCommandEvent&);

        // Message handlers:
public: // for PoeditApp
        void OnNew(wxCommandEvent& event);
        void OnOpen(wxCommandEvent& event);
        void OnOpenHist(wxCommandEvent& event);
private:
        void OnCloseCmd(wxCommandEvent& event);
        void OnSave(wxCommandEvent& event);
        void OnSaveAs(wxCommandEvent& event);
        wxString GetSaveAsFilename(Catalog *cat, const wxString& current);
        void DoSaveAs(const wxString& filename);
        void OnProperties(wxCommandEvent& event);
#ifdef __WXMSW__
        void OnWinsparkleCheck(wxCommandEvent& event);
#endif
        void OnUpdate(wxCommandEvent& event);
        void OnValidate(wxCommandEvent& event);
        void OnListSel(wxListEvent& event);
        void OnListRightClick(wxMouseEvent& event);
        void OnListFocus(wxFocusEvent& event);
        void OnCloseWindow(wxCloseEvent& event);
        void OnReference(wxCommandEvent& event);
        void OnReferencesMenu(wxCommandEvent& event);
        void ShowReference(int num);
        void OnRightClick(wxCommandEvent& event);
        void OnFuzzyFlag(wxCommandEvent& event);
        void OnQuotesFlag(wxCommandEvent& event);
        void OnLinesFlag(wxCommandEvent& event);
        void OnCommentWinFlag(wxCommandEvent& event);
        void OnAutoCommentsWinFlag(wxCommandEvent& event);
        void OnCopyFromSource(wxCommandEvent& event);
        void OnClearTranslation(wxCommandEvent& event);
        void OnFind(wxCommandEvent& event);
        void OnEditComment(wxCommandEvent& event);
        void OnCommentWindowText(wxCommandEvent& event);
        void OnSortByFileOrder(wxCommandEvent&);
        void OnSortBySource(wxCommandEvent&);
        void OnSortByTranslation(wxCommandEvent&);
        void OnSortUntranslatedFirst(wxCommandEvent&);
#ifdef USE_TRANSMEM
        void OnAutoTranslate(wxCommandEvent& event);
        void OnAutoTranslateAll(wxCommandEvent& event);
        bool AutoTranslateCatalog();
#endif
        void OnPurgeDeleted(wxCommandEvent& event);

        void OnGoToBookmark(wxCommandEvent& event);
        void OnSetBookmark(wxCommandEvent& event);

        void AddBookmarksMenu(wxMenu *menu);

        void OnExport(wxCommandEvent& event);
        bool ExportCatalog(const wxString& filename);

        void OnIdle(wxIdleEvent& event);
        void OnSize(wxSizeEvent& event);

        // updates the status of both comment windows: Automatic and Translator's
        void UpdateDisplayCommentWin();

        void ShowPluralFormUI(bool show = true);

        void RecreatePluralTextCtrls();

        void RefreshSelectedItem();

        void ReportValidationErrors(int errors, bool from_save);

        wxFileHistory& FileHistory() { return wxGetApp().FileHistory(); }

        DECLARE_EVENT_TABLE()

    private:
        std::set<int> m_itemsRefreshQueue;

        bool m_commentWindowEditable;
        Catalog *m_catalog;
        wxString m_fileName;

#ifdef USE_TRANSMEM
        TranslationMemory *m_transMem;
        bool m_transMemLoaded;
        wxArrayString m_autoTranslations;
#endif

        wxPanel *m_bottomLeftPanel;
        wxPanel *m_bottomRightPanel;
        wxSplitterWindow *m_splitter, *m_bottomSplitter;
        PoeditListCtrl *m_list;
        wxStaticText *m_labelComment, *m_labelAutoComments;
        wxStaticText *m_labelContext;
        ErrorBar *m_errorBar;
        wxTextCtrl *m_textOrig, *m_textOrigPlural, *m_textTrans, *m_textComment, *m_textAutoComments;
        std::vector<wxTextCtrl*> m_textTransPlural;
        wxTextCtrl *m_textTransSingularForm;
        wxNotebook *m_pluralNotebook;
        wxStaticText *m_labelSingular, *m_labelPlural;
        wxMenu *m_menuForHistory;

        wxFont m_normalGuiFont, m_boldGuiFont;

        AttentionBar *m_attentionBar;

        bool m_modified;
        bool m_hasObsoleteItems;
        bool m_displayQuotes;
        bool m_displayLines;
        bool m_displayCommentWin;
        bool m_displayAutoCommentsWin;
        bool m_dontAutoclearFuzzyStatus;
        bool m_setSashPositionsWhenMaximized;

        friend class ListHandler;
        friend class TextctrlHandler;
        friend class TransTextctrlHandler;
};


#endif // _EDFRAME_H_
