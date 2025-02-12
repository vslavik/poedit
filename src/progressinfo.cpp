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

#include "progressinfo.h"

#include "customcontrols.h"
#include "hidpi.h"
#include "icons.h"
#include "utility.h"

#include <atomic>
#include <mutex>
#include <set>

#include <wx/app.h>
#include <wx/dialog.h>
#include <wx/evtloop.h>
#include <wx/log.h>
#include <wx/gauge.h>
#include <wx/msgdlg.h>
#include <wx/stattext.h>
#include <wx/dialog.h>
#include <wx/scopeguard.h>
#include <wx/sizer.h>
#include <wx/thread.h>
#include <wx/button.h>
#include <wx/config.h>


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
    m_observedProgress = &observedProgress;
    observedProgress.m_impl->set_observer(this);
}

void ProgressObserver::detach()
{
    if (m_observedProgress)
    {
        m_observedProgress->m_impl->set_observer(nullptr);
        m_observedProgress = nullptr;
    }
}

ProgressObserver::~ProgressObserver()
{
    detach();
}


namespace
{

class CapturingThreadLogger : public wxLog
{
public:
    CapturingThreadLogger(std::shared_ptr<wxArrayString> buffer) : m_buffer(buffer)
    {
        m_previous = wxLog::SetThreadActiveTarget(this);
    }

    ~CapturingThreadLogger()
    {
        wxLog::SetThreadActiveTarget(m_previous);
    }

protected:
    void DoLogRecord(wxLogLevel level, const wxString& msg, [[maybe_unused]] const wxLogRecordInfo& info) override
    {
        if (level <= wxLOG_Warning)
        {
            m_buffer->push_back(msg);
        }
        else if ( level == wxLOG_Debug || level == wxLOG_Trace )
        {
            wxLog::LogTextAtLevel(level, msg);
        }
        // else: ignore
    }

private:
    std::shared_ptr<wxArrayString> m_buffer;
    wxLog *m_previous;
};


wxWindowPtr<wxMessageDialog> CreateErrorDialog(wxWindow *parent, [[maybe_unused]] wxStaticText *title, const wxArrayString& errors)
{
    if (errors.empty())
        return {};

    wxString text;
    wxString extended;

    if (errors.size() == 1)
    {
        text = _("An error occurred.");
        extended = errors.front();
    }
    else
    {
        text = wxString::Format(wxPLURAL("%d error occurred.", "%d errors occurred.", (int)errors.size()), (int)errors.size());
        extended = wxJoin(errors, '\n', 0);
    }

    wxWindowPtr<wxMessageDialog> dlg(new wxMessageDialog(parent, text, MSW_OR_OTHER(title->GetLabel(), ""), wxOK | wxICON_ERROR));
    if (!extended.empty())
        dlg->SetExtendedMessage(extended);

    return dlg;
}

} // anonymous namespace


thread_local ProgressWindow *ProgressWindow::ms_activeWindow = nullptr;

