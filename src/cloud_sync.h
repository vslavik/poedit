/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 2017 Vaclav Slavik
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

#include <wx/string.h>

#if wxUSE_GUI
    #include "customcontrols.h"
    #include "errors.h"
    #include "hidpi.h"

    #include <wx/dialog.h>
    #include <wx/msgdlg.h>
    #include <wx/sizer.h>
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

    /// Name of the destionation (e.g. Crowding or hostname)
    virtual wxString GetName() const = 0;

    virtual bool NeedsMO() const = 0;

    /// Asynchronously uploads the file. Returned future throws on error.
    virtual dispatch::future<void> Upload(CatalogPtr file) = 0;


    /// Convenicence for creating a destination from a lambda.
    template<typename F>
    static std::shared_ptr<CloudSyncDestination> Make(const wxString& name, bool needsMO, F&& func)
    {
        class Dest : public CloudSyncDestination
        {
        public:
            Dest(const wxString& name_, bool needsMO_, F&& func_)
                : name(name_), needsMO(needsMO_), func(func_) {}

            wxString GetName() const override { return name; }
            bool NeedsMO() const override { return needsMO; }
            dispatch::future<void> Upload(CatalogPtr file) override { return func(file); }

            wxString name;
            bool needsMO;
            F func;
        };

        return std::make_shared<Dest>(name, needsMO, std::move(func));
    }
};


#if wxUSE_GUI

class CloudSyncProgressWindow : public wxDialog
{
public:
    CloudSyncProgressWindow(wxWindow *parent, const wxString& title = _("Syncing"))
        : wxDialog(parent, wxID_ANY, title, wxDefaultPosition, wxDefaultSize, wxCAPTION | wxSYSTEM_MENU)
    {
        auto sizer = new wxBoxSizer(wxVERTICAL);
        sizer->SetMinSize(PX(300), -1);
        Activity = new ActivityIndicator(this);
        sizer->AddStretchSpacer();
        sizer->Add(Activity, wxSizerFlags().Expand().Border(wxALL, PX(25)));
        sizer->AddStretchSpacer();
        SetSizerAndFit(sizer);
        CenterOnParent();
    }

    CloudSyncProgressWindow(wxWindow *parent, std::shared_ptr<CloudSyncDestination> dest)
        : CloudSyncProgressWindow(parent)
    {
        // TRANSLATORS: %s is a cloud destination, e.g. "Crowdin" or ftp.wordpress.com etc.
        Activity->Start(wxString::Format(_(L"Syncing with %s…"), dest->GetName()));
    }

    ActivityIndicator *Activity;

    /// Show the window while performing background sync action. Show error if 
    static void RunSync(wxWindow *parent, std::shared_ptr<CloudSyncDestination> dest, CatalogPtr file)
    {
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
                    wxString::Format(_("Syncing with %s failed."), dest->GetName()),
                    _("Syncing error"),
                    wxOK | wxICON_ERROR
                ));
            err->SetExtendedMessage(DescribeCurrentException());
            err->ShowWindowModalThenDo([err](int){});
        }
    }
};

#endif // wxUSE_GUI

#endif // Poedit_cloud_sync_h
