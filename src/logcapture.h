/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 2013-2017 Vaclav Slavik
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
 */

#ifndef Poedit_logcapture_h
#define Poedit_logcapture_h

#include <wx/string.h>
#include <wx/log.h>

// Capture all wx log output into a text buffer and suppress normal output:
class LogCapture : public wxLogInterposer
{
public:
    LogCapture(wxLogLevel level = wxLOG_Info)
    {
        PassMessages(false);
        m_oldLevel = wxLog::GetLogLevel();
        m_verbose = wxLog::GetVerbose();

        wxLog::SetLogLevel(level);
        wxLog::SetVerbose();
    }

    ~LogCapture()
    {
        wxLog::SetLogLevel(m_oldLevel);
        wxLog::SetVerbose(m_verbose);
    }

    virtual void DoLogTextAtLevel(wxLogLevel, const wxString& msg)
    {
        Append(msg);
    }

    void Append(const wxString& msg)
    {
        text << msg << "\n";
    }

    wxString text;

private:
    wxLogLevel m_oldLevel;
    bool m_verbose;
};

#endif // Poedit_logcapture_h
