
/*

    poedit, a wxWindows i18n catalogs editor

    ---------------
      transmemupd_wizard.h
    
      Translation memory database update wizard
    
      (c) Vaclav Slavik, 2003

*/


#ifdef __GNUG__
#pragma interface
#endif

#ifndef _TRANSMEMUPD_WIZARD_H_
#define _TRANSMEMUPD_WIZARD_H_

#ifdef USE_TRANSMEM

class WXDLLEXPORT wxString;
class WXDLLEXPORT wxArrayString;
class WXDLLEXPORT wxWindow;

/** Runs a wizard to setup update of TM stored in \a dbPath with languages
    \a langs.
 */
void RunTMUpdateWizard(wxWindow *parent,
                       const wxString& dbPath, const wxArrayString& langs);

#endif // USE_TRANSMEM

#endif // _TRANSMEMUPD_WIZARD_H_
