
/*

    poedit, a wxWindows i18n catalogs editor

    ---------------
      gexecute.h
    
      Gettext execution code
    
      (c) Vaclav Slavik, 2000-2004

*/

#ifndef _GEXECUTE_H_
#define _GEXECUTE_H_

class WXDLLEXPORT wxString;
class WXDLLEXPORT wxEvtHandler;

struct GettextProcessData
{
        bool Running;
        int ExitCode;
        wxArrayString Stderr;
        wxArrayString Stdout;
};


/** Executes command. Writes stderr output to \a stderrOutput if not NULL,
    and logs it with wxLogError otherwise.
    \return true if program exited with exit code 0, false otherwise.
 */
extern bool ExecuteGettext(const wxString& cmdline,
                           wxString *stderrOutput = NULL);

/** Nonblocking version of the above -- upon termination, EVT_END_PROCESS
    event is delivered to \a parent and \a data are filled. */
extern bool ExecuteGettextNonblocking(const wxString& cmdline,
                                      GettextProcessData *data,
                                      wxEvtHandler *parent);

#endif // _GEXECUTE_H_
