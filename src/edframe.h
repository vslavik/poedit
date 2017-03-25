/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 1999-2017 Vaclav Slavik
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

#include <memory>
#include <set>

#include <wx/frame.h>
#include <wx/process.h>
#include <wx/msgdlg.h>
#include <wx/windowptr.h>

class WXDLLIMPEXP_FWD_CORE wxSplitterWindow;
class WXDLLIMPEXP_FWD_CORE wxSplitterEvent;

#include "catalog.h"
#include "gexecute.h"
#include "edlistctrl.h"
#include "edapp.h"

#ifdef __WXMSW__
  #include "windows/win10_menubar.h"
  typedef wxFrameWithWindows10Menubar PoeditFrameBase;
#else
  typedef wxFrame PoeditFrameBase;
#endif

class PoeditFrame;
class AttentionBar;
class FindFrame;
class MainToolbar;
class Sidebar;
class EditingArea;

/** This class provides main editing frame. It handles user's input
    and provides frontend to catalog editing engine. Nothing fancy.
 */
class PoeditFrame : public PoeditFrameBase
{
    public:
        /** Public constructor functions. Creates and shows frame
            and opens \a catalog. If \a catalog is already opened
            in another Poedit frame, then this function won't create
            new frame but instead return pointer to existing one.

            \param catalog filename of catalog to open.
         */
        static PoeditFrame *Create(const wxString& catalog, int lineno = 0);

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
        void OpenFile(const wxString& filename, int lineno = 0);

        /** Returns pointer to existing instance of PoeditFrame that currently
            exists and edits \a catalog. If no such frame exists, returns NULL.
         */
        static PoeditFrame *Find(const wxString& catalog);

        /// Returns active PoeditFrame, if it is unused (i.e. not showing
        /// content, not having catalog loaded); NULL otherwise.
        static PoeditFrame *UnusedActiveWindow() { return UnusedWindow(true); }
        /// Ditto, but not required to be active
        static PoeditFrame *UnusedWindow(bool active);

        /// Returns true if at least one one window has unsaved changes
        static bool AnyWindowIsModified();

        /// Returns true if any windows (with documents) are open
        static bool HasAnyWindow() { return !ms_instances.empty(); }

        static int GetOpenWindowsCount() { return (int)ms_instances.size(); }

        ~PoeditFrame();

        /// Reads catalog, refreshes controls, takes ownership of catalog.
        void ReadCatalog(const wxString& catalog);
        /// Reads catalog, refreshes controls, takes ownership of catalog.
        void ReadCatalog(const CatalogPtr& cat);
        /// Writes catalog.
        void WriteCatalog(const wxString& catalog);

        template<typename TFunctor>
        void WriteCatalog(const wxString& catalog, TFunctor completionHandler);

        void FixDuplicatesIfPresent();
        void WarnAboutLanguageIssues();

        /// Did the user modify the catalog?
        bool IsModified() const { return m_modified; }

        void MarkAsModified();

        /** Updates catalog and sets m_modified flag. Updates from POT
            if \a pot_file is not empty and from sources otherwise.
         */
        bool UpdateCatalog(const wxString& pot_file = wxEmptyString);

        void UpdateAfterPreferencesChange();
        static void UpdateAllAfterPreferencesChange();

        void EditCatalogProperties();
        void EditCatalogPropertiesAndUpdateFromSources();

        /// Returns currently selected (edited) item
        CatalogItemPtr GetCurrentItem() const;

        /// Puts text from catalog & listctrl to textctrls.
        void UpdateToTextCtrl(int flags);

        /// Puts text from textctrls to catalog & listctrl.
        void OnUpdatedFromTextCtrl(CatalogItemPtr item, bool statsChanged);

        wxString GetFileName() const
            { return m_catalog ? m_catalog->GetFileName() : wxString(); }
        wxString GetFileNamePartOfTitle() const
            { return m_fileNamePartOfTitle; }

