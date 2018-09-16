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
            return mode == Light ? "#a1a1a1" : wxNullColour;
        case Color::ItemFuzzy:
            return mode == Light ? sRGB(218, 123, 0) : "#a9861b";
        case Color::ItemError:
            return DoGet(Color::TagErrorLineFg, mode);
        case Color::ItemContextFg:
            return sRGB(70, 109, 137);
        case Color::ItemContextBg:
            return sRGB(217, 232, 242);
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
        case Color::TagSecondaryFg:
            return sRGB(87, 87, 87);
        case Color::TagSecondaryBg:
            return sRGB(236, 236, 236);
        case Color::TagErrorLineFg:
            return sRGB(162, 0, 20);
        case Color::TagErrorLineBg:
            return sRGB(255, 227, 230, 0.75);
        case Color::TagWarningLineFg:
            return sRGB(101, 91, 1);
        case Color::TagWarningLineBg:
            return sRGB(255, 249, 192, 0.75);


        // Separators:

        case Color::ToolbarSeparator:
            return "#cdcdcd";
        case Color::SidebarSeparator:
            return "#cbcbcb";
        case Color::EditingSeparator:
            return sRGB(204, 204, 204);
        case Color::EditingSubtleSeparator:
            return sRGB(229, 229, 229);

        // Backgrounds:

        case Color::SidebarBackground:
            if (GetAppMode() == Dark)
                return "#120f0b";
            else
                return "#edf0f4";
        case Color::EditingBackground:
            #ifdef __WXOSX__
            return wxColour([NSColor textBackgroundColor]);
            #else
            return wxSystemSettings::GetColour(wxSYS_COLOUR_LISTBOX);
            #endif

        // Fuzzy toggle:
        case Color::FuzzySwitch:
            return DoGet(Color::ItemFuzzy, mode);
        case Color::FuzzySwitchInactive:
            return sRGB(87, 87, 87);

        // Syntax highlighting:

        case Color::SyntaxLeadingWhitespaceBg:
            return sRGB(255, 233, 204);
        case Color::SyntaxEscapeFg:
            return sRGB(162, 0, 20);
        case Color::SyntaxEscapeBg:
            return sRGB(254, 234, 236);
        case Color::SyntaxMarkup:
            return sRGB(0, 121, 215);
        case Color::SyntaxFormat:
            return sRGB(178, 52, 197);

        // Attention bar:

#ifdef __WXGTK__
        case Color::AttentionWarningBackground:
            return sRGB(250, 173, 61);
        case Color::AttentionQuestionBackground:
            return sRGB(138, 173, 212);
        case Color::AttentionErrorBackground:
            return sRGB(237, 54, 54);
#else
        case Color::AttentionWarningBackground:
            return sRGB(255, 222, 91);
        case Color::AttentionQuestionBackground:
            return sRGB(150, 233, 109);
        case Color::AttentionErrorBackground:
            return sRGB(255, 125, 125);
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


wxColour ColorScheme::GetBlendedOn(Color color, wxWindow *win)
{
    auto bg = win->GetBackgroundColour();
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
