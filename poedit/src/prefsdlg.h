
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

#include <wx/wx.h>
#include <wx/dialog.h>
#include <wx/notebook.h>
#include <wx/config.h>

#include "parser.h"


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
        void OnNewParser(wxCommandEvent& event);
        void OnEditParser(wxCommandEvent& event);
        void OnDeleteParser(wxCommandEvent& event);
        
        /// Called to launch dialog for editting parser properties.
        bool EditParser(int num);
};


#endif // _PREFSDLG_H_
