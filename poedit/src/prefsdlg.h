
/*

    poedit, a wxWindows i18n catalogs editor

    ---------------
      prefsdlg.h
    
      Preferences settings dialog
    
      (c) Vaclav Slavik, 2000

*/


#ifdef __GNUG__
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

        void OnTMAddLang(wxCommandEvent& event);
        void OnTMBrowseDbPath(wxCommandEvent& event);
        void OnTMGenerate(wxCommandEvent& event);

        void OnNewParser(wxCommandEvent& event);
        void OnEditParser(wxCommandEvent& event);
        void OnDeleteParser(wxCommandEvent& event);
        /// Called to launch dialog for editting parser properties.
        bool EditParser(int num);
};


#endif // _PREFSDLG_H_
