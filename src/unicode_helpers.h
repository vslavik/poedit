/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 2016 Vaclav Slavik
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

#ifndef Poedit_unicode_helpers_h
#define Poedit_unicode_helpers_h

#include "language.h"

#include <wx/string.h>

namespace bidi
{

/// Bidi control characters
static const wchar_t LRE = L'\u202a';  // LEFT-TO-RIGHT EMBEDDING
static const wchar_t RLE = L'\u202b';  // RIGHT-TO-LEFT EMBEDDING
static const wchar_t PDF = L'\u202c';  // POP DIRECTIONAL FORMATTING
static const wchar_t LRO = L'\u202d';  // LEFT-TO-RIGHT OVERRIDE
static const wchar_t RLO = L'\u202e';  // RIGHT-TO-LEFT OVERRIDE
static const wchar_t LRI = L'\u2066';  // LEFT‑TO‑RIGHT ISOLATE
static const wchar_t RLI = L'\u2067';  // RIGHT‑TO‑LEFT ISOLATE
static const wchar_t FSI = L'\u2068';  // FIRST STRONG ISOLATE
static const wchar_t PDI = L'\u2069';  // POP DIRECTIONAL ISOLATE
static const wchar_t LRM = L'\u200e';  // LEFT-TO-RIGHT MARK
static const wchar_t RLM = L'\u200f';  // RIGHT-TO-LEFT MARK
static const wchar_t ALM = L'\u061c';  // ARABIC LETTER MARK


/**
    Removes leading directional control characters that don't make sense for
    text in given language. For example, prefixing RTL text with RLE is redundant;
    prefixing with LRE is not.

    This function exists primarily to solve issues with text controls when
    editing text in language different from the UI's language.
 */
wxString strip_pointless_control_chars(const wxString& text, TextDirection dir);

} // namespace bidi

#endif // Poedit_unicode_helpers_h
