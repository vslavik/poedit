
/*

    poedit, a wxWindows i18n catalogs editor

    ---------------
      edframe.h
    
      Editor frame
    
      (c) Vaclav Slavik, 2000

*/


#ifdef __GNUG__
#pragma interface
#endif

#ifndef _EDFRAME_H_
#define _EDFRAME_H_

#include <wx/frame.h>
#include <wx/docview.h>
#include <wx/list.h>
class WXDLLEXPORT wxListCtrl;
class WXDLLEXPORT wxSpliterWindow;
class WXDLLEXPORT wxTextCtrl;
class WXDLLEXPORT wxGauge;

#ifdef __WXMSW__
#include <wx/msw/helpchm.h>
#else
#include <wx/html/helpctrl.h>
#endif

#include "catalog.h"

class poEditListCtrl;
class TranslationMemory;
class poEditFrame;

WX_DECLARE_LIST(poEditFrame, poEditFramesList);

/** This class provides main editing frame. It handles user's input 
    and provides frontend to catalog editing engine. Nothing fancy.
 */
class poEditFrame : public wxFrame
{
    public:
        /** Public constructor functions. Creates and shows frame
            (and optionally opens \a catalog). If \a catalog is not
            empty and is already opened in another poEdit frame,
            then this function won't create new frame but instead
            return pointer to existing one.
            
            \param catalog filename of catalog to open. If empty, starts
                           w/o opened file.
         */        
        static poEditFrame *Create(const wxString& catalog = wxEmptyString);
        
        
        /** Returns pointer to existing instance of poEditFrame that currently
            exists and edits \a catalog. If no such frame exists, returns NULL.
         */
        static poEditFrame *Find(const wxString& catalog);        
        
        ~poEditFrame();

        /// Reads catalog, refreshes controls.
        void ReadCatalog(const wxString& catalog);
        /// Writes catalog.
        void WriteCatalog(const wxString& catalog);
        /// Did the user modify the catalog?
        bool IsModified() const { return m_modified; }
        /// Updates catalog and sets m_modified flag
        void UpdateCatalog();

    private:
        /** Ctor.
            \param catalog filename of catalog to open. If empty, starts
                           w/o opened file.
         */
        poEditFrame(const wxString& catalog = wxEmptyString);
        
        static poEditFramesList ms_instances;

    private:
        /// Refreshes controls.
        void RefreshControls();
        /// Puts text from textctrls to catalog & listctrl.
        void UpdateFromTextCtrl(int item = -1);
        /// Puts text from catalog & listctrl to textctrls.
        void UpdateToTextCtrl(int item = -1);

        /// Updates statistics in statusbar.
        void UpdateStatusBar();
        /// Updates frame title.
        void UpdateTitle();
        /// Updates menu -- disables and enables items.
        void UpdateMenu();

        /// Returns popup menu for given catalog entry.
        wxMenu *GetPopupMenu(size_t item);

#ifdef USE_TRANSMEM
        /// Initializes translation memory, if enabled
        TranslationMemory *GetTransMem();
#endif

        // Message handlers:
        void OnNew(wxCommandEvent& event);
        void OnAbout(wxCommandEvent& event);
        void OnHelp(wxCommandEvent& event);
        void OnQuit(wxCommandEvent& event);
        void OnSave(wxCommandEvent& event);
        void OnSaveAs(wxCommandEvent& event);
        void OnOpen(wxCommandEvent& event);
        void OnOpenHist(wxCommandEvent& event);
        void OnSettings(wxCommandEvent& event);
        void OnPreferences(wxCommandEvent& event);
        void OnUpdate(wxCommandEvent& event);
        void OnListSel(wxListEvent& event);
        void OnListDesel(wxListEvent& event);
        void OnCloseWindow(wxCloseEvent& event);
        void OnReference(wxCommandEvent& event);
        void OnReferencesMenu(wxCommandEvent& event);
        void ShowReference(int num);
        void OnRightClick(wxCommandEvent& event);            
        void OnFuzzyFlag(wxCommandEvent& event);
        void OnQuotesFlag(wxCommandEvent& event);
        void OnInsertOriginal(wxCommandEvent& event);
        void OnFullscreen(wxCommandEvent& event);
        void OnFind(wxCommandEvent& event);
        void OnEditComment(wxCommandEvent& event);
        void OnManager(wxCommandEvent& event);
#ifdef USE_TRANSMEM
        void OnAutoTranslate(wxCommandEvent& event);
#endif
        DECLARE_EVENT_TABLE()

    private:
        Catalog *m_catalog;
        wxString m_fileName;
        
#ifdef USE_TRANSMEM
        TranslationMemory *m_transMem;
        bool m_transMemLoaded;
        wxArrayString m_autoTranslations;
#endif

#ifdef __WXMSW__
        wxCHMHelpController m_help;
#else
        wxHtmlHelpController m_help;
#endif

        wxSplitterWindow *m_splitter;
        poEditListCtrl *m_list;
        wxTextCtrl *m_textOrig, *m_textTrans;
        wxGauge *m_statusGauge;
#ifdef __WXMSW__
        wxFont m_boldGuiFont;
#endif

        bool m_modified;
        bool m_hasObsoleteItems;
        bool m_displayQuotes;          
        int m_sel, m_selItem;
        wxFileHistory m_history;
        wxString m_edittedTextOrig;
        
        friend class ListHandler;
};


#endif // _EDFRAME_H_
