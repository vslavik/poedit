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

#ifndef Poedit_progressui_h
#define Poedit_progressui_h

#include "concurrency.h"
#include "errors.h"
#include "progress.h"
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
class WXDLLIMPEXP_FWD_CORE wxMessageDialog;
class ActivityIndicator;
class SecondaryLabel;


/// Structured result of a background task run under ProgressWindow
struct BackgroundTaskResult
{
    BackgroundTaskResult() {}
    BackgroundTaskResult(const wxString& summary_) : summary(summary_) {}

    explicit operator bool() const
    {
        return !summary.empty() || !details.empty() || user_data.has_value();
    }

    /// Larger text summary shown after finishing the task
    wxString summary;

    /**
        Further details (e.g. stats) about the operation. Each pair is a label-value pair,
        shown in a table. If the value is empty, the label is shown as wrapped text, i.e.
        can be longer.
     */
    std::vector<std::pair<wxString, wxString>> details;

    /// Additional user data for custom implementations of ProgressWindow::SetSummaryContent()
    std::any user_data;
};


/**
    Exception thrown by background tasks to indicate an error with additional details.

    When used in a task run under ProgressWindow, the exception will be caught and
    displayed as wxMessageDialog with "message" being the main, more generic message,
    and "details" being the extended explanation of the error.
 */
class BackgroundTaskException : public Exception
{
public:
    BackgroundTaskException(const wxString& message, const wxString& details = wxString())
        : Exception(message), m_details(details) {}

    const wxString& Details() const { return m_details; }

private:
    wxString m_details;
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

    The window may optionally be initialized with a cancellation token. If provided, the
    window will show a "Cancel" button and the RunTask* methods will return false if cancelled.

    Note that the background task may not be able to honor the cancellation request, e.g. when
    it is done too late in execution. To check if the job was cancelled, you should check RunTask*
    return value not @a cancellationToken state.

    If the task supports cancellation, it should indicate successful cancellation by either
    returning empty summary or throwing dispatch::cancellation_exception.
 */
class ProgressWindow : public TitlelessDialog, public ProgressObserver
{
public:
    /**
        Creates the progress window.

        @param parent             Parent window, if any.
        @param title              Title of the operation.
        @param cancellationToken  Cancellation token to use for the task. If provided, the window
                                  will show a "Cancel" button and the task should check the token
                                  for cancellation requests.
     */
    ProgressWindow(wxWindow *parent, const wxString& title,
                   dispatch::cancellation_token_ptr cancellationToken = dispatch::cancellation_token_ptr());

    /**
        Returns currently active window (or nullptr) for the current thread.
        Should only be used from within an active task.
     */
    static ProgressWindow *GetActive() { return ms_activeWindow; }

    /**
        Runs the task modally, i.e. blocking any other execution in the app.

        Returns true on success (no fatal errors, no cancellation).
     */
    template<typename TBackgroundJob>
    bool RunTaskModal(TBackgroundJob&& task)
    {
        bool retval = false;
        RunTaskTempl(std::move(task), [&](bool status){ retval = status; }, /*forceModal=*/true);
        return retval;
    }

    /**
        Runs the task window-modal (if given a parent) or app-modal (if not)

        @param completionHandler Lambda expression to call after the task completes.
                                 This can either take a bool argument (indicating success or
                                 failure-or-cancellation) or not, if you don't care about the result.
     */
    template<typename TBackgroundJob, typename TCompletion>
    void RunTaskThenDo(TBackgroundJob&& task, TCompletion&& completionHandler)
    {
        RunTaskTempl(std::move(task), std::move(completionHandler));
    }

    /**
        Sets custom error message to use as the "header" message in case of errors.
        Detailed errors are shown in the details.

        This message is only used if no summary window was shown.

        It is also overriden if a BackgroundTaskException is thrown.
     */
    void SetErrorMessage(const wxString& message) { m_errorMessage = message; }

protected:
    std::function<void(bool)> WrapCompletionHandler(std::function<void(bool)>&& f)
        { return f; }
    std::function<void(bool)> WrapCompletionHandler(std::function<void()>&& f)
        { return [f = std::move(f)](bool){ f(); /*ignore return value*/ }; }

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
            WrapCompletionHandler(completionHandler),
            forceModal
        );
    }

    void DoRunTask(std::function<BackgroundTaskResult()>&& task,
                   std::function<void(bool)>&& completionHandler,
                   bool forceModal);


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

    wxBoxSizer *GetDetailsSizer() { return m_detailsSizer; }
    wxBoxSizer *GetButtonSizer() { return m_buttonSizer; }

    void AddSummaryText(const wxString& text);
    wxBoxSizer *AddSummaryDetailLine();
    wxBoxSizer *AddSummaryDetailLine(const wxString& label, const wxString& value);

    wxWindowPtr<wxMessageDialog> CreateErrorDialog(const wxArrayString& errors);

    // ProgressObserver overrides:
    void update_message(const wxString& text) override;
    void update_progress(double completedFraction) override;

protected:
    const int PROGRESS_BAR_RANGE = 100;

    void OnCancel(wxCommandEvent&);

private:
    std::shared_ptr<Progress> m_progress;
    wxString m_errorMessage;

    wxStaticBitmap *m_image;
    wxStaticText *m_title;
    SecondaryLabel *m_progressMessage;
    wxGauge *m_gauge;
    wxBoxSizer *m_infoSizer = nullptr, *m_infoSummarySizer = nullptr, *m_errorsSizer = nullptr, *m_detailsSizer = nullptr, *m_buttonSizer = nullptr;
    wxButton *m_okButton = nullptr;
    wxButton *m_cancelButton = nullptr;
    ActivityIndicator *m_cancellationProgress = nullptr;

    dispatch::cancellation_token_ptr m_cancellationToken;

    static thread_local ProgressWindow *ms_activeWindow;
};


#endif // Poedit_progressui_h
