///////////////////////////////////////////////////////////////////////////////
// Name:        src/gtk/nativewin.cpp
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

#include <gtk/gtk.h>

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// wxNativeWindow
// ----------------------------------------------------------------------------

bool
wxNativeWindow::Create(wxWindow* parent,
                       wxWindowID winid,
                       wxNativeWindowHandle widget)
{
    wxCHECK_MSG( widget, false, wxS("Invalid null GtkWidget") );

    // Standard wxGTK controls use PreCreation() but we never have any size
    // specified at this stage, so don't bother with it.
    if ( !CreateBase(parent, winid) )
        return false;

    // Add a reference to the widget to match g_object_unref() in wxWindow dtor
    // (and by using the "_sink" version we avoid memory leaks when we're
    // passed a newly allocated widget, as is typically the case).
    m_widget = widget;
    g_object_ref_sink(m_widget);

    parent->DoAddChild(this);

    PostCreation();

    // Ensure that the best (and minimal) size is set to fully display the
    // widget.
    GtkRequisition req;
#ifdef __WXGTK3__
    gtk_widget_get_preferred_size(widget, NULL, &req);
#else
    gtk_widget_size_request(widget, &req);
#endif
    SetInitialSize(wxSize(req.width, req.height));

    return true;
}

#endif // !wxCHECK_VERSION(3,1,0)
