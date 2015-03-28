///////////////////////////////////////////////////////////////////////////////
// Name:        wx/activityindicator.h
// Purpose:     wxActivityIndicator declaration.
// Author:      Vadim Zeitlin
// Created:     2015-03-05
// Copyright:   (c) 2015 Vadim Zeitlin <vadim@wxwidgets.org>
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

#ifndef _WX_ACTIVITYINDICATOR_H_
#define _WX_ACTIVITYINDICATOR_H_

#include "wx/defs.h"

#include "wx/control.h"

#define wxActivityIndicatorNameStr wxS("activityindicator")

// ----------------------------------------------------------------------------
// wxActivityIndicator: small animated indicator of some application activity.
// ----------------------------------------------------------------------------

class wxActivityIndicatorBase : public wxControl
{
public:
    // Start or stop the activity animation (it is stopped initially).
    virtual void Start() = 0;
    virtual void Stop() = 0;

    // Return true if the control is currently showing activity.
    virtual bool IsRunning() const = 0;

    // Override some base class virtual methods.
    virtual bool AcceptsFocus() const { return false; }
    virtual bool HasTransparentBackground() { return true; }

protected:
    // choose the default border for this window
    virtual wxBorder GetDefaultBorder() const { return wxBORDER_NONE; }
};

#ifndef __WXUNIVERSAL__
#if defined(__WXGTK20__)
    #define wxHAS_NATIVE_ACTIVITYINDICATOR
    #include "gtk_activityindicator.h"
#elif defined(__WXOSX_COCOA__)
    #define wxHAS_NATIVE_ACTIVITYINDICATOR
    #include "osx_activityindicator.h"
#endif
#endif // !__WXUNIVERSAL__

#ifndef wxHAS_NATIVE_ACTIVITYINDICATOR
    #include "generic_activityindicator.h"
#endif

#endif // _WX_ACTIVITYINDICATOR_H_
