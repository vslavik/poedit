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

#include "filemonitor.h"

#include "edframe.h"

#include <wx/fswatcher.h>

#include <algorithm>
#include <memory>
#include <vector>


namespace
{

const int MONITORING_FLASG = wxFSW_EVENT_CREATE | wxFSW_EVENT_RENAME | wxFSW_EVENT_MODIFY;

class FSWatcher
{
public:
    static FSWatcher& Get()
    {
        if (!ms_instance)
            ms_instance.reset(new FSWatcher);
        return *ms_instance;
    }

    void Add(const wxFileName& dir)
    {
        if (m_watcher)
        {
#ifdef __WXOSX__
            // kqueue-based monitoring is unreliable on macOS, we need to use AddTree() to force FSEvents usage
            m_watcher->AddTree(dir, MONITORING_FLASG);
#else
            m_watcher->Add(dir, MONITORING_FLASG);
#endif
        }
        else
        {
            m_pending.push_back(dir);
            return;
        }
    }

    void Remove(const wxFileName& dir)
    {
        if (m_watcher)
        {
#ifdef __WXOSX__
            m_watcher->RemoveTree(dir);
#else
            m_watcher->Remove(dir);
#endif
        }
        else
        {
            m_pending.erase(std::remove(m_pending.begin(), m_pending.end(), dir), m_pending.end());
        }
    }

    void EventLoopStarted()
    {
        if (m_watcher)
            return;  // already initiated

        m_watcher.reset(new wxFileSystemWatcher());
        m_watcher->Bind(wxEVT_FSWATCHER, [=](wxFileSystemWatcherEvent& event)
        {
            event.Skip();
            auto fn = event.GetNewPath();
            if (!fn.IsOk())
                return;
            auto window = PoeditFrame::Find(fn.GetFullPath());
            if (window)
                window->ReloadFileIfChanged();
        });

        for (auto& dir : m_pending)
            Add(dir);
        m_pending.clear();
    }

    static void CleanUp() { ms_instance.reset(); }

private:
    FSWatcher() {}

    std::vector<wxFileName> m_pending;
    std::unique_ptr<wxFileSystemWatcher> m_watcher;

    static std::unique_ptr<FSWatcher> ms_instance;
};

std::unique_ptr<FSWatcher> FSWatcher::ms_instance;

} // anonymous namespace


void FileMonitor::EventLoopStarted()
{
    FSWatcher::Get().EventLoopStarted();
}

void FileMonitor::CleanUp()
{
    FSWatcher::CleanUp();
}


void FileMonitor::SetFile(wxFileName file)
{
    // unmonitor first (needed even if the filename didn't change)xx
    if (file == m_file)
    {
        m_loadTime = m_file.GetModificationTime();
        return;
    }

    Reset();

    m_file = file;
    if (!m_file.IsOk())
        return;
    m_dir = wxFileName::DirName(m_file.GetPath());

    FSWatcher::Get().Add(m_dir);
    m_loadTime = m_file.GetModificationTime();
}

void FileMonitor::Reset()
{
    if (m_file.IsOk())
    {
        FSWatcher::Get().Remove(m_dir);
        m_file.Clear();
    }
}
