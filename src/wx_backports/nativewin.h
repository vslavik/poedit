///////////////////////////////////////////////////////////////////////////////
// Name:        wx/nativewin.h
// Purpose:     classes allowing to wrap a native window handle
// Author:      Vadim Zeitlin
// Created:     2008-03-05
// Copyright:   (c) 2008 Vadim Zeitlin <vadim@wxwindows.org>
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

#ifndef _WX_NATIVEWIN_BACKPORT_H_
#define _WX_NATIVEWIN_BACKPORT_H_

// this file contains only differences between nativewin.h in wx 3.0 and 3.1+

#include "wx/nativewin.h"


#if defined(__WXMSW__)
    typedef HWND wxNativeWindowHandle;
    #define wxHAS_NATIVE_WINDOW
#elif defined(__WXGTK__)
    #include <gtk/gtk.h>
    typedef GtkWidget *wxNativeWindowHandle;
    #define wxHAS_NATIVE_WINDOW
#elif defined(__WXOSX_COCOA__)
    typedef NSView *wxNativeWindowHandle;
    #define wxHAS_NATIVE_WINDOW
#endif

#ifdef wxHAS_NATIVE_WINDOW

// ----------------------------------------------------------------------------
// wxNativeWindow: for using native windows inside wxWidgets windows
// ----------------------------------------------------------------------------

class wxNativeWindow : public wxWindow
{
public:
    // Default ctor, Create() must be called later to really create the window.
    wxNativeWindow() { }

    // Create a window from an existing native window handle.
    //
    // Notice that this ctor doesn't take the usual pos and size parameters,
    // they're taken from the window handle itself.
    //
    // Use GetHandle() to check if the creation was successful, it will return
    // 0 if the handle was invalid.
    wxNativeWindow(wxWindow* parent, wxWindowID winid, wxNativeWindowHandle handle)
    {
        Create(parent, winid, handle);
    }

    // Same as non-default ctor, but with a return code.
    bool Create(wxWindow* parent, wxWindowID winid, wxNativeWindowHandle handle);

private:
    wxDECLARE_NO_COPY_CLASS(wxNativeWindow);
};

#endif // wxHAS_NATIVE_WINDOW

#endif // _WX_NATIVEWIN_BACKPORT_H_

