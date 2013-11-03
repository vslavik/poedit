/*
 *  This file is part of Poedit (http://www.poedit.net)
 *
 *  Copyright (C) 2013 Vaclav Slavik
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

#ifndef Poedit_icuhelpers_h
#define Poedit_icuhelpers_h

#include <unicode/unistr.h>

#include <wx/string.h>


/**
    Create read-only icu::UnicodeString from wxString efficiently.
    
    Notice that the resulting string is only valid for the input wxString's
    lifetime duration, unless you make a copy.
 */
inline icu::UnicodeString ToIcuStr(const wxString& str)
{
#if wxUSE_UNICODE_UTF8
    return icu::UnicodeString::fromUTF8((const char*)str.utf8_str());
#elif SIZEOF_WCHAR_T == 4
    return icu::UnicodeString::fromUTF32((const UChar32*)str.wx_str(), (int32_t)str.length());
#elif SIZEOF_WCHAR_T == 2
    // read-only aliasing ctor, doesn't copy data
    return icu::UnicodeString(true, str.wx_str(), str.length());
#else
    #error "WTF?!"
#endif
}

/**
Create read-only icu::UnicodeString from std::wstring efficiently.

Notice that the resulting string is only valid for the input std::wstring's
lifetime duration, unless you make a copy.
*/
inline icu::UnicodeString ToIcuStr(const std::wstring& str)
{
#if SIZEOF_WCHAR_T == 4
    return icu::UnicodeString::fromUTF32((const UChar32*) str.c_str(), (int32_t) str.length());
#elif SIZEOF_WCHAR_T == 2
    // read-only aliasing ctor, doesn't copy data
    return icu::UnicodeString(true, str.c_str(), str.length());
#else
    #error "WTF?!"
#endif
}

/// Create wxString from icu::UnicodeString, making a copy.
inline wxString FromIcuStr(const icu::UnicodeString& str)
{
#if wxUSE_UNICODE_WCHAR && SIZEOF_WCHAR_T == 2
    return wxString(str.getBuffer(), str.length());
#else
    return wxString((const char*)str.getBuffer(), wxMBConvUTF16(), str.length() * 2);
#endif
}


#endif // Poedit_icuhelpers_h
