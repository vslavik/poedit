/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 2025 Vaclav Slavik
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

#ifndef Poedit_layout_helpers_h
#define Poedit_layout_helpers_h

inline int AboveChoicePadding()
{
#ifdef __WXOSX__
    if (__builtin_available(macOS 26.0, *))
        return 0;
    else
        return 2;
#else
    return 0;
#endif
}

inline int UnderCheckboxIndent()
{
#if defined(__WXOSX__)
    if (__builtin_available(macOS 26.0, *))
        return 22;
    else
        return 20;
#elif defined(__WXMSW__)
    return PX(17);
#elif defined(__WXGTK__)
    return PX(25);
#else
    #error "Implement UnderCheckboxIndent() for your platform"
#endif
}


#endif // Poedit_layout_helpers_h
