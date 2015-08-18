///////////////////////////////////////////////////////////////////////////////
// Name:        src/msw/nativewin.cpp
// Purpose:     wxNativeWindow implementation
// Author:      Vadim Zeitlin
// Created:     2008-03-05
// Copyright:   (c) 2008 Vadim Zeitlin <vadim@wxwindows.org>
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

// for compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#if !wxCHECK_VERSION(3,1,0)

#include "nativewin.h"
#include "wx/msw/private.h"

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// wxNativeWindow
// ----------------------------------------------------------------------------

bool
wxNativeWindow::Create(wxWindow* parent,
                       wxWindowID winid,
                       wxNativeWindowHandle hwnd)
{
    wxCHECK_MSG( hwnd, false, wxS("Invalid null HWND") );
    wxCHECK_MSG( parent, false, wxS("Must have a valid parent") );
    wxASSERT_MSG( ::GetParent(hwnd) == GetHwndOf(parent),
                  wxS("The native window has incorrect parent") );

    const wxRect r = wxRectFromRECT(wxGetWindowRect(hwnd));

    // Skip wxWindow::Create() which would try to create a new HWND, we don't
    // want this as we already have one.
    if ( !CreateBase(parent, winid,
                     r.GetPosition(), r.GetSize(),
                     0, wxDefaultValidator, wxS("nativewindow")) )
        return false;

    parent->AddChild(this);

    SubclassWin(hwnd);

    if ( winid == wxID_ANY )
    {
        // We allocated a new ID to the control, use it at Windows level as
        // well because we assume that our and MSW IDs are the same in many
        // places and it seems prudent to avoid breaking this assumption.
        SetId(GetId());
    }
    else // We used a fixed ID.
    {
        // For the same reason as above, check that it's the same as the one
        // used by the native HWND.
        wxASSERT_MSG( ::GetWindowLong(hwnd, GWL_ID) == winid,
                      wxS("Mismatch between wx and native IDs") );
    }

    InheritAttributes();

    return true;
}

#endif // !wxCHECK_VERSION(3,1,0)
