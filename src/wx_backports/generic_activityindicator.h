///////////////////////////////////////////////////////////////////////////////
// Name:        wx/generic/activityindicator.h
// Purpose:     Declaration of wxActivityIndicatorGeneric.
// Author:      Vadim Zeitlin
// Created:     2015-03-06
// Copyright:   (c) 2015 Vadim Zeitlin <vadim@wxwidgets.org>
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

#ifndef _WX_GENERIC_ACTIVITYINDICATOR_H_
#define _WX_GENERIC_ACTIVITYINDICATOR_H_

#ifndef wxHAS_NATIVE_ACTIVITYINDICATOR
    // This is the only implementation we have, so call it accordingly.
    #define wxActivityIndicatorGeneric wxActivityIndicator
#endif

// ----------------------------------------------------------------------------
// wxActivityIndicatorGeneric: built-in generic implementation.
// ----------------------------------------------------------------------------

class wxActivityIndicatorGeneric : public wxActivityIndicatorBase
{
public:
    wxActivityIndicatorGeneric()
    {
        m_impl = NULL;
    }

    explicit
    wxActivityIndicatorGeneric(wxWindow* parent,
                               wxWindowID winid = wxID_ANY,
                               const wxPoint& pos = wxDefaultPosition,
                               const wxSize& size = wxDefaultSize,
                               long style = 0,
                               const wxString& name = wxActivityIndicatorNameStr)
    {
        m_impl = NULL;

        Create(parent, winid, pos, size, style, name);
    }

    bool Create(wxWindow* parent,
                wxWindowID winid = wxID_ANY,
                const wxPoint& pos = wxDefaultPosition,
                const wxSize& size = wxDefaultSize,
                long style = 0,
                const wxString& name = wxActivityIndicatorNameStr);

    virtual ~wxActivityIndicatorGeneric();

    virtual void Start();
    virtual void Stop();
    virtual bool IsRunning() const;

protected:
    virtual wxSize DoGetBestClientSize() const;

private:
    class wxActivityIndicatorImpl *m_impl;

#ifndef wxHAS_NATIVE_ACTIVITYINDICATOR
    wxDECLARE_DYNAMIC_CLASS(wxActivityIndicator);
#endif
};

#endif // _WX_GENERIC_ACTIVITYINDICATOR_H_
