/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 2013-2017 Vaclav Slavik
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

#ifndef Poedit_str_helpers_h
#define Poedit_str_helpers_h

#include <string>

#include <boost/locale/encoding_utf.hpp>

#ifdef __OBJC__
#include <Foundation/NSString.h>
#endif

#include <wx/string.h>
#include <wx/buffer.h>

/**
    Defines conversions between various string types.
    
    Supported string classes are std::wstring, std::string (UTF-8 encoded),
    wxString and icu::UnicodeString.
    
    Usage:
        - to_wx(...)
        - to_icu(...)
        - to_wstring(...)
        - to_utf8(...)
        - to_NSString()
 */
namespace str
{

inline std::string to_utf8(const std::wstring& str)
{
    return boost::locale::conv::utf_to_utf<char>(str);
}

inline std::string to_utf8(const wchar_t *str)
{
    return boost::locale::conv::utf_to_utf<char>(str);
}

inline std::wstring to_wstring(const std::string& utf8str)
{
    return boost::locale::conv::utf_to_utf<wchar_t>(utf8str);
}

inline std::string to_utf8(const wxString& str)
{
    return std::string(str.utf8_str());
}

inline std::wstring to_wstring(const wxString& str)
{
    return str.ToStdWstring();
}

#if defined(__cplusplus) && defined(__OBJC__)

inline NSString *to_NS(const wxString& str)
{
    return [NSString stringWithUTF8String:str.utf8_str()];
}

inline wxString to_wx(NSString *str)
{
    return wxString::FromUTF8Unchecked([str UTF8String]);
}

inline NSString *to_NS(const std::string& utf8str)
{
    return [NSString stringWithUTF8String:utf8str.c_str()];
}

inline NSString *to_NS(const char *utf8str)
{
    return [NSString stringWithUTF8String:utf8str];
}

inline std::string to_utf8(NSString *str)
{
    return std::string([str UTF8String]);
}

inline NSString *to_NS(const std::wstring& str)
{
    return to_NS(to_utf8(str));
}

inline std::wstring to_wstring(NSString *str)
{
    return boost::locale::conv::utf_to_utf<wchar_t>([str UTF8String]);
}

#endif // Objective-C++


// ICU conversions; only include them if ICU is included
#ifdef UNISTR_H

/**
    Create read-only icu::UnicodeString from wxString efficiently.
    
    Notice that the resulting string is only valid for the input wxString's
    lifetime duration, unless you make a copy.
 */
inline icu::UnicodeString to_icu(const wxString& str)
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
inline icu::UnicodeString to_icu(const std::wstring& str)
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
inline wxString to_wx(const icu::UnicodeString& str)
{
#if wxUSE_UNICODE_WCHAR && SIZEOF_WCHAR_T == 2
    return wxString(str.getBuffer(), str.length());
#else
    return wxString((const char*)str.getBuffer(), wxMBConvUTF16(), str.length() * 2);
#endif
}

/// Create std::wstring from icu::UnicodeString, making a copy.
inline std::wstring to_wstring(const icu::UnicodeString& str)
{
    return to_wx(str).ToStdWstring();
}

#endif // UNISTR_H

// Low-level ICU conversions:
#ifdef U_SIZEOF_UCHAR

/**
    Create buffer with raw UChar* string.

    Notice that the resulting string is only valid for the input's lifetime.
 */
inline wxScopedCharTypeBuffer<UChar> to_icu_raw(const wxString& str)
{
    static_assert(U_SIZEOF_UCHAR == 2, "unexpected UChar size");
#if SIZEOF_WCHAR_T == 2
    // read-only aliasing ctor, doesn't copy data
    return wxScopedCharTypeBuffer<UChar>::CreateNonOwned((const UChar*)str.wx_str(), str.length());
#else
    auto buf = wxMBConvUTF16().cWC2MB(str.wc_str());
    auto len = buf.length();
    return wxCharTypeBuffer<UChar>::CreateOwned((UChar*)buf.release(), len);
#endif
}

#endif // U_SIZEOF_UCHAR


} // namespace str

#endif // Poedit_str_helpers_h
