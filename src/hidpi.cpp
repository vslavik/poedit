/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 2015-2017 Vaclav Slavik
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

#include "hidpi.h"

#include <wx/bitmap.h>
#include <wx/dcscreen.h>
#include <wx/image.h>


#ifdef NEEDS_MANUAL_HIDPI

double g_pxScalingFactor = 1.0;

void InitHiDPIHandling()
{
    wxSize dpi = wxScreenDC().GetPPI();
    g_pxScalingFactor = dpi.y / 96.0;
}

#endif // NEEDS_MANUAL_HIDPI

namespace
{

void LoadPNGImage(wxImage& img, const wxString& filename)
{
    img.LoadFile(filename, wxBITMAP_TYPE_PNG);
    // wxImage doesn't load alpha from PNG if it could be expressed as a mask.
    // Too bad this breaks a) scaling and b) wxToolbar's disabled bitmaps.
    // Beat some sense into it:
    if (img.IsOk() && img.HasMask())
    {
        img.InitAlpha();
    }
}

} // anonymous namespace

wxImage LoadScaledBitmap(const wxString& name)
{
    const wxString filename(name + ".png");
    if (!wxFileExists(filename))
        return wxNullImage;

    wxImage img;

#ifdef NEEDS_MANUAL_HIDPI
    if (HiDPIScalingFactor() > 1.25)
    {
        double imgScale = HiDPIScalingFactor();
        const wxString filename_2x(name + "@2x.png");
        if (wxFileExists(filename_2x))
        {
            LoadPNGImage(img, filename_2x);
            if (HiDPIScalingFactor() == 2.0)
                return img;
            else
                imgScale /= 2.0;
        }
        else
        {
            LoadPNGImage(img, filename);
        }

        if (!img.IsOk())
            return wxNullImage;

        wxImageResizeQuality quality;
        if (imgScale == 2.0)
            quality = wxIMAGE_QUALITY_NEAREST;
        else if (imgScale == 1.5)
            quality = wxIMAGE_QUALITY_BILINEAR;
        else
            quality = wxIMAGE_QUALITY_BICUBIC;
        img.Rescale(img.GetWidth() * imgScale, img.GetHeight() * imgScale, quality);
        return img;
    }
    // else: load normally
#endif

    LoadPNGImage(img, filename);
    return img;
}
