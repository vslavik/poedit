
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


// This class provides preferences dialog for setting 
// user's identity 

class PreferencesDialog : public wxDialog
{
    public:
            PreferencesDialog(wxWindow *parent = NULL);
            
            // Reads data from config/registry and fill dialog's controls
            void TransferTo(wxConfigBase *cfg);
            
            // Save data from dialog to config/registry
            void TransferFrom(wxConfigBase *cfg);
            
    private:
            ParsersDB m_Parsers;

    private:
            DECLARE_EVENT_TABLE()
            
            void OnNewParser(wxCommandEvent& event);
            void OnEditParser(wxCommandEvent& event);
            void OnDeleteParser(wxCommandEvent& event);
            bool EditParser(int num);
};



#endif // _PREFSDLG_H_
