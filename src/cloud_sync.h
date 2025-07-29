/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 2017-2025 Vaclav Slavik
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

#ifndef Poedit_cloud_sync_h
#define Poedit_cloud_sync_h

#include "catalog.h"
#include "concurrency.h"
#include "titleless_window.h"

#include <wx/string.h>

#if wxUSE_GUI
    #include "errors.h"
    #include "hidpi.h"
    #include "utility.h"

    #include <wx/dialog.h>
    #include <wx/gauge.h>
    #include <wx/msgdlg.h>
    #include <wx/sizer.h>
    #include <wx/stattext.h>
    #include <wx/translation.h>
    #include <wx/windowptr.h>
#endif

#include <memory>


/**
    Abstract interface to cloud sync location for a file being edited.
    
    If associated with a Catalog instance, upon saving changes, they are
    automatically synced using this class' specialization.
 */
class CloudSyncDestination
{
public:
    virtual ~CloudSyncDestination() {}

    /// Name of the destination (e.g. Crowdin or hostname)
    virtual wxString GetName() const = 0;

    /// Asynchronously uploads the file. Returned future throws on error.
    virtual dispatch::future<void> Upload(CatalogPtr file) = 0;

    /// Ensures the user is authenticated with sync service, possibly showing
    /// login UI in the process.
    /// @return true if logged in, false if the user declined
    virtual bool AuthIfNeeded(wxWindow *parent) = 0;

    /// Convenicence for creating a destination from a lambda.
    template<typename F>
    static std::shared_ptr<CloudSyncDestination> Make(const wxString& name, F&& func)
    {
        class Dest : public CloudSyncDestination
        {
        public:
            Dest(const wxString& name_, F&& func_)
                : name(name_), func(func_) {}

            wxString GetName() const override { return name; }
            dispatch::future<void> Upload(CatalogPtr file) override { return func(file); }

            wxString name;
            F func;
        };

        return std::make_shared<Dest>(name, std::move(func));
    }
};


#if wxUSE_GUI

class CloudSyncProgressWindow : public TitlelessDialog
{
public:
    CloudSyncProgressWindow(wxWindow *parent, const wxString& title = _("Syncing"))
        : TitlelessDialog(parent, wxID_ANY, title, wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE & ~wxCLOSE_BOX)
    {
        m_message = new wxStaticText(this, wxID_ANY, title);
        auto gauge = new wxGauge(this, wxID_ANY, 100, wxDefaultPosition, wxSize(-1, MACOS_OR_OTHER(PX(4), PX(6))), wxGA_SMOOTH);

        auto sizer = new wxBoxSizer(wxVERTICAL);
        sizer->SetMinSize(PX(300), -1);
        sizer->AddSpacer(PX(20));
        sizer->Add(m_message, wxSizerFlags().Center().Border(wxLEFT|wxRIGHT, PX(80)));
        sizer->AddSpacer(PX(10));
        sizer->Add(gauge, wxSizerFlags().Expand().Border(wxLEFT|wxRIGHT, PX(40)));
        sizer->AddSpacer(PX(30));

        SetSizerAndFit(sizer);
        CenterOnParent();

        gauge->Pulse();
#ifdef __WXMSW__
        ::SendMessage(gauge->GetHandle(), PBM_SETMARQUEE, TRUE, 1); // make the pulsing faster
#endif
    }

    CloudSyncProgressWindow(wxWindow *parent, std::shared_ptr<CloudSyncDestination> dest)
          // TRANSLATORS: %s is a cloud destination, e.g. "Crowdin" or ftp.wordpress.com etc.
        : CloudSyncProgressWindow(parent, wxString::Format(_(L"Uploading translations to %s…"), dest->GetName()))
    {
    }

    void UpdateMessage(const wxString& msg)
    {
        m_message->SetLabel(msg);
        Layout();
        Refresh();
    }

    /// Show the window while performing background sync action. Show error if 
    static void RunSync(wxWindow *parent, std::shared_ptr<CloudSyncDestination> dest, CatalogPtr file)
    {
        if (!dest->AuthIfNeeded(parent))
        {
            return;
        }

        wxWindowPtr<CloudSyncProgressWindow> progress(new CloudSyncProgressWindow(parent, dest));
#ifdef __WXOSX__
        progress->ShowWindowModal();
#else
        progress->Show();
#endif

        auto future = dispatch::async([=]{ return dest->Upload(file); });
        for (;;)
        {
            if (future.wait_for(boost::chrono::milliseconds(10)) == dispatch::future_status::ready)
                break; // all is done
            wxYield();
        }

#ifdef __WXOSX__
        progress->EndModal(wxID_OK);
#else
        progress->Hide();
#endif

        try
        {
            future.get();
        }
        catch (...)
        {
            wxWindowPtr<wxMessageDialog> err(new wxMessageDialog
            (
                    parent,
                    // TRANSLATORS: %s is a cloud destination, e.g. "Crowdin" or ftp.wordpress.com etc.
                    wxString::Format(_("Uploading translations to %s failed."), dest->GetName()),
                    _("Syncing error"),
                    wxOK | wxICON_ERROR
                ));
            err->SetExtendedMessage(DescribeCurrentException());
            err->ShowWindowModalThenDo([err](int){});
        }
    }

private:
    wxStaticText *m_message;
};

#endif // wxUSE_GUI

#endif // Poedit_cloud_sync_h
