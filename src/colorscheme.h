/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 2016-2020 Vaclav Slavik
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

#ifndef Poedit_colorscheme_h
#define Poedit_colorscheme_h

#include <wx/colour.h>
#include <wx/window.h>

#include <memory>


/// Symbolic color names
enum class Color : size_t
{
    Label,
    SecondaryLabel,

    ErrorText,

    ItemID,
    ItemFuzzy,
    ItemError,
    ItemContextFg,
    ItemContextBg,
    ItemContextBgHighlighted,

    TagContextFg,
    TagContextBg,
    TagSecondaryFg,
    TagSecondaryBg,
    TagErrorLineFg,
    TagErrorLineBg,
    TagWarningLineFg,
    TagWarningLineBg,

    ToolbarSeparator,
    SidebarSeparator,
    SidebarBlockSeparator,
    EditingSeparator,
    EditingThickSeparator,

    SidebarBackground,
    EditingBackground,

    FuzzySwitch,
    FuzzySwitchInactive,

    SyntaxLeadingWhitespaceBg,
    SyntaxEscapeFg,
    SyntaxEscapeBg,
    SyntaxMarkup,
    SyntaxFormat,

    AttentionWarningBackground,
    AttentionQuestionBackground,
    AttentionErrorBackground,

    TranslucentButton,

    Max
};


/**
    Defines colors for various non-standard UI elements in one place.
    
    Includes platform-specific customizations as appropriate.
 */
class ColorScheme
{
public:
    /// Mode of the scheme to use (not used a lot for now)
    enum Mode
    {
        Light,
        Dark
    };

    static const wxColour& Get(Color color, Mode type = GetAppMode())
    {
        if (!s_data)
            s_data = std::make_unique<Data>();
        auto& c = s_data->colors[static_cast<size_t>(color)][type];
        if (c.IsOk())
            return c;
        else
            return c = DoGet(color, type);
    }

    static const wxColour& Get(Color color, wxWindow *win)
    {
        return Get(color, GetWindowMode(win));
    }

    static wxColour GetBlendedOn(Color color, wxWindow *win, Color bgColor = Color::Max);

    /**
        Setup window for updating when color scheme changes.

        Will execute @a update closure initially and then every time system
        color scheme changes. This allows putting all color-configuration code
        into the closure without duplication.

        Should be called during window creation from ctor or Create().
     */
    template<typename Func>
    static void SetupWindowColors(wxWindow *win, Func&& setup)
    {
        // Initial setup of the window:
        setup();

        // React to system color scheme changes:
        win->Bind(wxEVT_SYS_COLOUR_CHANGED, [=](wxSysColourChangedEvent& e)
        {
            e.Skip();

            // Do everything as deferred to the next event loop cycle so that
            // macOS has a chance to update its information about current
            // appearance that we rely on:
            win->CallAfter([=]
            {
                // Invalidate global cached data. This is efficient if called
                // repeatedly, so we can do it for every affected window without
                // worrying about performance impact:
                InvalidateCachesIfNeeded();

                // Update and redraw the window:
                setup();
                win->Refresh();
            });
        });
    }

    /// Simpler version of SetupWindowColors() for when redrawing is enough
    /// (e.g. native control or all colors fetched in OnPaint)
    static void RefreshOnChange(wxWindow *win) { SetupWindowColors(win, []{}); }

    /// Returns app-wide mode (dark, light)
    static Mode GetAppMode();
    static Mode GetWindowMode(wxWindow *win);

    static void CleanUp();

private:
    struct Data
    {
        wxColour colors[static_cast<size_t>(Color::Max)][2];
    };

    static wxColour DoGet(Color color, Mode type);
    static void InvalidateCachesIfNeeded();

    static std::unique_ptr<Data> s_data;
    static bool s_appModeDetermined;
    static Mode s_appMode;
};

#endif // Poedit_colorscheme_h
