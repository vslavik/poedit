
/*

    poedit, a wxWindows i18n catalogs editor

    ---------------
      gexecute.h
    
      Gettext execution code
    
      (c) Vaclav Slavik, 2000

*/


#ifdef __GNUG__
#pragma interface
#endif

#ifndef _GEXECUTE_H_
#define _GEXECUTE_H_

class wxString;


// executes command, logs stderr output and returns success
extern bool ExecuteGettext(const wxString& cmdline);

#endif // _GEXECUTE_H_
