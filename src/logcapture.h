/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 2013-2021 Vaclav Slavik
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

#include <wx/log.h>
#include <wx/string.h>
#include <wx/thread.h>

// Capture all wx log output into a text buffer and suppress normal output:
class LogCapture : public wxLog
{
public:
    LogCapture(wxLogLevel level = wxLOG_Info)
    {
        m_active = true;
        m_oldLevel = wxLog::GetLogLevel();
        m_verbose = wxLog::GetVerbose();

        if (wxThread::IsMain())
            m_oldLogger = wxLog::SetActiveTarget(this);
        else
            m_oldLogger = wxLog::SetThreadActiveTarget(this);

        wxLog::SetLogLevel(level);
        wxLog::SetVerbose();
    }

    ~LogCapture()
    {
        Stop();
    }

    void Stop()
    {
        if (!m_active)
            return;
        m_active = false;

        if (wxThread::IsMain())
            wxLog::SetActiveTarget(m_oldLogger);
        else
            wxLog::SetThreadActiveTarget(m_oldLogger);

        wxLog::SetLogLevel(m_oldLevel);
        wxLog::SetVerbose(m_verbose);
    }

    void DoLogTextAtLevel(wxLogLevel, const wxString& msg) override
    {
        if (m_active)
            Append(msg);
    }

    void Append(const wxString& msg)
    {
        text << msg << "\n";
    }

    wxString text;

private:
    bool m_active;
    wxLog *m_oldLogger;
    wxLogLevel m_oldLevel;
    bool m_verbose;
};

#endif // Poedit_logcapture_h
