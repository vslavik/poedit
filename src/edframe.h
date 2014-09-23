/*
 *  This file is part of Poedit (http://poedit.net)
 *
 *  Copyright (C) 1999-2014 Vaclav Slavik
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

#include <set>

#include <wx/frame.h>
#include <wx/process.h>
#include <wx/msgdlg.h>
#include <wx/windowptr.h>

class WXDLLIMPEXP_FWD_CORE wxSplitterWindow;
class WXDLLIMPEXP_FWD_CORE wxSplitterEvent;
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
class SourceTextCtrl;
class TranslationTextCtrl;

class PoeditFrame;
class AttentionBar;
class ErrorBar;
class Sidebar;

/** This class provides main editing frame. It handles user's input
    and provides frontend to catalog editing engine. Nothing fancy.
 */
class PoeditFrame : public wxFrame
{
    public:
        /** Public constructor functions. Creates and shows frame
            and opens \a catalog. If \a catalog is already opened
            in another Poedit frame, then this function won't create
            new frame but instead return pointer to existing one.

            \param catalog filename of catalog to open.
         */
        static PoeditFrame *Create(const wxString& catalog);

        /** Public constructor functions. Creates and shows frame
            without catalog or other content.
         */
        static PoeditFrame *CreateEmpty();

        /** Public constructor functions. Creates and shows frame
            without catalog, just a welcome screen.
         */
        static PoeditFrame *CreateWelcome();

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

        /// Returns true if at least one one window has unsaved changes
        static bool AnyWindowIsModified();

        /// Returns true if any windows (with documents) are open
        static bool HasAnyWindow() { return !ms_instances.empty(); }

        ~PoeditFrame();

        /// Reads catalog, refreshes controls, takes ownership of catalog.
        void ReadCatalog(const wxString& catalog);
        /// Reads catalog, refreshes controls, takes ownership of catalog.
        void ReadCatalog(Catalog *cat);
        /// Writes catalog.
        void WriteCatalog(const wxString& catalog);

        template<typename TFunctor>
        void WriteCatalog(const wxString& catalog, TFunctor completionHandler);

        /// Did the user modify the catalog?
        bool IsModified() const { return m_modified; }
        /** Updates catalog and sets m_modified flag. Updates from POT
            if \a pot_file is not empty and from sources otherwise.
         */
        bool UpdateCatalog(const wxString& pot_file = wxEmptyString);


        virtual void DoGiveHelp(const wxString& text, bool show);

        void UpdateAfterPreferencesChange();
        static void UpdateAllAfterPreferencesChange();

        void EditCatalogProperties();
        void EditCatalogPropertiesAndUpdateFromSources();

    private:
        /** Ctor.
            \param catalog filename of catalog to open. If empty, starts
                           w/o opened file.
         */
        PoeditFrame();

        /// Current kind of content view
        enum class Content
        {
            Invalid, // no content whatsoever
            Welcome,
            PO,
            Empty_PO
        };
        Content m_contentType;
        /// parent of all content controls etc.
        wxWindow *m_contentView;
        wxSizer *m_contentWrappingSizer;

        /// Ensures creation of specified content view, destroying the
        /// current content if necessary
        void EnsureContentView(Content type);
        wxWindow* CreateContentViewPO();
        wxWindow* CreateContentViewWelcome();
        wxWindow* CreateContentViewEmptyPO();
        void DestroyContentView();

        typedef std::set<PoeditFrame*> PoeditFramesList;
        static PoeditFramesList ms_instances;

    private:
        /// Refreshes controls.
        enum { Refresh_NoCatalogChanged = 1 };
        void RefreshControls(int flags = 0);

        /// Sets controls custom fonts.
        void SetCustomFonts();
        void SetAccelerators();

        CatalogItem *GetCurrentItem() const;

        // if there's modified catalog, ask user to save it; return true
        // if it's save to discard m_catalog and load new data
        template<typename TFunctor>
        void DoIfCanDiscardCurrentDoc(TFunctor completionHandler);
        bool NeedsToAskIfCanDiscardCurrentDoc() const;
        wxWindowPtr<wxMessageDialog> CreateAskAboutSavingDialog();

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

        // Called when catalog's language possibly changed
        void UpdateTextLanguage();

        /// Returns popup menu for given catalog entry.
        wxMenu *GetPopupMenu(int item);

        // (Re)initializes spellchecker, if needed
        void InitSpellchecker();

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
        void NewFromScratch();
        void NewFromPOT();

