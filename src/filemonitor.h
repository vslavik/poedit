/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 2021 Vaclav Slavik
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


#ifndef Poedit_filemonitor_h
#define Poedit_filemonitor_h

#include "edapp.h"

#include <wx/filename.h>


class FileMonitor
{
public:
    FileMonitor() : m_isRespondingGuard(false) {}
    ~FileMonitor() { Reset(); }

    void SetFile(wxFileName file);

    bool WasModifiedOnDisk() const
    {
        if (!m_file.IsOk())
            return false;
        return m_loadTime != m_file.GetModificationTime();
    }

    // if true is returned, _must_ call StopRespondingToEvent() afterwards
    bool ShouldRespondToFileChange()
    {
        if (!m_file.IsOk() || m_isRespondingGuard)
            return false;

        if (!WasModifiedOnDisk())
            return false;

        m_isRespondingGuard = true;
        return true;
    }

    // logic for preventing multiple FS events from causing duplicate reloads
    void StopRespondingToEvent()
    {
        wxASSERT( m_isRespondingGuard );
        m_isRespondingGuard = false;
    }

    struct WritingGuard
    {
        WritingGuard(FileMonitor& monitor) : m_monitor(monitor)
        {
            m_monitor.m_isRespondingGuard = true;
        }

        ~WritingGuard()
        {
            m_monitor.StopRespondingToEvent();
        }

        FileMonitor& m_monitor;
    };

    static void EventLoopStarted();
    static void CleanUp();

private:
    void Reset();

private:
    bool m_isRespondingGuard;
    wxString m_monitoredPath;
    wxFileName m_file, m_dir;
    wxDateTime m_loadTime;
};


#endif // Poedit_filemonitor_h
