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

#ifndef Poedit_progressinfo_h
#define Poedit_progressinfo_h

#include "concurrency.h"
#include "errors.h"
#include "titleless_window.h"

#include <wx/string.h>
#include <wx/windowptr.h>

#include <any>
#include <functional>
#include <memory>

class WXDLLIMPEXP_FWD_CORE wxBoxSizer;
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


/// Structured result of a background task run under ProgressWindow
struct BackgroundTaskResult
{
    BackgroundTaskResult() {}
    BackgroundTaskResult(const wxString& summary_) : summary(summary_) {}

    explicit operator bool() const
    {
        return !summary.empty() || !details.empty() || user_data.has_value();
    }

    wxString summary;
    std::vector<std::pair<wxString, wxString>> details;
    std::any user_data;
};


/**
    Window showing progress of a long-running task.

    Must be used in conjuction with the static RunTaskXXX functions.

    The task may return BgTaskResult value to show a summary/result of execution. This may
    require overriding ShowSummary().

    The task may also throw an exception to indicate an error. This will be caught and
    shown to the user. If non-fatal errors happen during execution and are logged with
    wxLogError, they will be shown to the user as well (as part of summary, if shown,
    otherwise as separate window).
 */
class ProgressWindow : public TitlelessDialog, public ProgressObserver
{
protected:
    ProgressWindow(wxWindow *parent, const wxString& title,
                   dispatch::cancellation_token_ptr cancellationToken = dispatch::cancellation_token_ptr());

    void DoRunTask(std::function<BackgroundTaskResult()>&& task,
                   std::function<void()>&& completionHandler,
                   bool forceModal);

    template<typename TBackgroundJob, typename TCompletion>
    void RunTaskTempl(TBackgroundJob&& task,
                      TCompletion&& completionHandler,
                      bool forceModal = false)
    {
        DoRunTask
        (
            [task = std::move(task)]{
                if constexpr (std::is_same_v<std::invoke_result_t<decltype(task)>, void>)
                {
                    task();
                    return BackgroundTaskResult();
                }
                else
                {
                    return task();
                }
            },
            std::move(completionHandler),
            forceModal
        );
    }

    /**
        Shows summary of the task's run, as returned by the task function
        and puts the window into a state where the user can view it.

        By default, shows a string summary if present in @a data, otherwise
        shows nothing and closes the window right away.

        @return true if a summary was shown, false otherwise

        @see SetSummaryContent()
     */
    bool ShowSummary(const BackgroundTaskResult& data, const wxArrayString& errors);

    /**
        Adds summary content to the UI.

        By default, shows content of @a data if present, otherwise
        shows nothing and closes the window right away.

        @return true if a summary was added, false otherwise
     */
    virtual bool SetSummaryContent(const BackgroundTaskResult& data);

    // Helpers for creating summary UI:
    void AddSummaryText(const wxString& text);
    wxBoxSizer *AddSummaryDetailLine();
    void AddSummaryDetailLine(const wxString& label, const wxString& value);

public:
    /**
        Returns currently active window (or nullptr) for the current thread.
        Should only be used from within an active task.
     */
    static ProgressWindow *GetActive() { return ms_activeWindow; }

    template<typename TProgressWindowClass = ProgressWindow, typename TBackgroundJob, typename TCompletion>
    static void RunTaskThenDo(wxWindow *parent, const wxString& title,
                              TBackgroundJob&& task, TCompletion&& completionHandler)
    {
        wxWindowPtr<ProgressWindow> window(new TProgressWindowClass(parent, title));
        window->RunTaskTempl(task, [window, completionHandler = std::move(completionHandler)]{
            completionHandler();
            (void)window; // important to keep a reference and not destroy too early
        });
    }

    template<typename TProgressWindowClass = ProgressWindow, typename TBackgroundJob, typename TCompletion>
    static void RunCancellableTaskThenDo(wxWindow *parent, const wxString& title,
                                         TBackgroundJob&& task, const TCompletion& completionHandler)
    {
        auto token = std::make_shared<dispatch::cancellation_token>();
        wxWindowPtr<ProgressWindow> window(new TProgressWindowClass(parent, title, token));

        window->RunTaskTempl
        (
            [token, task = std::move(task)]{ return task(token); },
            [token, window, completionHandler = std::move(completionHandler)]{
                completionHandler(!token->is_cancelled());
                (void)window; // important to keep a reference and not destroy too early
            }
        );
    }

    template<typename TProgressWindowClass = ProgressWindow, typename TBackgroundJob>
    static void RunTask(wxWindow *parent, const wxString& title,
                        TBackgroundJob&& task)
    {
        wxWindowPtr<ProgressWindow> window(new TProgressWindowClass(parent, title));
        window->RunTaskTempl(task, nullptr, /*forceModal=*/true);
    }

    template<typename TProgressWindowClass = ProgressWindow, typename TBackgroundJob>
    static bool RunCancellableTask(wxWindow *parent, const wxString& title,
                                   TBackgroundJob&& task)
    {
        auto token = std::make_shared<dispatch::cancellation_token>();
        wxWindowPtr<ProgressWindow> window(new TProgressWindowClass(parent, title, token));
        window->RunTaskTempl([=]{ return task(token); }, nullptr, /*forceModal=*/true);
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
    SecondaryLabel *m_progressMessage;
    wxGauge *m_gauge;
    wxBoxSizer *m_infoSizer = nullptr, *m_detailsSizer = nullptr, *m_buttonSizer = nullptr;
    wxButton *m_okButton = nullptr;
    wxButton *m_cancelButton = nullptr;

    dispatch::cancellation_token_ptr m_cancellationToken;

    static thread_local ProgressWindow *ms_activeWindow;
};


#endif // Poedit_progressinfo_h
