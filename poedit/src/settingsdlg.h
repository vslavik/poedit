
/*

    poedit, a wxWindows i18n catalogs editor

    ---------------
      settingsdlg.h
    
      Catalog settings dialog
    
      (c) Vaclav Slavik, 2000

*/


#ifdef __GNUG__
#pragma interface
#endif

#ifndef _SETTINGSDLG_H_
#define _SETTINGSDLG_H_

#include <wx/wx.h>
#include <wx/dialog.h>
#include <wx/notebook.h>

#include "catalog.h"

class WXDLLEXPORT wxEditableListBox;

// This class provides settings dialog for setting various
// catalog parameters

class SettingsDialog : public wxDialog
{
    public:
            SettingsDialog(wxWindow *parent = NULL);
            
            // Reads data from catalog and fill dialog's controls
            void TransferTo(Catalog *cat);
            
            // Save data from dialog to catalog
            void TransferFrom(Catalog *cat);
            
    private:
            wxTextCtrl *m_Team, *m_TeamEmail, *m_Project;            
            wxComboBox *m_Charset, *m_Language;
            wxTextCtrl *m_BasePath;
            wxEditableListBox *m_Paths, *m_Keywords;
};



#endif // _SETTINGSDLG_H_
