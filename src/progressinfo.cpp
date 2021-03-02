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

#include "progressinfo.h"

#include "customcontrols.h"
#include "hidpi.h"
#include "utility.h"

#include <atomic>
#include <mutex>
#include <set>

#include <wx/app.h>
#include <wx/dialog.h>
#include <wx/evtloop.h>
#include <wx/log.h>
#include <wx/gauge.h>
#include <wx/stattext.h>
#include <wx/dialog.h>
#include <wx/sizer.h>
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


ProgressWindow::ProgressWindow(wxWindow *parent, const wxString& title, dispatch::cancellation_token_ptr cancellationToken)
    : TitlelessDialog(parent, wxID_ANY, title, wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE & ~wxCLOSE_BOX)
{
    m_cancellationToken = cancellationToken;

    auto sizer = new wxBoxSizer(wxVERTICAL);
    auto topsizer = new wxBoxSizer(wxHORIZONTAL);
    sizer->Add(topsizer, wxSizerFlags(1).Expand().Border(wxALL, PX(20)));

    wxSize logoSize(PX(64), PX(64));
#if defined(__WXMSW__)
    wxIcon logo;
    logo.LoadFile("appicon", wxBITMAP_TYPE_ICO_RESOURCE, 256, 256);
    {
        wxBitmap bmp;
        bmp.CopyFromIcon(logo);
        logo.CopyFromBitmap(wxBitmap(bmp.ConvertToImage().Scale(logoSize.x, logoSize.y, wxIMAGE_QUALITY_BICUBIC)));
    }
#elif defined(__WXGTK__)
    auto logo = wxArtProvider::GetIcon("net.poedit.Poedit", wxART_FRAME_ICON, logoSize);
#else
    auto logo = wxArtProvider::GetBitmap("Poedit");
#endif
    m_image = new wxStaticBitmap(this, wxID_ANY, logo, wxDefaultPosition, logoSize);
    m_image->SetMinSize(logoSize);
    topsizer->Add(m_image, wxSizerFlags().Center().Border(wxTOP, MSW_OR_OTHER(PX(10), 0)));

    auto infosizer = new wxBoxSizer(wxVERTICAL);
    topsizer->Add(infosizer, wxSizerFlags().Center().Border(wxLEFT, PX(10)));

    m_title = new wxStaticText(this, wxID_ANY, title);
#ifdef __WXMSW__
    auto titleFont = m_title->GetFont().Scaled(1.3f);
#else
    auto titleFont = m_title->GetFont().Bold();
#endif
    m_title->SetFont(titleFont);
    infosizer->Add(m_title, wxSizerFlags().Left().Border(wxBOTTOM, PX(3)));
    m_message = new SecondaryLabel(this, "");
    infosizer->Add(m_message, wxSizerFlags().Left().Border(wxBOTTOM, PX(2)));
    m_gauge = new wxGauge(this, wxID_ANY, PROGRESS_BAR_RANGE, wxDefaultPosition, wxSize(PX(350), -1), wxGA_SMOOTH);
    m_gauge->Pulse();
    infosizer->Add(m_gauge, wxSizerFlags().Expand());

    if (cancellationToken)
    {
        auto cancelButton = new wxButton(this, wxID_CANCEL);
        cancelButton->Bind(wxEVT_BUTTON, &ProgressWindow::OnCancel, this);
        sizer->Add(cancelButton, wxSizerFlags().Right().Border(wxRIGHT|wxBOTTOM, PX(20)));
    }

    SetSizerAndFit(sizer);
    if (parent)
        CenterOnParent();
}

void ProgressWindow::update_message(const wxString& text)
{
    dispatch::on_main([=]
    {
        m_message->SetLabel(text);
    });
}

void ProgressWindow::update_progress(double completedFraction)
{
    dispatch::on_main([=]
    {
        auto value = std::min((int)std::lround(completedFraction * PROGRESS_BAR_RANGE), PROGRESS_BAR_RANGE);
        m_gauge->SetValue(value);
    });
}


void ProgressWindow::UpdateMessage(const wxString& text)
{
    if (m_cancellationToken && m_cancellationToken->is_cancelled())
        return;

    m_message->SetLabel(text);
    m_message->Refresh();
    m_message->Update();
}

void ProgressWindow::OnCancel(wxCommandEvent&)
{
    ((wxButton*)FindWindow(wxID_CANCEL))->Enable(false);
    UpdateMessage(_(L"Cancelling…"));
    m_cancellationToken->cancel();
}