    protected:
        // Don't show help in status bar, it's not common to do these days:
        void DoGiveHelp(const wxString& /*help*/, bool /*show*/) override {}

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
            POT,
            Empty_PO
        };
        Content m_contentType;
        /// parent of all content controls etc.
        wxWindow *m_contentView;
        wxSizer *m_contentWrappingSizer;

        /// Ensures creation of specified content view, destroying the
        /// current content if necessary
        void EnsureContentView(Content type);
        void EnsureAppropriateContentView();
        wxWindow* CreateContentViewPO(Content type);
        wxWindow* CreateContentViewWelcome();
        wxWindow* CreateContentViewEmptyPO();
        void DestroyContentView();

        void PlaceInitialFocus(int lineno = 0);

        typedef std::set<PoeditFrame*> PoeditFramesList;
        static PoeditFramesList ms_instances;

    private:
        /// Refreshes controls.
        enum { Refresh_NoCatalogChanged = 1 };
        void RefreshControls(int flags = 0);
        void NotifyCatalogChanged(const CatalogPtr& cat);

        /// Sets controls custom fonts.
        void SetCustomFonts();
        void SetAccelerators();

        // if there's modified catalog, ask user to save it; return true
        // if it's save to discard m_catalog and load new data
        template<typename TFunctor>
        void DoIfCanDiscardCurrentDoc(TFunctor completionHandler);
        bool NeedsToAskIfCanDiscardCurrentDoc() const;
        wxWindowPtr<wxMessageDialog> CreateAskAboutSavingDialog();

        // implements opening of files, without asking user
        void DoOpenFile(const wxString& filename, int lineno = 0);

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
        typedef bool (*NavigatePredicate)(const CatalogItemPtr& item);
        int NavigateGetNextItem(const int start, int step, NavigatePredicate predicate, bool wrap, CatalogItemPtr *out_item);
        void Navigate(int step, NavigatePredicate predicate, bool wrap);
        void OnDoneAndNext(wxCommandEvent&);
        void OnPrev(wxCommandEvent&);
        void OnNext(wxCommandEvent&);
        void OnPrevPage(wxCommandEvent&);
        void OnNextPage(wxCommandEvent&);
        void OnPrevUnfinished(wxCommandEvent&);
        void OnNextUnfinished(wxCommandEvent&);
        void OnPrevPluralForm(wxCommandEvent&);
        void OnNextPluralForm(wxCommandEvent&);

        // Message handlers:
public: // for PoeditApp
        void OnNew(wxCommandEvent& event);
        void NewFromScratch();
        void NewFromPOT();
        void NewFromPOT(const wxString& pot_file, Language language = Language());

        void OnOpen(wxCommandEvent& event);
        void OnOpenFromCrowdin(wxCommandEvent& event);
#ifndef __WXOSX__
        void OnOpenHist(wxCommandEvent& event);
        void OnCloseCmd(wxCommandEvent& event);
