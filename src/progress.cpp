/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 2000-2026 Vaclav Slavik
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

#include "progress.h"

#include <atomic>
#include <mutex>
#include <set>


class Progress::impl
{
public:
    static constexpr double MIN_REPORTED_STEP = 0.01;

    impl(const impl&) = delete;
    impl(int totalCount, std::weak_ptr<impl> parent, int parentCountTaken)
        : m_parent(parent),
          m_observer(nullptr),
          m_totalCount(totalCount), m_parentCountTaken(parentCountTaken), m_completedCount(0),
          m_dirty(true), m_completedFraction(0), m_lastReportedFraction(-1)
    {
    }

    ~impl()
    {

    }

    void set_observer(ProgressObserver *observer) { m_observer = observer; }

    void message(const wxString& text)
    {
        if (m_observer)
            m_observer->update_message(text);

        if (auto p = parent())
            p->message(text);
    }

    void increment(int count)
    {
        m_completedCount += count;
        notify_changed();
    }

    void set(int count)
    {
        if (count == m_completedCount)
            return;

        m_completedCount = count;
        notify_changed();
    }

    void add_child(std::shared_ptr<impl> c)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_children.insert(c);
    }

    void remove_child(std::shared_ptr<impl> c)
    {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_children.erase(c);
            m_completedCount += c->m_parentCountTaken;
        }
        notify_changed();
    }

    std::shared_ptr<impl> parent() const { return m_parent.lock(); }

protected:
    void notify_changed()
    {
        m_dirty = true;
        if (auto p = parent())
            p->notify_changed();

        if (m_observer)
        {
            const double completed = completed_fraction();
            std::lock_guard<std::mutex> lock(m_mutex);
            if (completed == 1.0 || completed - m_lastReportedFraction >= MIN_REPORTED_STEP)
            {
                m_lastReportedFraction = completed;
                m_observer->update_progress(completed);
            }
        }
    }

    double completed_fraction()
    {
        // Note that the completed fraction may be inaccurate by the time the
        // calculation ends if an update happens in parallels. We're assuming
        // that's rare and just recalculate then in a loop.
        while (m_dirty)
        {
            m_dirty = false;

            std::lock_guard<std::mutex> lock(m_mutex);
            double completed = m_completedCount;
            for (auto c: m_children)
                completed += c->m_parentCountTaken * c->completed_fraction();
            m_completedFraction = std::min(completed / m_totalCount, 1.0);
        }

        return m_completedFraction;
    }

private:
    std::weak_ptr<impl> m_parent;
    std::set<std::shared_ptr<impl>> m_children;
    ProgressObserver *m_observer;

    int m_totalCount, m_parentCountTaken;
    std::atomic_int m_completedCount;
    std::atomic_bool m_dirty;
    std::atomic<double> m_completedFraction;
    double m_lastReportedFraction;

    std::mutex m_mutex;
};


thread_local std::weak_ptr<Progress::impl> Progress::ms_threadImplicitParent;

Progress::Progress(int totalCount)
{
    init(new impl(totalCount, ms_threadImplicitParent, 1));
}

Progress::Progress(int totalCount, Progress& parent, int parentCountTaken)
{
    init(new impl(totalCount, parent.m_impl, parentCountTaken));
}

void Progress::init(impl *implObj)
{
    m_impl.reset(implObj);
    if (auto p = m_impl->parent())
        p->add_child(m_impl);

    m_previousImplicitParent = ms_threadImplicitParent;
    ms_threadImplicitParent = m_impl;
}

Progress::~Progress()
{
    ms_threadImplicitParent = m_previousImplicitParent;
    if (auto p = m_impl->parent())
        p->remove_child(m_impl);

    // If any observer was set through this object, remove it because it is not reference counted:
    m_impl->set_observer(nullptr);
}

void Progress::message(const wxString& text)
{
    m_impl->message(text);
}

void Progress::increment(int count)
{
    m_impl->increment(count);
}

void Progress::set(int count)
{
    m_impl->set(count);
}

void ProgressObserver::attach(Progress& observedProgress)
{
    m_observedProgress = observedProgress.weak_from_this();
    observedProgress.m_impl->set_observer(this);
}

void ProgressObserver::detach()
{
    if (auto observed = m_observedProgress.lock())
    {
        observed->m_impl->set_observer(nullptr);
    }

    m_observedProgress.reset();
}

ProgressObserver::~ProgressObserver()
{
    detach();
}