        void OnOpen(wxCommandEvent& event);
#ifndef __WXOSX__
        void OnOpenHist(wxCommandEvent& event);
#endif
private:
        void OnCloseCmd(wxCommandEvent& event);
        void OnSave(wxCommandEvent& event);
        void OnSaveAs(wxCommandEvent& event);
        wxString GetSaveAsFilename(Catalog *cat, const wxString& current);
        void DoSaveAs(const wxString& filename);
        void OnProperties(wxCommandEvent& event);
        void OnUpdate(wxCommandEvent& event);
        void OnValidate(wxCommandEvent& event);
        void OnListSel(wxListEvent& event);
        void OnListRightClick(wxMouseEvent& event);
        void OnListFocus(wxFocusEvent& event);
        void OnSplitterSashMoving(wxSplitterEvent& event);
        void OnCloseWindow(wxCloseEvent& event);
        void OnReference(wxCommandEvent& event);
        void OnReferencesMenu(wxCommandEvent& event);
        void ShowReference(int num);
        void OnRightClick(wxCommandEvent& event);
        void OnFuzzyFlag(wxCommandEvent& event);
        void OnIDsFlag(wxCommandEvent& event);
        void OnCopyFromSource(wxCommandEvent& event);
        void OnClearTranslation(wxCommandEvent& event);
        void OnFind(wxCommandEvent& event);
        void OnFindNext(wxCommandEvent& event);
        void OnFindPrev(wxCommandEvent& event);
        void OnEditComment(wxCommandEvent& event);
        void OnSortByFileOrder(wxCommandEvent&);
        void OnSortBySource(wxCommandEvent&);
        void OnSortByTranslation(wxCommandEvent&);
        void OnSortGroupByContext(wxCommandEvent&);
        void OnSortUntranslatedFirst(wxCommandEvent&);

        void OnSelectionUpdate(wxUpdateUIEvent& event);
        void OnSingleSelectionUpdate(wxUpdateUIEvent& event);

#if defined(__WXMSW__) || defined(__WXGTK__)
        void OnTextEditingCommand(wxCommandEvent& event);
        void OnTextEditingCommandUpdate(wxUpdateUIEvent& event);
#endif

        void OnSuggestion(wxCommandEvent& event);
        void OnAutoTranslateAll(wxCommandEvent& event);
        bool AutoTranslateCatalog(int *matchesCount = nullptr);

        void OnPurgeDeleted(wxCommandEvent& event);

        void OnGoToBookmark(wxCommandEvent& event);
        void OnSetBookmark(wxCommandEvent& event);

        void AddBookmarksMenu(wxMenu *menu);

        void OnExport(wxCommandEvent& event);
        bool ExportCatalog(const wxString& filename);

        void OnSize(wxSizeEvent& event);

        void ShowPluralFormUI(bool show = true);

        void RecreatePluralTextCtrls();

        template<typename TFunctor>
        void ReportValidationErrors(int errors, Catalog::CompilationStatus mo_compilation_status,
                                    bool from_save, TFunctor completionHandler);

#ifndef __WXOSX__
        wxFileHistory& FileHistory() { return wxGetApp().FileHistory(); }
#endif
        void NoteAsRecentFile();

        DECLARE_EVENT_TABLE()

    private:
        Catalog *m_catalog;
        wxString m_fileName;
        bool m_fileExistsOnDisk;

        wxPanel *m_bottomPanel;
        wxSplitterWindow *m_splitter;
        PoeditListCtrl *m_list;
        wxStaticText *m_labelContext;
        ErrorBar *m_errorBar;
        SourceTextCtrl *m_textOrig, *m_textOrigPlural;
        TranslationTextCtrl *m_textTrans;
        std::vector<TranslationTextCtrl*> m_textTransPlural;
        TranslationTextCtrl *m_textTransSingularForm;
        wxNotebook *m_pluralNotebook;
        wxStaticText *m_labelSingular, *m_labelPlural;
#ifndef __WXOSX__
        wxMenu *m_menuForHistory;
#endif

        wxFont m_normalGuiFont, m_boldGuiFont;

        AttentionBar *m_attentionBar;
        Sidebar *m_sidebar;

        bool m_modified;
        bool m_hasObsoleteItems;
        bool m_displayIDs;
        bool m_dontAutoclearFuzzyStatus;
        bool m_setSashPositionsWhenMaximized;

        friend class ListHandler;
        friend class TextctrlHandler;
        friend class TransTextctrlHandler;
};


#endif // _EDFRAME_H_
