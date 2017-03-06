/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 2004-2017 Vaclav Slavik
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

#include <wx/log.h>
#include <wx/stdpaths.h>
#include <wx/image.h>

#include <set>

#include "icons.h"
#include "edapp.h"
#include "hidpi.h"
#include "utility.h"

#ifndef __WXOSX__

namespace
{

#ifdef __WXGTK20__
// translates poedit item id or Tango stock item id to "legacy" GNOME id:
wxString GetGnomeStockId(const wxString& id)
{
    #define MAP(poedit, gnome) if ( id == _T(poedit) ) return _T(gnome)

    MAP("document-open",        "gtk-open");
    MAP("document-save",        "gtk-save");
    MAP("window-close",         "gtk-close");

    MAP("document-new",         "gtk-new");
    MAP("document-properties",  "stock_edit");
    MAP("edit-delete",          "gtk-delete");

    #undef MAP

    return wxEmptyString; // no match found
}
#endif // __WXGTK20__

// Check if a given icon needs mirroring in right-to-left locales:
bool ShouldBeMirorredInRTL(const wxArtID& id, const wxArtClient& client)
{
    // Silence warning about unused parameter in some of the builds
    (void)client;

    static std::set<wxString> s_directional = {
        "ContributeOn",
        "poedit-status-comment",
        "follow-link",
        "sidebar"
    };

    bool mirror = s_directional.find(id) != s_directional.end();

#ifdef __WXMSW__
    // Toolbar is already mirrored
    if (client == wxART_TOOLBAR)
        mirror = !mirror;
#endif

    return mirror;
}

} // anonymous namespace

wxBitmap PoeditArtProvider::CreateBitmap(const wxArtID& id_,
                                         const wxArtClient& client,
                                         const wxSize& size)
{
    wxLogTrace("poedit.icons", "getting icon '%s'", id_.c_str());

    wxArtID id(id_);
    const bool disabledVariant = id.Contains("@disabled");
    if (disabledVariant)
        id.Replace("@disabled", "");

    // Silence warning about unused parameter in some of the builds
    (void)client;
    (void)size;

    // Note: On Unix, this code is only called as last resort, if standard
    //       theme provider (that uses current icon theme and files from
    //       /usr/share/icons/<theme>) didn't find any matching icon.

#ifdef __WXGTK20__
    // try legacy GNOME icons from standard theme:
    wxString gnomeId = GetGnomeStockId(id);
    if ( !gnomeId.empty() )
    {
        wxLogTrace("poedit.icons", "-> legacy '%s'", gnomeId.c_str());
        wxBitmap gbmp(wxArtProvider::GetBitmap(gnomeId, client, size));
        if ( gbmp.Ok() )
            return gbmp;
    }
#endif // __WXGTK20__

    wxString iconsdir =
#if defined(__WXMSW__)
        wxStandardPaths::Get().GetResourcesDir() + "\\Resources";
#else
        wxStandardPaths::Get().GetInstallPrefix() + "/share/poedit/icons";
#endif
    if ( !wxDirExists(iconsdir) )
    {
        wxLogTrace("poedit.icons",
                   "icons dir %s not found", iconsdir.c_str());
        return wxNullBitmap;
    }

    wxString icon;
    icon.Printf("%s/%s", iconsdir, id);
    wxLogTrace("poedit.icons", "loading from %s", icon);
    wxImage img = LoadScaledBitmap(icon);
    if (!img.IsOk())
    {
        wxLogTrace("poedit.icons", "failed to load icon '%s'", id);
        return wxNullBitmap;
    }

    if (disabledVariant)
        img = img.ConvertToDisabled();

    if (wxTheApp->GetLayoutDirection() == wxLayout_RightToLeft && ShouldBeMirorredInRTL(id, client))
    {
        img = img.Mirror();
    }

#ifdef __WXMSW__
    if (IsWindows10OrGreater())
    {
        const int padding = PX(1);
        auto sz = img.GetSize();
        sz.IncBy(padding * 2);
        img.Resize(sz, wxPoint(padding, padding));
    }
#endif // __WXMSW__

    return wxBitmap(img);
}

#endif // !__WXOSX__