ProgressWindow::ProgressWindow(wxWindow *parent, const wxString& title, dispatch::cancellation_token_ptr cancellationToken)
    : TitlelessDialog(parent, wxID_ANY, title, wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE & ~wxCLOSE_BOX)
{
    m_cancellationToken = cancellationToken;

    auto sizer = new wxBoxSizer(wxVERTICAL);
    auto topsizer = new wxBoxSizer(wxHORIZONTAL);
    sizer->Add(topsizer, wxSizerFlags(1).Expand().Border(wxALL, PX(20)));

    auto appicon = GetPoeditAppIcon(64);
    const wxSize logoSize(PX(64), PX(64));
    m_image = new wxStaticBitmap(this, wxID_ANY, appicon, wxDefaultPosition, logoSize);
    m_image->SetMinSize(logoSize);

    m_infoSizer = new wxBoxSizer(wxVERTICAL);

    m_title = new wxStaticText(this, wxID_ANY, title);
#ifdef __WXMSW__
    auto titleFont = m_title->GetFont().Scaled(1.3f);
#else
    auto titleFont = m_title->GetFont().Bold();
#endif
    m_title->SetFont(titleFont);
    m_infoSizer->Add(m_title, wxSizerFlags().Left().Border(wxBOTTOM, PX(3)));
    m_gauge = new wxGauge(this, wxID_ANY, PROGRESS_BAR_RANGE, wxDefaultPosition, wxSize(PX(350), -1), wxGA_SMOOTH);
    m_gauge->Pulse();
    m_infoSizer->Add(m_gauge, wxSizerFlags().Expand());
    m_progressMessage = new SecondaryLabel(this, "");
    m_infoSizer->Add(m_progressMessage, wxSizerFlags().Left().Border(wxTOP, MSW_OR_OTHER(PX(2), 0)));

    // Manually align the topsizer area to be centered on the icon. We do it like this because we'll want to add some
    // additional content to the bottom as part of summary display:
    int topMarginImage = MSW_OR_OTHER(PX(2), 0);
    int topMarginInfo = (m_image->GetMinSize().y - m_infoSizer->GetMinSize().y) / 2;
    if (topMarginInfo < 0)
    {
        topMarginImage += -topMarginInfo;
        topMarginInfo = 0;
    }

    topsizer->Add(m_image, wxSizerFlags().Top().Border(wxTOP, topMarginImage));
    topsizer->AddSpacer(PX(10));
    topsizer->Add(m_infoSizer, wxSizerFlags(1).Top().Border(wxTOP,topMarginInfo));

    m_buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    m_buttonSizer->AddStretchSpacer();
    sizer->Add(m_buttonSizer, wxSizerFlags().Expand().Border(wxRIGHT|wxBOTTOM, PX(20)));

    if (cancellationToken)
    {
        m_cancelButton = new wxButton(this, wxID_CANCEL);
        m_cancelButton->Bind(wxEVT_BUTTON, &ProgressWindow::OnCancel, this);
        m_buttonSizer->Add(m_cancelButton);
    }

    SetSizerAndFit(sizer);
    sizer->SetMinSize(sizer->GetSize()); // prevent resizing down later

    if (parent)
        CenterOnParent();
}


bool ProgressWindow::ShowSummary(const BackgroundTaskResult& data, const wxArrayString& errors)
{
    if (!data)
        return false;

    if (!SetSummaryContent(data))
        return false;

    if (!errors.empty())
    {
        wxString text;
        if (errors.size() == 1)
        {
            text = _("Error: ") + errors.front();
        }
        else
        {
            text = wxString::Format(wxPLURAL("%d error occurred:", "%d errors occurred:", (int)errors.size()), (int)errors.size());
            text += '\n';
            text += wxJoin(errors, '\n', 0);
        }

        auto errctrl = new SelectableAutoWrappingText(this, wxID_ANY, text);
        errctrl->SetForegroundColour(ColorScheme::Get(Color::ErrorText));
        m_infoSizer->Add(errctrl, wxSizerFlags().Expand().Border(wxTOP, PX(8)));
    }

    m_gauge->SetValue(PROGRESS_BAR_RANGE);
#ifdef __WXOSX__
    m_gauge->SetWindowVariant(wxWINDOW_VARIANT_SMALL);
#else
	auto gaugeHeight = m_gauge->GetSize().y;
    m_infoSizer->Hide(m_gauge);
    m_infoSizer->Insert(1, new wxStaticLine(this), wxSizerFlags().Expand().Border(wxTOP|wxBOTTOM, gaugeHeight / 2 - 1));
#endif
    m_infoSizer->Hide(m_progressMessage);

    if (m_cancelButton)
        m_buttonSizer->Hide(m_cancelButton);
    m_okButton = new wxButton(this, wxID_OK);
    m_buttonSizer->Add(m_okButton);
    m_okButton->SetDefault();
    m_okButton->SetFocus();

    auto sizer = GetSizer();
    sizer->Layout();
    sizer->SetSizeHints(this);

#ifdef __WXMSW__
    Refresh();
#endif

    return true;
}


void ProgressWindow::AddSummaryText(const wxString& text)
{
    auto summary = new AutoWrappingText(this, wxID_ANY, text);
    m_infoSizer->Add(summary, wxSizerFlags().Expand().Border(wxTOP, PX(2)));
}


wxBoxSizer *ProgressWindow::AddSummaryDetailLine()
{
    if (!m_detailsSizer)
    {
        m_detailsSizer = new wxBoxSizer(wxVERTICAL);
        m_infoSizer->Add(m_detailsSizer, wxSizerFlags().Expand().Border(wxTOP, PX(8)));
    }

    auto row = new wxBoxSizer(wxHORIZONTAL);
    m_detailsSizer->AddSpacer(PX(2));
    m_detailsSizer->Add(row, wxSizerFlags().Expand().Border(wxRIGHT, PX(2)));
    return row;
}


