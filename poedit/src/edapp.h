
/*

    poedit, a wxWindows i18n catalogs editor

    ---------------
      edapp.h
    
      Editor application object
    
      (c) Vaclav Slavik, 2001

*/


#ifdef __GNUG__
#pragma implementation
#endif

#ifndef _EDAPP_H_
#define _EDAPP_H_

#include <wx/app.h>
#include <wx/string.h>

class poEditApp : public wxApp
{
    public:
        bool OnInit();
        
        wxString GetAppPath() const;
        
};

DECLARE_APP(poEditApp);


#endif // _EDAPP_H_
