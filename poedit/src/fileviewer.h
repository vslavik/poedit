
/*

    poedit, a wxWindows i18n catalogs editor

    ---------------
      fileviewer.h
    
      Shows part of file around specified line
    
      (c) Vaclav Slavik, 2000

*/


#ifdef __GNUG__
#pragma interface
#endif

#ifndef _FILEVIEWER_H_
#define _FILEVIEWER_H_

#include <wx/frame.h>


/** This class implements frame that shows part of file (40 lines each way) 
    surrounding specified line.
 */
class FileViewer : public wxFrame
{
    public:
        FileViewer(wxWindow *parent, const wxString& filename, int linenum);
        ~FileViewer();
};



#endif // _FILEVIEWER_H_
