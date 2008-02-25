/*
 *  This file is part of Poedit (http://www.poedit.net)
 *
 *  Copyright (C) 1999-2008 Vaclav Slavik
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
 *  Editor frame
 *
 */

#ifndef _EDFRAME_H_
#define _EDFRAME_H_

#if defined(__WXMSW__)
  #define CAN_MODIFY_DEFAULT_FONT
#endif

// disable on-the-fly gettext validation, launching so many xmsgfmt
// processes doesn't work well:
#define USE_GETTEXT_VALIDATION 0

#include <list>

#include <wx/frame.h>
#include <wx/docview.h>
#include <wx/process.h>

class WXDLLEXPORT wxSplitterWindow;
class WXDLLEXPORT wxTextCtrl;
class WXDLLEXPORT wxGauge;
class WXDLLEXPORT wxNotebook;
class WXDLLEXPORT wxStaticText;

#ifdef __WXMSW__
#include <wx/msw/helpchm.h>
#else
#include <wx/html/helpctrl.h>
#endif

#include "catalog.h"
#include "gexecute.h"
#include "edlistctrl.h"

class ListHandler;
class TextctrlHandler;
class TransTextctrlHandler;

class TranslationMemory;
class PoeditFrame;

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


        /** Returns pointer to existing instance of PoeditFrame that currently
            exists and edits \a catalog. If no such frame exists, returns NULL.
         */
        static PoeditFrame *Find(const wxString& catalog);

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

        CatalogItem *GetCurrentItem() const;

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

        // Message handlers:
        void OnNew(wxCommandEvent& event);
        void OnAbout(wxCommandEvent& event);
        void OnHelp(wxCommandEvent& event);
        void OnHelpGettext(wxCommandEvent& event);
        void OnQuit(wxCommandEvent& event);
        void OnSave(wxCommandEvent& event);
        void OnSaveAs(wxCommandEvent& event);
        wxString GetSaveAsFilename(Catalog *cat, const wxString& current);
        void DoSaveAs(const wxString& filename);
        void OnOpen(wxCommandEvent& event);
        void OnOpenHist(wxCommandEvent& event);
#ifdef __WXMSW__
        void OnFileDrop(wxDropFilesEvent& event);
#endif
        void OnSettings(wxCommandEvent& event);
        void OnPreferences(wxCommandEvent& event);
        void OnUpdate(wxCommandEvent& event);
        void OnListSel(wxListEvent& event);
        void OnListActivated(wxListEvent& event);
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
        void OnShadedListFlag(wxCommandEvent& event);
        void OnInsertOriginal(wxCommandEvent& event);
#ifndef __WXMAC__
        void OnFullscreen(wxCommandEvent& event);
#endif
        void OnFind(wxCommandEvent& event);
        void OnEditComment(wxCommandEvent& event);
        void OnManager(wxCommandEvent& event);
        void OnCommentWindowText(wxCommandEvent& event);
#ifdef USE_TRANSMEM
        void OnAutoTranslate(wxCommandEvent& event);
        void OnAutoTranslateAll(wxCommandEvent& event);
        bool AutoTranslateCatalog();
#endif
        void OnPurgeDeleted(wxCommandEvent& event);

        void OnGoToBookmark(wxCommandEvent& event);
        void OnSetBookmark(wxCommandEvent& event);

        void AddBookmarksMenu();

        void OnExport(wxCommandEvent& event);
        bool ExportCatalog(const wxString& filename);

        void OnIdle(wxIdleEvent& event);
        void OnEndProcess(wxProcessEvent& event);

        void BeginItemValidation();
        void EndItemValidation();

        // stops validation
        void CancelItemsValidation();
        // starts validation from scratch
        void RestartItemsValidation();

        // updates the status of both comment windows: Automatic and Translator's
        void UpdateDisplayCommentWin();

        void ShowPluralFormUI(bool show = true);

        void RecreatePluralTextCtrls();

        void InitHelp();
        wxString LoadHelpBook(const wxString& name);

        DECLARE_EVENT_TABLE()

    private:
#if USE_GETTEXT_VALIDATION
        struct ValidationProcessData : public GettextProcessData
        {
            wxString tmp1, tmp2;
        };

        wxProcess *m_gettextProcess;
        int m_itemBeingValidated;
        std::list<int> m_itemsToValidate;
        ValidationProcessData m_validationProcess;
#endif

        bool m_commentWindowEditable;
        Catalog *m_catalog;
        wxString m_fileName;

#ifdef USE_TRANSMEM
        TranslationMemory *m_transMem;
        bool m_transMemLoaded;
        wxArrayString m_autoTranslations;
#endif

        bool m_helpInitialized;
#ifdef __WXMSW__
        wxCHMHelpController m_help;
#else
        wxHtmlHelpController m_help;
#endif
        wxString m_helpBook, m_helpBookGettext;

        wxPanel *m_bottomLeftPanel;
        wxPanel *m_bottomRightPanel;
        wxSplitterWindow *m_splitter, *m_bottomSplitter;
        PoeditListCtrl *m_list;
        wxTextCtrl *m_textOrig, *m_textOrigPlural, *m_textTrans, *m_textComment, *m_textAutoComments;
        std::vector<wxTextCtrl*> m_textTransPlural;
        wxTextCtrl *m_textTransSingularForm;
        wxNotebook *m_pluralNotebook;
        wxStaticText *m_labelSingular, *m_labelPlural;
#ifdef CAN_MODIFY_DEFAULT_FONT
        wxFont m_boldGuiFont;
#endif

        bool m_modified;
        bool m_hasObsoleteItems;
        bool m_displayQuotes;
        bool m_displayLines;
        bool m_displayCommentWin;
        bool m_displayAutoCommentsWin;
        wxFileHistory m_history;
        bool m_dontAutoclearFuzzyStatus;

        friend class ListHandler;
        friend class TextctrlHandler;
        friend class TransTextctrlHandler;
};


#endif // _EDFRAME_H_
