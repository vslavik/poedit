
/*

    poedit, a wxWindows i18n catalogs editor

    ---------------
      summarydlg.h
    
      Catalog update summary dialog
    
      (c) Vaclav Slavik, 2000

*/


#if defined(__GNUG__) && !defined(__APPLE__)
#pragma interface
#endif

#ifndef _SUMMARYDLG_H_
#define _SUMMARYDLG_H_

#include <wx/string.h>
#include <wx/dialog.h>


/** This class provides simple dialog that displays list
  * of changes made in the catalog.
  */
class MergeSummaryDialog : public wxDialog
{
    public:
        MergeSummaryDialog(wxWindow *parent = NULL);
        ~MergeSummaryDialog();

        /** Reads data from catalog and fill dialog's controls.
            \param snew      list of strings that are new to the catalog
            \param sobsolete list of strings that no longer appear in the
                             catalog (as compared to catalog's state before
                             parsing sources).
         */
        void TransferTo(const wxArrayString& snew, 
                        const wxArrayString& sobsolete);
};



#endif // _SUMMARYDLG_H_
