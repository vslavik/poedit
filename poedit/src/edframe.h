
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
#include <wx/treectrl.h>
#include <wx/textctrl.h>
#include <wx/splitter.h>
#include <wx/docview.h>
#include <wx/listctrl.h>

#ifdef __WXMSW__
#include <wx/msw/helpchm.h>
#else
#include <wx/html/helpctrl.h>
#endif

#include "catalog.h"

class poEditListCtrl;

// This class is main editing frame. It handles user's input 
// and provides fronted to catalog editing engine

class poEditFrame : public wxFrame
{
    public:
            poEditFrame(const wxString& title, const wxString& catalog = wxEmptyString);
            ~poEditFrame();
            
    private:
            // Reads catalog, refreshes controls
            void ReadCatalog(const wxString& catalog);
            // Refreshes controls
            void RefreshControls();
            // Writes catalog
            void WriteCatalog(const wxString& catalog);
            // Puts text from textctrls to catalog & listctrl
            void UpdateFromTextCtrl(int item = -1);
            // ...and the other way around
            void UpdateToTextCtrl(int item = -1);
            
            // Updates statistics in statusbar
            void UpdateStatusBar();
            // Updates title
            void UpdateTitle();
            // Updates menu -- disables and enables items
            void UpdateMenu();

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
    
            DECLARE_EVENT_TABLE()

    private:
            Catalog *m_Catalog;
            wxString m_FileName;
	    
#ifdef __WXMSW__
    	    wxCHMHelpController m_Help;
#else
    	    wxHtmlHelpController m_Help;
#endif
            
            wxString m_Title;
            wxSplitterWindow *m_Splitter;
            poEditListCtrl *m_List;
            wxTextCtrl *m_TextOrig, *m_TextTrans;
            bool m_Modified;
            bool m_HasObsoleteItems;
            bool m_DisplayQuotes;          
            int m_Sel, m_SelItem;
            wxFileHistory m_History;
            wxString m_edittedTextOrig;
};



#endif // _EDFRAME_H_
