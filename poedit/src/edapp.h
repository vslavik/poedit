
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

/// wxApp for use with 
class poEditApp : public wxApp
{
    public:
        /** wxWin initalization hook. Shows poEditFrame and initializes
            configuration entries to default values if they were missing.
         */
        bool OnInit();
        /** Gets application's path. This path is used when looking for 
            resources.zip and help files, both of them can be found
            in {appPath}/share/poedit. Looks in registry under Windows
            and returns value of POEDIT_PREFIX which is supplied at
            compilation time and equals configure's --prefix argument.
            
            \todo Check for \$(POEDIT_HOME) under Unix to allow rellocatable
                  packages?
         */
        wxString GetAppPath() const;
};

DECLARE_APP(poEditApp);


#endif // _EDAPP_H_
