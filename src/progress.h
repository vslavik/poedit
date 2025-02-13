/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 2000-2025 Vaclav Slavik
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

#ifndef Poedit_progress_h
#define Poedit_progress_h

#include <wx/string.h>

#include <memory>



/**
    Accurate progress tracking for both simple and composed operations.

    Modelled after https://developer.apple.com/documentation/foundation/nsprogress
 */
class Progress
{
public:
    Progress(int totalCount);
    Progress(int totalCount, Progress& parent, int parentCountTaken);

    Progress(const Progress&) = delete;
    ~Progress();

    void message(const wxString& text);
    void increment(int count = 1);
    void set(int count);

private:
    class impl;
    std::shared_ptr<impl> m_impl;

    static thread_local std::weak_ptr<impl> ms_threadImplicitParent;
    std::weak_ptr<impl> m_previousImplicitParent;

    void init(impl *implObj);

    friend class ProgressObserver;
};


/**
    Base class for UI objects (window, progress bar) observing progress of
    some operation.
 */
class ProgressObserver
{
public:
    ProgressObserver() : m_observedProgress(nullptr) {}
    virtual ~ProgressObserver();

    void attach(Progress& observedProgress);
    void detach();

    virtual void update_message(const wxString& text) = 0;
    virtual void update_progress(double completedFraction) = 0;

private:
    Progress *m_observedProgress;
};

#endif // Poedit_progress_h
