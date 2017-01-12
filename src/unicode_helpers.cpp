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

#include "unicode_helpers.h"

#include "str_helpers.h"

#include <unicode/ubidi.h>

namespace bidi
{

TextDirection get_base_direction(const wxString& text)
{
    if (text.empty())
        return TextDirection::LTR;

    auto s = str::to_icu_raw(text);
    switch (ubidi_getBaseDirection(s.data(), (int)s.length()))
    {
        case UBIDI_RTL:
            return TextDirection::RTL;
        case UBIDI_LTR:
        case UBIDI_NEUTRAL:
            return TextDirection::LTR;
        case UBIDI_MIXED:
            assert(false); // impossible value here
            return TextDirection::LTR;
    }
    return TextDirection::LTR; // VC++ is stupid
}


wxString strip_pointless_control_chars(const wxString& text, TextDirection dir)
{
    if (text.empty())
        return text;

    const wchar_t first = *text.begin();
    const wchar_t last = *text.rbegin();

    // POP DIRECTIONAL FORMATTING at the end is pointless (can happen on macOS
    // when editing RTL text under LTR locale:
    if (last == PDF)
    {
        return strip_pointless_control_chars(text.substr(0, text.size() - 1), dir);
    }

    if (dir == TextDirection::LTR)
    {
        switch (first)
        {
            case LRE:
            case LRO:
            case LRI:
            case LRM:
                return text.substr(1);
            default:
                return text;
        }
    }
    else if (dir == TextDirection::RTL)
    {
        switch (first)
        {
            case RLE:
            case RLO:
            case RLI:
            case RLM:
                return text.substr(1);
            default:
                return text;
        }
    }

    return text;
}


wxString strip_control_chars(const wxString& text)
{
    if (text.empty())
        return text;

    const wchar_t first = *text.begin();
    switch (first)
    {
        case LRE:
        case LRO:
        case LRI:
        case LRM:
        case RLE:
        case RLO:
        case RLI:
        case RLM:
            return text.substr(1);
        default:
            break;
    }
    return text;
}


wxString mark_direction(const wxString& text, TextDirection dir)
{
    wchar_t mark = (dir == TextDirection::LTR) ? LRE : RLE;
    auto out = mark + text;
#ifdef BIDI_NEEDS_DIRECTION_ON_EACH_LINE
    wchar_t replacement[3] = {0};
    replacement[0] = L'\n';
    replacement[1] = mark;
    out.Replace("\n", replacement);
#endif
    return out;
}

} // namespace bidi
