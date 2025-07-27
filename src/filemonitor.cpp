/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 2021-2025 Vaclav Slavik
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
#include "str_helpers.h"

#include <wx/fswatcher.h>

#include <algorithm>
#include <memory>
#include <vector>


#ifdef __WXOSX__

// We can't use wxFileSystemWatcher, because it monitors the directory, not the file, and that
// triggers scary warnings on macOS if the directory is ~/Desktop, ~/Downloads etc.
// So instead, use a minimal sufficient NSFilePresenter for the monitoring

#include <Foundation/Foundation.h>

@interface POFilePresenter : NSObject<NSFilePresenter>
@property(readwrite, copy) NSURL* presentedItemURL;
@property(readwrite, assign) NSOperationQueue* presentedItemOperationQueue;
@property FileMonitor::Impl* owner;

-(instancetype)initWithOwner:(FileMonitor::Impl*)owner URL:(NSURL*)url;
@end

class FileMonitor::Impl
{
public:
    Impl(const wxFileName& fn)
    {
        m_path = fn.GetFullPath();
        NSURL *url = [NSURL fileURLWithPath:str::to_NS(m_path)];
        m_presenter = [[POFilePresenter alloc] initWithOwner:this URL:url];
        [NSFileCoordinator addFilePresenter:m_presenter];
    }

    ~Impl()
    {
        m_presenter.owner = nullptr;
        [NSFileCoordinator removeFilePresenter:m_presenter];
        m_presenter = nil;
    }

    void OnChanged()
    {
        FileMonitor::NotifyFileChanged(m_path);
    }

private:
    wxString m_path;
    POFilePresenter *m_presenter;
};

@implementation POFilePresenter

-(instancetype)initWithOwner:(FileMonitor::Impl*)owner URL:(NSURL*)url
{
    self = [super init];
    if (self)
    {
        self.owner = owner;
        self.presentedItemURL = url;
        self.presentedItemOperationQueue = NSOperationQueue.mainQueue;
    }
    return self;
}

- (void)presentedItemDidChange
{
    if (self.owner)
        self.owner->OnChanged();
}

@end

#else // !__WXOSX__

namespace
{

const int MONITORING_FLASG = wxFSW_EVENT_CREATE | wxFSW_EVENT_RENAME | wxFSW_EVENT_MODIFY;

class FSWatcher
{
public:
    static auto Get()
    {
        static bool s_initialized = false;
        if (!ms_instance)
        {
            wxCHECK_MSG(!s_initialized, ms_instance, "using FSWatcher after cleanup");
            
            ms_instance.reset(new FSWatcher);
            s_initialized = true;
        }
        return ms_instance;
    }

    void Add(const wxFileName& dir)
    {
        if (m_watcher)
        {
            m_watcher->Add(dir, MONITORING_FLASG);
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
            m_watcher->Remove(dir);
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
            FileMonitor::NotifyFileChanged(fn.GetFullPath());
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

    static std::shared_ptr<FSWatcher> ms_instance;
};

std::shared_ptr<FSWatcher> FSWatcher::ms_instance;

} // anonymous namespace

class FileMonitor::Impl
{
public:
    Impl(const wxFileName& fn)
    {
        m_dir = wxFileName::DirName(fn.GetPath());
        auto watcher = FSWatcher::Get();
        if (watcher)
            watcher->Add(m_dir);
        m_watcher = watcher;
    }

    ~Impl()
    {
        // we may be being destroyed after FSWatcher::CleanUp was called, in which
        // case nothing needs to be done
        if (auto watcher = m_watcher.lock())
            watcher->Remove(m_dir);
    }

private:
    wxFileName m_dir;
    std::weak_ptr<FSWatcher> m_watcher;
};

#endif // !__WXOSX__


void FileMonitor::EventLoopStarted()
{
#ifndef __WXOSX__
    auto watcher = FSWatcher::Get();
    if (watcher)
        watcher->EventLoopStarted();
#endif
}

void FileMonitor::CleanUp()
{
#ifndef __WXOSX__
    FSWatcher::CleanUp();
#endif
}


FileMonitor::FileMonitor() : m_isRespondingGuard(false)
{
}

FileMonitor::~FileMonitor()
{
    Reset();
}

void FileMonitor::SetFile(wxFileName file)
{
    // unmonitor first (needed even if the filename didn't change)
    if (file == m_file)
    {
        m_loadTime = m_file.GetModificationTime();
        return;
    }

    Reset();

    m_file = file;
    if (!m_file.IsOk())
        return;

    m_impl = std::make_unique<Impl>(m_file);
    m_loadTime = m_file.GetModificationTime();
}

void FileMonitor::Reset()
{
    m_impl.reset();
    m_file.Clear();
}

void FileMonitor::NotifyFileChanged(const wxString& path)
{
    auto window = PoeditFrame::Find(path);
    if (window)
        window->ReloadFileIfChanged();
}

