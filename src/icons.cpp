/*
 *  This file is part of poEdit (http://www.poedit.org)
 *
 *  Copyright (C) 2004-2006 Vaclav Slavik
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
 *  $Id$
 *
 *  Art provider classes
 *
 */

#include <wx/log.h>

#include "icons.h"
#include "edapp.h"

#ifdef __UNIX__
#include "icons/appicon/poedit.xpm"
#endif

#ifdef __WXGTK20__
// translates poedit item id or Tango stock item id to "legacy" GNOME id:
static wxString GetGnomeStockId(const wxString& id)
{
    #define MAP(poedit, gnome) if ( id == _T(poedit) ) return _T(gnome)

    MAP("document-open",        "gtk-open");
    MAP("document-save",        "gtk-save");
    MAP("view-fullscreen",      "stock_fullscreen");
    MAP("help-browser",         "gtk-help");

    MAP("document-new",         "gtk-new");
    MAP("document-properties",  "stock_edit");
    MAP("edit-delete",          "gtk-delete");

    #undef MAP

    return wxEmptyString; // no match found
}
#endif // __WXGTK20__


// wxWidgets prior to 2.7.1 didn't have wxArtProvider::InsertProvider() and
// so it wasn't possibly to add a provider that would be used as the last
// resort. Unfortunately that's exactly what we need: use the stock (GTK2)
// provider to try to look up poedit icons in the theme (so that themes
// may override them) and only if that fails, use the Tango icons shipped
// with poedit. So we use a hack instead: temporarily disable this provider
// and re-run the lookup from its CreateBitmap, thus getting the bitmap from
// providers lower on the stack:
#if defined(HAS_THEMES_SUPPORT) && !defined(HAS_INSERT_PROVIDER)
    #define USE_THEME_HACK
#endif

#ifdef USE_THEME_HACK
static bool gs_disablePoeditProvider = false;
#endif


wxBitmap PoeditArtProvider::CreateBitmap(const wxArtID& id,
                                         const wxArtClient& client,
                                         const wxSize& size)
{
#ifdef USE_THEME_HACK
    if ( gs_disablePoeditProvider )
        return wxNullBitmap;

    // try to get the icon from theme provider:
    gs_disablePoeditProvider = true;
    wxBitmap hackbmp = wxArtProvider::GetBitmap(id, client, size);
    gs_disablePoeditProvider = false;
    if ( hackbmp.Ok() )
        return hackbmp;
#endif // USE_THEME_HACK

    wxLogTrace(_T("poedit.icons"), _T("getting icon '%s'"), id.c_str());

#ifdef __UNIX__
    if (id == _T("poedit"))
        return wxBitmap(appicon_xpm);
#endif

#ifdef __WXGTK20__
    // try legacy GNOME icons:
    wxString gnomeId = GetGnomeStockId(id);
    if ( !gnomeId.empty() )
    {
        wxLogTrace(_T("poedit.icons"), _T("-> legacy '%s'"), gnomeId.c_str());
        wxBitmap gbmp(wxArtProvider::GetBitmap(gnomeId, client, size));
        if ( gbmp.Ok() )
            return gbmp;
    }
#endif // __WXGTK20__

    wxString iconsdir = wxGetApp().GetAppPath() + _T("/share/poedit/icons");
    if ( !wxDirExists(iconsdir) )
        return wxNullBitmap;

    wxString icon;
    icon.Printf(_T("%s/%s.png"), iconsdir.c_str(), id.c_str());
    if ( !wxFileExists(icon) )
        return wxNullBitmap;

    wxLogTrace(_T("poedit.icons"), _T("loading from %s"), icon.c_str());
    wxBitmap bmp;
    bmp.LoadFile(icon, wxBITMAP_TYPE_ANY);
    return bmp;
}
