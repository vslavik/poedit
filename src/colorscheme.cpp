/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 2016-2018 Vaclav Slavik
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

#include "colorscheme.h"

#include <wx/settings.h>

#ifdef __WXGTK__
    #include <pango/pango.h>
    #if PANGO_VERSION_CHECK(1,38,0)
        #define SUPPORTS_BGALPHA
    #endif
#else
    #define SUPPORTS_BGALPHA
#endif


namespace
{

#ifdef __WXOSX__

inline wxColour sRGB(int r, int g, int b, double a = 1.0)
{
    return wxColour([NSColor colorWithSRGBRed:r/255.0 green:g/255.0 blue:b/255.0 alpha:a]);
}

inline bool IsDarkAppearance(NSAppearance *appearance)
{
    if (@available(macOS 10.14, *))
    {
        NSAppearanceName appearanceName = [appearance bestMatchFromAppearancesWithNames:@[NSAppearanceNameAqua, NSAppearanceNameDarkAqua]];
        return [appearanceName isEqualToString:NSAppearanceNameDarkAqua];
    }
    else
    {
        return false;
    }
}

#else

inline wxColour sRGB(int r, int g, int b, double a = 1.0)
{
    return wxColour(r, g, b, int(a * wxALPHA_OPAQUE));
}

#endif

} // anonymous namespace

std::unique_ptr<ColorScheme::Data> ColorScheme::s_data;
bool ColorScheme::s_appModeDetermined = false;
ColorScheme::Mode ColorScheme::s_appMode = ColorScheme::Mode::Light;


wxColour ColorScheme::DoGet(Color color, Mode mode)
{
    switch (color)
    {
        // Labels:

        case Color::SecondaryLabel:
            #ifdef __WXOSX__
            return wxColour([NSColor secondaryLabelColor]);
            #elif defined(__WXGTK__)
            return wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT);
            #else
            return wxSystemSettings::GetColour(wxSYS_COLOUR_GRAYTEXT);
            #endif

        // List items:

        case Color::ItemID:
            #ifdef __WXOSX__
            return wxColour([NSColor tertiaryLabelColor]);
            #else
            return mode == Light ? "#a1a1a1" : wxSystemSettings::GetColour(wxSYS_COLOUR_LISTBOXTEXT).ChangeLightness(50);
            #endif
        case Color::ItemFuzzy:
            return mode == Dark ? sRGB(253, 178, 72) : sRGB(230, 134, 0);
        case Color::ItemError:
            return sRGB(225, 77, 49);
        case Color::ItemContextFg:
            return mode == Dark ? sRGB(180, 222, 254) : sRGB(70, 109, 137);
        case Color::ItemContextBg:
            return mode == Dark ? sRGB(67, 94, 147, 0.6) : sRGB(217, 232, 242);
        case Color::ItemContextBgHighlighted:
            #if defined(__WXMSW__)
            return sRGB(255, 255, 255, 0.50);
            #elif defined(SUPPORTS_BGALPHA)
            return sRGB(255, 255, 255, 0.35);
            #else
            return DoGet(Color::ItemContextBg, type);
            #endif

        // Tags:

        case Color::TagContextFg:
            return DoGet(Color::ItemContextFg, mode);
        case Color::TagContextBg:
            return DoGet(Color::ItemContextBg, mode);
        case Color::TagSecondaryBg:
            return mode == Dark ? sRGB(255, 255, 255, 0.5) : sRGB(0, 0, 0, 0.10);
        case Color::TagErrorLineBg:
            return sRGB(241, 134, 135);
        case Color::TagWarningLineBg:
            return mode == Dark ? sRGB(198, 171, 113) : sRGB(253, 235, 176);
        case Color::TagErrorLineFg:
            return sRGB(0, 0, 0, 0.8);
        case Color::TagSecondaryFg:
        case Color::TagWarningLineFg:
            return sRGB(0, 0, 0, 0.9);


        // Separators:

        case Color::ToolbarSeparator:
            return mode == Dark ? "#505050" : "#cdcdcd";
        case Color::SidebarSeparator:
            return mode == Dark ? *wxBLACK : "#cbcbcb";
        case Color::EditingSeparator:
            return mode == Dark ? sRGB(80, 80, 80) : sRGB(204, 204, 204);
        case Color::EditingSubtleSeparator:
            return mode == Dark ? sRGB(60, 60, 60) : sRGB(229, 229, 229);

        // Backgrounds:

        case Color::SidebarBackground:
            return mode == Dark ? sRGB(43, 44, 47) : "#edf0f4";

        case Color::EditingBackground:
            #ifdef __WXOSX__
            return wxColour([NSColor textBackgroundColor]);
            #else
            return wxSystemSettings::GetColour(wxSYS_COLOUR_LISTBOX);
            #endif

        // Fuzzy toggle:
        case Color::FuzzySwitch:
            return mode == Dark ? sRGB(253, 178, 72) : sRGB(244, 143, 0);
        case Color::FuzzySwitchInactive:
            return mode == Dark ? sRGB(163, 163, 163) : sRGB(87, 87, 87);

        // Syntax highlighting:

        case Color::SyntaxLeadingWhitespaceBg:
            return mode == Dark ? sRGB(75, 49, 111) : sRGB(234, 223, 247);
        case Color::SyntaxEscapeFg:
            return mode == Dark ? sRGB(234, 188, 244) : sRGB(162, 0, 20);
        case Color::SyntaxEscapeBg:
            return mode == Dark ? sRGB(90, 15, 167, 0.5) : sRGB(254, 234, 236);
        case Color::SyntaxMarkup:
            return mode == Dark ? sRGB(76, 156, 230) : sRGB(0, 121, 215);
        case Color::SyntaxFormat:
            return mode == Dark ? sRGB(250, 165, 251) : sRGB(178, 52, 197);

        // Attention bar:

#ifdef __WXGTK__
        // FIXME: use system colors
        case Color::AttentionWarningBackground:
            return sRGB(250, 173, 61);
        case Color::AttentionQuestionBackground:
            return sRGB(138, 173, 212);
        case Color::AttentionErrorBackground:
            return sRGB(237, 54, 54);
#else
        case Color::AttentionWarningBackground:
            return mode == Dark ? sRGB(254, 224, 132) : sRGB(254, 228, 149);
        case Color::AttentionQuestionBackground:
            return sRGB(199, 244, 156);
        case Color::AttentionErrorBackground:
            return sRGB(241, 103, 104);
#endif

        // Buttons:

        case Color::TranslucentButton:
            return sRGB(255, 255, 255, 0.5);

        case Color::Max:
            return wxColour();
    }

    return wxColour(); // Visual C++ being silly
}


