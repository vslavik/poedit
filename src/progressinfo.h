/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 2000-2021 Vaclav Slavik
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

#ifndef Poedit_progressinfo_h
#define Poedit_progressinfo_h

#include "concurrency.h"
#include "titleless_window.h"

#include <wx/string.h>
#include <wx/windowptr.h>

#include <memory>

class WXDLLIMPEXP_FWD_CORE wxStaticBitmap;
class WXDLLIMPEXP_FWD_CORE wxStaticText;
class WXDLLIMPEXP_FWD_CORE wxGauge;
class SecondaryLabel;


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


/// Window showing progress of a long-running task
class ProgressWindow : public TitlelessDialog, public ProgressObserver
{
protected:
    ProgressWindow(wxWindow *parent, const wxString& title,
                   dispatch::cancellation_token_ptr cancellationToken = dispatch::cancellation_token_ptr());

    template<typename TBackgroundJob, typename TCompletion>
    void DoRunTask(const TBackgroundJob& task, const TCompletion& completionHandler, bool forceModal = false)
    {
        m_progress = std::make_shared<Progress>(1);
        attach(*m_progress);

        // don't show any flushed log output until the modal progress window closes:
        // FIXME: proper UI for showing logged errors directly within
        wxLog::Suspend();

        auto bg = dispatch::async([=]
        {
            Progress progress(1, *m_progress, 1);
            task();
        })
        .then_on_main([=]
        {
            // make sure EndModal() is only called within event loop, i.e. not before
            // Show*Modal() was called (which could happen if `bg` finished instantly):
            CallAfter([=]{
                EndModal(wxID_OK);
                wxLog::Resume();
            });
        });
        // TODO: handle exceptions (can't just rethrow on main thread)

        if (forceModal || !GetParent())
        {
            ShowModal();
            detach();
            m_progress.reset();
            completionHandler();
        }
        else
        {
            ShowWindowModalThenDo([=](int /*retcode*/)
            {
                detach();
                m_progress.reset();
                completionHandler();
            });
        }
    }

public:
    template<typename TBackgroundJob, typename TCompletion>
    static void RunTaskThenDo(wxWindow *parent, const wxString& title,
                              const TBackgroundJob& task, const TCompletion& completionHandler)
    {
        wxWindowPtr<ProgressWindow> window(new ProgressWindow(parent, title));
        window->DoRunTask(task, [=]{
            completionHandler();
            window->Hide();
        });
    }

    template<typename TBackgroundJob, typename TCompletion>
    static void RunCancellableTaskThenDo(wxWindow *parent, const wxString& title,
                                         const TBackgroundJob& task, const TCompletion& completionHandler)
    {
        auto token = std::make_shared<dispatch::cancellation_token>();
        wxWindowPtr<ProgressWindow> window(new ProgressWindow(parent, title, token));
        window->DoRunTask([=]{
            task(token);
        },
        [=]{
            completionHandler(!token->is_cancelled());
            window->Hide();
        });
    }

    template<typename TBackgroundJob>
    static void RunTask(wxWindow *parent, const wxString& title,
                        const TBackgroundJob& task)
    {
        wxWindowPtr<ProgressWindow> window(new ProgressWindow(parent, title));
        window->DoRunTask(task, [=]{}, true);
    }

    template<typename TBackgroundJob>
    static bool RunCancellableTask(wxWindow *parent, const wxString& title,
                                   const TBackgroundJob& task)
    {
        auto token = std::make_shared<dispatch::cancellation_token>();
        wxWindowPtr<ProgressWindow> window(new ProgressWindow(parent, title, token));
        window->DoRunTask([=]{ task(token); }, []{}, true);
        return !token->is_cancelled();
    }

    void update_message(const wxString& text) override;
    void update_progress(double completedFraction) override;

    void UpdateMessage(const wxString& text); // TODO: remove

protected:
    const int PROGRESS_BAR_RANGE = 100;

    void OnCancel(wxCommandEvent&);

private:
    std::shared_ptr<Progress> m_progress;

    wxStaticBitmap *m_image;
    wxStaticText *m_title;
    SecondaryLabel *m_message;
    wxGauge *m_gauge;

    dispatch::cancellation_token_ptr m_cancellationToken;
};


#endif // Poedit_progressinfo_h
