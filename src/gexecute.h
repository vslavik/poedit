/*
 *  This file is part of poEdit (http://www.poedit.net)
 *
 *  Copyright (C) 2000-2005 Vaclav Slavik
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a
 *  copy of this software and associated documentation files (the "Software"),
 *  to deal in the Software without restriction, including without limitation
 *  the rights to use, copy, modify, merge, publish, distribute, sublicense,
 *  and/or sell copies of the Software, and to permit persons to whom the
 *  Software is furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in
 *  all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 *  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 *  DEALINGS IN THE SOFTWARE.
 *
 *  $Id$
 *
 *  Gettext execution code
 *
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
extern wxProcess *ExecuteGettextNonblocking(const wxString& cmdline,
                                            GettextProcessData *data,
                                            wxEvtHandler *parent);

#endif // _GEXECUTE_H_
