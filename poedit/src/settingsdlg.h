
/*

    poedit, a wxWindows i18n catalogs editor

    ---------------
      settingsdlg.h
    
      Catalog settings dialog
    
      (c) Vaclav Slavik, 2000

*/


#if defined(__GNUG__) && !defined(__APPLE__)
#pragma interface
#endif

#ifndef _SETTINGSDLG_H_
#define _SETTINGSDLG_H_

#include <wx/dialog.h>
#include <wx/notebook.h>

#include "catalog.h"

class WXDLLEXPORT wxEditableListBox;
class WXDLLEXPORT wxTextCtrl;
class WXDLLEXPORT wxComboBox;

/// Dialog setting various catalog parameters.
class SettingsDialog : public wxDialog
{
    public:
        SettingsDialog(wxWindow *parent = NULL);

        /// Reads data from the catalog and fill dialog's controls.
        void TransferTo(Catalog *cat);

        /// Saves data from the dialog to the catalog.
        void TransferFrom(Catalog *cat);
            
    private:
        wxTextCtrl *m_team, *m_teamEmail, *m_project;            
        wxComboBox *m_charset, *m_language, *m_country, *m_sourceCodeCharset;
        wxTextCtrl *m_basePath;
        wxEditableListBox *m_paths, *m_keywords;
};



#endif // _SETTINGSDLG_H_
