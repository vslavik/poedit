
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
class WXDLLEXPORT wxConfigBase;


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
            compilation time and equals configure's --prefix argument
            (unless $POEDIT_PREFIX environment variable exists, in which
            case its content is returned).
         */
        wxString GetAppPath() const;

        /// Returns poEdit version string.
        wxString GetAppVersion() const;
        
    protected:
        /** Sets default values to configuration items that don't
            have anything set. (This may happen after fresh installation or
            upgrade to new version.)
         */
        void SetDefaultCfg(wxConfigBase *cfg);
};

DECLARE_APP(poEditApp);


#endif // _EDAPP_H_
