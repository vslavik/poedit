/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 2016-2017 Vaclav Slavik
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
    SecondaryLabel,

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
    EditingSeparator,
    EditingSubtleSeparator,

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
    /// Type of the scheme to use (not used a lot for now)
    enum Type
    {
        Light,
        Dark
    };

    static const wxColour& Get(Color color, Type type = Light)
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
        return Get(color, win->GetDefaultAttributes());
    }

    static const wxColour& Get(Color color, const wxVisualAttributes& win)
    {
        return Get(color, GetSchemeTypeFromWindow(win));
    }

    static wxColour GetBlendedOn(Color color, wxWindow *win);

    static void CleanUp();

private:
    struct Data
    {
        wxColour colors[static_cast<size_t>(Color::Max)][2];
    };

    static wxColour DoGet(Color color, Type type);
    static Type GetSchemeTypeFromWindow(const wxVisualAttributes& win);

    static std::unique_ptr<Data> s_data;
};

#endif // Poedit_colorscheme_h