#endif
private:
        void OnSave(wxCommandEvent& event);
        void OnSaveAs(wxCommandEvent& event);
        template<typename F>
        void GetSaveAsFilenameThenDo(const CatalogPtr& cat, F then);
        void DoSaveAs(const wxString& filename);
        void OnProperties(wxCommandEvent& event);

        void OnUpdateFromSources(wxCommandEvent& event);
        void OnUpdateFromSourcesUpdate(wxUpdateUIEvent& event);
        void OnUpdateFromPOT(wxCommandEvent& event);
        void OnUpdateFromPOTUpdate(wxUpdateUIEvent& event);
        void OnUpdateFromCrowdin(wxCommandEvent& event);
        void OnUpdateFromCrowdinUpdate(wxUpdateUIEvent& event);
        void OnUpdateSmart(wxCommandEvent& event);
        void OnUpdateSmartUpdate(wxUpdateUIEvent& event);

        void OnValidate(wxCommandEvent& event);
        void OnListSel(wxDataViewEvent& event);
        void OnListRightClick(wxDataViewEvent& event);
        void OnListFocus(wxFocusEvent& event);
        void OnSplitterSashMoving(wxSplitterEvent& event);
        void OnSidebarSplitterSashMoving(wxSplitterEvent& event);
        void OnCloseWindow(wxCloseEvent& event);
        void OnReference(wxCommandEvent& event);
        void OnReferencesMenu(wxCommandEvent& event);
        void OnReferencesMenuUpdate(wxUpdateUIEvent& event);
        void ShowReference(int num);
        void OnRightClick(wxCommandEvent& event);
        void OnFuzzyFlag(wxCommandEvent& event);
        void OnIDsFlag(wxCommandEvent& event);
        void OnCopyFromSource(wxCommandEvent& event);
        void OnCopyFromSingular(wxCommandEvent& event);
        void OnClearTranslation(wxCommandEvent& event);
        void OnFind(wxCommandEvent& event);
        void OnFindAndReplace(wxCommandEvent& event);
        void OnFindNext(wxCommandEvent& event);
        void OnFindPrev(wxCommandEvent& event);
        void OnUpdateFind(wxUpdateUIEvent& event);
        void OnEditComment(wxCommandEvent& event);
        void OnSortByFileOrder(wxCommandEvent&);
        void OnSortBySource(wxCommandEvent&);
        void OnSortByTranslation(wxCommandEvent&);
        void OnSortGroupByContext(wxCommandEvent&);
        void OnSortUntranslatedFirst(wxCommandEvent&);
        void OnSortErrorsFirst(wxCommandEvent&);

        void OnShowHideSidebar(wxCommandEvent& event);
        void OnUpdateShowHideSidebar(wxUpdateUIEvent& event);
        void OnShowHideStatusbar(wxCommandEvent& event);
        void OnUpdateShowHideStatusbar(wxUpdateUIEvent& event);

        void OnSelectionUpdate(wxUpdateUIEvent& event);
        void OnSelectionUpdateEditable(wxUpdateUIEvent& event);
        void OnSingleSelectionUpdate(wxUpdateUIEvent& event);
        void OnSingleSelectionWithPluralsUpdate(wxUpdateUIEvent& event);
        void OnHasCatalogUpdate(wxUpdateUIEvent& event);
        void OnIsEditableUpdate(wxUpdateUIEvent& event);
        void OnEditCommentUpdate(wxUpdateUIEvent& event);

#if defined(__WXMSW__) || defined(__WXGTK__)
        void OnTextEditingCommand(wxCommandEvent& event);
        void OnTextEditingCommandUpdate(wxUpdateUIEvent& event);
#endif

        void OnSuggestion(wxCommandEvent& event);
        void OnPreTranslateAll(wxCommandEvent& event);

        void OnPurgeDeleted(wxCommandEvent& event);

        void OnGoToBookmark(wxCommandEvent& event);
        void OnSetBookmark(wxCommandEvent& event);

        void AddBookmarksMenu(wxMenu *menu);

        void OnCompileMO(wxCommandEvent& event);
        void OnExport(wxCommandEvent& event);
        bool ExportCatalog(const wxString& filename);

        void OnSize(wxSizeEvent& event);

        void RecreatePluralTextCtrls();

        template<typename TFunctor>
        void ReportValidationErrors(int errors, Catalog::CompilationStatus mo_compilation_status,
                                    bool from_save, bool other_file_saved,
                                    TFunctor completionHandler);

#ifndef __WXOSX__
        wxFileHistory& FileHistory() { return wxGetApp().FileHistory(); }
#endif
        void NoteAsRecentFile();

        void OnNewTranslationEntered(const CatalogItemPtr& item);

        DECLARE_EVENT_TABLE()

    private:
        CatalogPtr m_catalog;

        bool m_fileExistsOnDisk;

        wxString m_fileNamePartOfTitle;

        std::unique_ptr<MainToolbar> m_toolbar;

        CatalogItemPtr m_pendingHumanEditedItem;

        EditingArea *m_editingArea;
        wxSplitterWindow *m_splitter;
        wxSplitterWindow *m_sidebarSplitter;
        PoeditListCtrl *m_list;
#ifndef __WXOSX__
        wxMenu *m_menuForHistory;
#endif

        AttentionBar *m_attentionBar;
        Sidebar *m_sidebar;
        wxWeakRef<FindFrame> m_findWindow;

        bool m_modified;
        bool m_hasObsoleteItems;
        bool m_displayIDs;
        bool m_setSashPositionsWhenMaximized;
};


#endif // _EDFRAME_H_