void ProgressWindow::AddSummaryDetailLine(const wxString& label, const wxString& value)
{
    auto row = AddSummaryDetailLine();

    row->Add(new SecondaryLabel(this, label), wxSizerFlags().Center());
    row->AddStretchSpacer();
    row->Add(new SecondaryLabel(this, value), wxSizerFlags().Center());
}


bool ProgressWindow::SetSummaryContent(const BackgroundTaskResult& data)
{
    bool done = false;

    if (!data.summary.empty())
    {
        AddSummaryText(data.summary);
        done = true;
    }

    if (!data.details.empty())
    {
        for (auto& line: data.details)
            AddSummaryDetailLine(line.first, line.second);
        done = true;
    }

    return done;
}


void ProgressWindow::DoRunTask(std::function<BackgroundTaskResult()>&& task,
                               std::function<void()>&& completionHandler,
                               bool forceModal)
{
    const bool runModally = forceModal || !GetParent();

    auto loggedErrors = std::make_shared<wxArrayString>();
    m_progress = std::make_shared<Progress>(1);
    attach(*m_progress);

    auto windowShownSemaphore = std::make_shared<wxSemaphore>();
    Bind(wxEVT_SHOW, [=](wxShowEvent& e){
        windowShownSemaphore->Post();
        e.Skip();
    });

    auto bg = dispatch::async([=, task = std::move(task)]
    {
        CapturingThreadLogger logger(loggedErrors);
        Progress progress(1, *m_progress, 1);

        // Wait until progress window is shown. This greatly simplifies handling of exceptional
        // states, showing summaries etc. below, because the window is guaranteed to be visible
        // and (window-)modally showing even if the task finishes immediately.
        windowShownSemaphore->Wait();

        ms_activeWindow = this;
        wxON_BLOCK_EXIT_SET(ms_activeWindow, nullptr);

        return task();
    })
    .then_on_main([=](BackgroundTaskResult result)
    {
        bool summaryShown = result ? ShowSummary(result, *loggedErrors) : false;
        if (summaryShown)
        {
            loggedErrors->clear();  // for handling below
        }
        else
        {
            EndModal(wxID_OK);
        }
    })
    .catch_all([=](dispatch::exception_ptr e){
        loggedErrors->push_back(DescribeException(e));
        EndModal(wxID_CANCEL);
    });

    if (runModally)
    {
        ShowModal();
        detach();
        m_progress.reset();
        Hide();

        if (!loggedErrors->empty())
        {
            auto error = CreateErrorDialog(GetParent(), m_title, *loggedErrors);
            error->ShowModal();
        }

        if (completionHandler)
            completionHandler();
    }
    else
    {
        ShowWindowModalThenDo([=, completionHandler = std::move(completionHandler)](int /*retcode*/)
        {
            detach();
            m_progress.reset();
            Hide();

            if (!loggedErrors->empty())
            {
                auto error = CreateErrorDialog(GetParent(), m_title, *loggedErrors);
                error->ShowWindowModalThenDo([completionHandler,error](int /*retcode*/){
                    if (completionHandler)
                        completionHandler();
                });
            }
            else
            {
                if (completionHandler)
                completionHandler();
            }
        });
    }
}


void ProgressWindow::update_message(const wxString& text)
{
    if (m_cancellationToken && m_cancellationToken->is_cancelled())
        return;

    dispatch::on_main([=]
    {
        m_progressMessage->SetLabel(text);
    });
}


void ProgressWindow::update_progress(double completedFraction)
{
    dispatch::on_main([=]
    {
        auto value = std::min((int)std::lround(completedFraction * PROGRESS_BAR_RANGE), PROGRESS_BAR_RANGE);
        if (m_cancellationToken && m_cancellationToken->is_cancelled())
            return; // don't update anymore
        m_gauge->SetValue(value);
    });
}


void ProgressWindow::UpdateMessage(const wxString& text)
{
    if (m_cancellationToken && m_cancellationToken->is_cancelled())
        return;

    m_progressMessage->SetLabel(text);
    m_progressMessage->Refresh();
    m_progressMessage->Update();
}

void ProgressWindow::OnCancel(wxCommandEvent&)
{
    ((wxButton*)FindWindow(wxID_CANCEL))->Enable(false);
    UpdateMessage(_(L"Cancelling…"));
    m_gauge->Pulse();
    m_cancellationToken->cancel();
}