ColorScheme::Mode ColorScheme::GetAppMode()
{
    if (!s_appModeDetermined)
    {
#ifdef __WXOSX__
        if (@available(macOS 10.14, *))
            s_appMode = IsDarkAppearance(NSApp.effectiveAppearance) ? Dark : Light;
        else
            s_appMode = Light;
#else
        auto colBg = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW);
        if (colBg.Red() < 0x60 && colBg.Green() < 0x60 && colBg.Blue() < 0x60)
            s_appMode = Dark;
        else
            s_appMode = Light;
#endif
        s_appModeDetermined = true;
    }

    return s_appMode;
}


ColorScheme::Mode ColorScheme::GetWindowMode(wxWindow *win)
{
#ifdef __WXOSX__
    NSView *view = win->GetHandle();
    return IsDarkAppearance(view.effectiveAppearance) ? Dark : Light;
#else
    auto colBg = win->GetDefaultAttributes().colBg;

    // Use dark scheme for very dark backgrounds:
    if (colBg.Red() < 0x60 && colBg.Green() < 0x60 && colBg.Blue() < 0x60)
        return Dark;
    else
        return Light;
#endif
}


wxColour ColorScheme::GetBlendedOn(Color color, wxWindow *win, Color bgColor)
{
    auto bg = (bgColor != Color::Max) ? Get(bgColor, win) : win->GetBackgroundColour();
    auto fg = Get(color, win);
#ifndef __WXOSX__
    if (fg.Alpha() != wxALPHA_OPAQUE)
    {
        double alpha = fg.Alpha() / 255.0;
        return wxColour(wxColour::AlphaBlend(fg.Red(), bg.Red(), alpha),
                        wxColour::AlphaBlend(fg.Green(), bg.Green(), alpha),
                        wxColour::AlphaBlend(fg.Blue(), bg.Blue(), alpha));
    }
#endif
    return fg;
}

void ColorScheme::CleanUp()
{
    s_data.reset();
}
