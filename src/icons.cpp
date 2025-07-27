/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 2004-2025 Vaclav Slavik
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
#include <wx/rawbmp.h>

#include <set>

#ifdef __WXGTK__
#include <gtk/gtk.h>
#endif

#include "icons.h"

#include "colorscheme.h"
#include "edapp.h"
#include "hidpi.h"
#include "utility.h"

#ifndef __WXOSX__

namespace
{

// Check if a given icon needs mirroring in right-to-left locales:
bool ShouldBeMirorredInRTL(const wxArtID& id, const wxArtClient& client)
{
    // Silence warning about unused parameter in some of the builds
    (void)client;

    static std::set<wxString> s_directional = {
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

void ProcessTemplateImage(wxImage& img, bool keepOpaque, bool inverted)
{
    int size = img.GetWidth() * img.GetHeight();

    ColorScheme::Mode inverseMode = inverted ? ColorScheme::Light : ColorScheme::Dark;
    if (ColorScheme::GetAppMode() == inverseMode)
    {
        auto rgb = img.GetData();
        for (int i = 0; i < 3*size; ++i, ++rgb)
            *rgb = 255 - *rgb;
    }

    // Apply 50% transparency
    if (!keepOpaque)
    {
        auto alpha = img.GetAlpha();
        for (int i = 0; i < size; ++i, ++alpha)
            *alpha /= 2;
    }
}

#ifdef __WXGTK3__
// FIXME: This is not correct, should use dedicated loading API instead
void ProcessSymbolicImage(wxBitmap& bmp)
{
    wxAlphaPixelData data(bmp);
    if (!data)
        return;

    const char color = (ColorScheme::GetAppMode() == ColorScheme::Light) ? 0 : 255;

    wxAlphaPixelData::Iterator p(data);
    for (int y = 0; y < data.GetHeight(); ++y)
    {
        wxAlphaPixelData::Iterator rowStart = p;
        for (int x = 0; x < data.GetWidth(); ++x, ++p)
        {
            if (p.Red() == 0xBE && p.Green() == 0xBE && p.Blue() == 0xBE)
                p.Red() = p.Green() = p.Blue() = color;
        }
        p = rowStart;
        p.OffsetY(data, 1);
    }
}
#endif // __WXGTK3__

} // anonymous namespace



PoeditArtProvider::PoeditArtProvider()
{
#ifdef __WXGTK3__
    gtk_icon_theme_prepend_search_path(gtk_icon_theme_get_default(), GetIconsDir().fn_str());
#endif
}


wxString PoeditArtProvider::GetIconsDir()
{
#if defined(__WXMSW__)
    return wxStandardPaths::Get().GetResourcesDir() + "\\Resources";
#else
    return wxStandardPaths::Get().GetInstallPrefix() + "/share/poedit/icons";
#endif
}


wxBitmap PoeditArtProvider::CreateBitmap(const wxArtID& id_,
                                         const wxArtClient& client,
                                         const wxSize& size)
{
    wxLogTrace("poedit.icons", "getting icon '%s'", id_.c_str());

    wxArtID id(id_);
    #define CHECK_FOR_VARIANT(name)                         \
        const bool name##Variant = id.Contains("@" #name);  \
        if (name##Variant)                                  \
            id.Replace("@" #name, "")
    CHECK_FOR_VARIANT(disabled);
    CHECK_FOR_VARIANT(opaque);
    CHECK_FOR_VARIANT(inverted);

    // Silence warning about unused parameter in some of the builds
    (void)client;
    (void)size;

    // Note: On Unix, this code is only called as last resort, if standard
    //       theme provider (that uses current icon theme and files from
    //       /usr/share/icons/<theme>) didn't find any matching icon.

#ifdef __WXGTK3__
    CHECK_FOR_VARIANT(symbolic);
    if (symbolicVariant)
    {
        wxBitmap bmp(wxArtProvider::GetBitmap(id + "-symbolic", client, size));
        if (bmp.Ok())
        {
            ProcessSymbolicImage(bmp);
            return bmp;
        }
    }
#endif

    auto iconsdir = GetIconsDir();
    if ( !wxDirExists(iconsdir) )
    {
        wxLogTrace("poedit.icons",
                   "icons dir %s not found", iconsdir.c_str());
        return wxNullBitmap;
    }

    wxString iconfile;
    iconfile.Printf("%s/%s", iconsdir, id);
    wxLogTrace("poedit.icons", "loading from %s", iconfile);
    ScaledImage icon;

    if (ColorScheme::GetAppMode() == ColorScheme::Dark)
        icon = LoadScaledBitmap(iconfile + "Dark");
    if (!icon.IsOk())
        icon = LoadScaledBitmap(iconfile);

    if (!icon.IsOk())
    {
        wxLogTrace("poedit.icons", "failed to load icon '%s'", id);
        return wxNullBitmap;
    }

    if (id.ends_with("Template"))
        ProcessTemplateImage(icon.image, opaqueVariant, invertedVariant);

    if (disabledVariant)
        icon.image = icon.image.ConvertToDisabled();

    if (wxTheApp->GetLayoutDirection() == wxLayout_RightToLeft && ShouldBeMirorredInRTL(id, client))
    {
        icon.image = icon.image.Mirror();
    }

#ifdef __WXMSW__
    if (client == wxART_TOOLBAR)
    {
        const int padding = PX(1);
        auto sz = icon.image.GetSize();
        sz.IncBy(padding * 2);
        icon.image.Resize(sz, wxPoint(padding, padding));
    }
#endif // __WXMSW__

    auto bitmap = wxBitmap(icon.image);
    bitmap.SetScaleFactor(icon.scale);
    return bitmap;
}

#endif // !__WXOSX__


wxBitmap GetPoeditAppIcon([[maybe_unused]]int pointSize)
{
#if defined(__WXMSW__)
    const auto scale = HiDPIScalingFactor();

    int loadSize;
    if (scale == 1.0)
        loadSize = pointSize;
    else if (scale == 2.0)
		loadSize = 2 * pointSize;
	else
		loadSize = 256;
    if (loadSize != 48 && loadSize != 64 && loadSize != 256)
        loadSize = 256; // 128px etc. is not present

    wxIcon appicon("appicon", wxBITMAP_TYPE_ICO_RESOURCE, loadSize, loadSize);

    if (scale == 1.0 && loadSize == pointSize)
    {
        return appicon;
    }
    else if (scale == 2.0 && loadSize == 2 * pointSize)
    {
        appicon.SetScaleFactor(scale);
        return appicon;
    }
    else
    {
        wxBitmap appbmp(appicon);
        appicon.CopyFromBitmap(wxBitmap(appbmp.ConvertToImage().Scale(PX(pointSize), PX(pointSize), wxIMAGE_QUALITY_BICUBIC)));
        appicon.SetScaleFactor(scale);
        return appicon;
    }
#elif defined(__WXOSX__)
    return wxBitmap(NSApplication.sharedApplication.applicationIconImage);
#elif defined(__WXGTK__)
    return wxArtProvider::GetIcon("net.poedit.Poedit", wxART_FRAME_ICON, wxSize(pointSize, pointSize));
#else
    return wxArtProvider::GetBitmap("Poedit");
#endif
}
