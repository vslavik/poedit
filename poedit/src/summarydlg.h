
/*

    poedit, a wxWindows i18n catalogs editor

    ---------------
      summarydlg.h
    
      Catalog update summary dialog
    
      (c) Vaclav Slavik, 2000

*/


#ifdef __GNUG__
#pragma interface
#endif

#ifndef _SUMMARYDLG_H_
#define _SUMMARYDLG_H_

#include <wx/string.h>
#include <wx/dialog.h>


// This class provides simple dialog that displays list
// of changes in catalog

class MergeSummaryDialog : public wxDialog
{
    public:
            MergeSummaryDialog(wxWindow *parent = NULL);
	    ~MergeSummaryDialog();
            
            // Reads data from catalog and fill dialog's controls
            void TransferTo(const wxArrayString& snew, const wxArrayString& sobsolete);
};



#endif // _SUMMARYDLG_H_
