
/*

    poedit, a wxWindows i18n catalogs editor

    ---------------
      gexecute.h
    
      Gettext execution code
    
      (c) Vaclav Slavik, 2000

*/

#ifndef _GEXECUTE_H_
#define _GEXECUTE_H_

class wxString;


/** Executes command. Logs stderr output with wxLogXXXX classes.
    \return true if program exited with exit code 0, false otherwise.
 */
extern bool ExecuteGettext(const wxString& cmdline);

#endif // _GEXECUTE_H_
