
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


/** Executes command. Writes stderr output to \a stderrOutput if not NULL,
    and logs it with wxLogError otherwise.
    \return true if program exited with exit code 0, false otherwise.
 */
extern bool ExecuteGettext(const wxString& cmdline,
                           wxString *stderrOutput = NULL);

#endif // _GEXECUTE_H_
