
/*

    poedit, a wxWindows i18n catalogs editor

    ---------------
      prefsdlg.h
    
      Preferences settings dialog
    
      (c) Vaclav Slavik, 2000

*/


#if defined(__GNUG__) && !defined(__APPLE__)
#pragma interface
#endif

#ifndef _PREFSDLG_H_
#define _PREFSDLG_H_

#include <wx/dialog.h>

#include "parser.h"

class WXDLLEXPORT wxConfigBase;

/** Preferences dialog for setting user's identity, parsers and other
    global, catalog-independent settings.
 */
class PreferencesDialog : public wxDialog
{
    public:
        /// Ctor.
        PreferencesDialog(wxWindow *parent = NULL);

        /// Reads data from config/registry and fills dialog's controls.
        void TransferTo(wxConfigBase *cfg);

        /// Saves data from the dialog to config/registry.
        void TransferFrom(wxConfigBase *cfg);
            
    private:
        ParsersDB m_parsers;

    private:
        DECLARE_EVENT_TABLE()

#ifdef USE_TRANSMEM
        void OnTMAddLang(wxCommandEvent& event);
        void OnTMBrowseDbPath(wxCommandEvent& event);
        void OnTMGenerate(wxCommandEvent& event);
#endif

#ifndef __UNIX__
        void OnUILanguage(wxCommandEvent& event);
#endif
        void OnNewParser(wxCommandEvent& event);
        void OnEditParser(wxCommandEvent& event);
        void OnDeleteParser(wxCommandEvent& event);
        /// Called to launch dialog for editting parser properties.
        bool EditParser(int num);

        void OnChooseListFont(wxCommandEvent& event);
        void OnChooseTextFont(wxCommandEvent& event);
        void DoChooseFont(wxTextCtrl *nameField);
};


#endif // _PREFSDLG_H_
