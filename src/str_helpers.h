/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 2013-2025 Vaclav Slavik
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
#include <utility>

#include <boost/locale/encoding_utf.hpp>

#ifdef __OBJC__
    #include <Foundation/NSString.h>
#endif

#ifdef __cplusplus
    #include <wx/string.h>
    #include <wx/buffer.h>

    #include <unicode/utypes.h>
    #include <unicode/ustring.h>
#endif // __cplusplus


/**
    Defines conversions between various string types.
    
    Supported string classes are std::wstring, std::string (UTF-8 encoded),
    wxString and ICU UChar* strings.
    
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

inline std::string to_utf8(const unsigned char *str)
{
    return std::string(reinterpret_cast<const char*>(str));
}

inline std::wstring to_wstring(const std::string& utf8str)
{
    return boost::locale::conv::utf_to_utf<wchar_t>(utf8str);
}

inline std::wstring to_wstring(const char *utf8str)
{
    return boost::locale::conv::utf_to_utf<wchar_t>(utf8str);
}

inline std::wstring to_wstring(const unsigned char *utf8str)
{
    return boost::locale::conv::utf_to_utf<wchar_t>(utf8str);
}

inline std::string to_utf8(const wxString& str)
{
    return str.utf8_string();
}

#if wxUSE_STD_STRING && wxUSE_UNICODE_WCHAR && wxUSE_STL_BASED_WXSTRING
typedef const std::wstring& wstring_conv_t;
#else
typedef std::wstring wstring_conv_t;
#endif
inline wstring_conv_t to_wstring(const wxString& str)
{
    return str.ToStdWstring();
}

inline wxString to_wx(const char *utf8)
{
    return wxString::FromUTF8(utf8);
}

inline wxString to_wx(const unsigned char *utf8)
{
    return wxString::FromUTF8((const char*)utf8);
}

inline wxString to_wx(const std::string& utf8)
{
    return wxString::FromUTF8(utf8.c_str());
}

inline wxString to_wx(const std::wstring& str)
{
    return wxString(str);
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

inline NSString *to_NS(const unsigned char *utf8str)
{
    return [NSString stringWithUTF8String:(const char*)utf8str];
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


// ICU conversions:


/// Buffer holding, possibly non-owned, UChar* NULL-terminated string
class UCharBuffer
{
public:
    UCharBuffer(UCharBuffer&& other) noexcept
        : m_owned(std::exchange(other.m_owned, false)),
          m_data(std::exchange(other.m_data, nullptr)),
          m_capacity(std::exchange(other.m_capacity, 0))
    {}

    UCharBuffer(const UCharBuffer&) = delete;
    UCharBuffer& operator=(const UCharBuffer&) = delete;

    static UCharBuffer owned(int32_t length) { return UCharBuffer(true, new UChar[length + 1], length + 1); }
    static UCharBuffer non_owned(const UChar *data) { return UCharBuffer(false, data, -1); }
    static UCharBuffer null() { static UChar empty[1] = {0}; return UCharBuffer(false, empty, 0); }

    ~UCharBuffer()
    {
        if (m_owned)
            delete[] m_data;
    }

    operator const UChar*() const { return m_data; }
    UChar* data() { return const_cast<UChar*>(m_data); }

    /// Available buffer size, only for owned versions, returns 0 for read-only non-owned
    int32_t capacity() { return m_capacity; }

    /// Ensure the buffer has a deep copy of the data, if it's not already owned
    UCharBuffer& ensure_owned()
    {
        if (m_owned)
			return *this;
        if (m_capacity == -1)
            m_capacity = u_strlen(m_data) + 1;
        auto copy = new UChar[m_capacity];
        memcpy(copy, m_data, m_capacity * sizeof(UChar));
        m_data = copy;
        m_owned = true;
		return *this;
	}

private:
    UCharBuffer(bool owned, const UChar *data, int32_t capacity) : m_owned(owned), m_data(data), m_capacity(capacity) {}

    bool m_owned;
    const UChar *m_data;
    int32_t m_capacity;
};


// Simple check for empty buffer / C string:
template<typename T>
inline bool empty(const T *str)
{
    return !str || *str == 0;
}


inline UCharBuffer to_icu(const char *str)
{
    int32_t destLen = 0;
    UErrorCode err = U_ZERO_ERROR;
    u_strFromUTF8Lenient(nullptr, 0, &destLen, str, -1, &err);
    if (!destLen)
        return UCharBuffer::null();
    auto buf = UCharBuffer::owned(destLen);
    err = U_ZERO_ERROR;
    u_strFromUTF8Lenient(buf.data(), buf.capacity(), nullptr, str, -1, &err);
    if (U_FAILURE(err))
        return UCharBuffer::null();
    return buf;
}

inline UCharBuffer to_icu(const wchar_t *str)
{
    static_assert(SIZEOF_WCHAR_T == 2 || SIZEOF_WCHAR_T == 4, "unexpected wchar_t size");
    static_assert(U_SIZEOF_UCHAR == 2, "unexpected UChar size");

#if SIZEOF_WCHAR_T == 2
    // read-only aliasing ctor, doesn't copy data
    return UCharBuffer::non_owned(reinterpret_cast<const UChar*>(str));
#else
    int32_t destLen = 0;
    UErrorCode err = U_ZERO_ERROR;
    u_strFromUTF32(nullptr, 0, &destLen, reinterpret_cast<const UChar32*>(str), -1, &err);
    if (!destLen)
        return UCharBuffer::null();
    auto buf = UCharBuffer::owned(destLen);
    err = U_ZERO_ERROR;
    u_strFromUTF32(buf.data(), buf.capacity(), nullptr, reinterpret_cast<const UChar32*>(str), -1, &err);
    if (U_FAILURE(err))
        return UCharBuffer::null();
    return buf;
#endif
}

/**
    Create buffer with raw UChar* string.

    Notice that the resulting string is only valid for the input's lifetime.
 */
inline UCharBuffer to_icu(const wxString& str)
{
    return to_icu(str.wx_str());
}

inline UCharBuffer to_icu(const wxString&& str)
{
    return std::move(to_icu(str.wx_str()).ensure_owned());
}

inline UCharBuffer to_icu(const std::wstring& str)
{
    return to_icu(str.c_str());
}

inline UCharBuffer to_icu(const std::wstring&& str)
{
    return std::move(to_icu(str.c_str()).ensure_owned());
}

inline UCharBuffer to_icu(const std::string& str)
{
    return to_icu(str.c_str());
}

inline const UChar* to_icu(const UChar *str)
{
    return str;
}

inline UCharBuffer to_icu(const UCharBuffer& str) = delete;


#if SIZEOF_WCHAR_T == 2

inline wxString to_wx(const UChar *str)
{
    static_assert(sizeof(wchar_t) == sizeof(UChar));
    return wxString(reinterpret_cast<const wchar_t*>(str));
}

inline wxString to_wx(const UChar *str, size_t count)
{
    static_assert(sizeof(wchar_t) == sizeof(UChar));
    return wxString(reinterpret_cast<const wchar_t*>(str), count);
}

inline std::wstring to_wstring(const UChar *str)
{
    static_assert(sizeof(wchar_t) == sizeof(UChar));
    return std::wstring(reinterpret_cast<const wchar_t*>(str));
}

#else // SIZEOF_WCHAR_T == 4

inline wxString to_wx(const UChar *str)
{
    return wxString(reinterpret_cast<const char*>(str), wxMBConvUTF16(), u_strlen(str) * 2);
}

inline wxString to_wx(const UChar *str, size_t count)
{
    return wxString(reinterpret_cast<const char*>(str), wxMBConvUTF16(), count * 2);
}

inline std::wstring to_wstring(const UChar *str)
{
    static_assert(sizeof(wchar_t) == 4);

    int32_t destLen = 0;
    UErrorCode err = U_ZERO_ERROR;
    u_strToUTF32(nullptr, 0, &destLen, str, -1, &err);
    if (!destLen)
        return std::wstring();
    std::wstring out(destLen, '\0');
    err = U_ZERO_ERROR;
    u_strToUTF32(reinterpret_cast<UChar32*>(out.data()), (int32_t)out.length() + 1, nullptr, str, -1, &err);
    if (U_FAILURE(err))
        return std::wstring();
    return out;
}

#endif // SIZEOF_WHCAR_T

inline std::string to_utf8(const UChar *str)
{
    int32_t destLen = 0;
    UErrorCode err = U_ZERO_ERROR;
    u_strToUTF8(nullptr, 0, &destLen, str, -1, &err);
    if (!destLen)
        return std::string();
    std::string out(destLen, '\0');
    err = U_ZERO_ERROR;
    u_strToUTF8(out.data(), (int32_t)out.length() + 1, nullptr, str, -1, &err);
    if (U_FAILURE(err))
        return std::string();
    return out;
}


// Template-friendly API:

namespace detail
{

template<typename TOut>
struct converter
{
};

template<>
struct converter<wxString>
{
    template<typename TIn>
    static auto convert(const TIn& s) { return str::to_wx(s); }
};

template<>
struct converter<std::wstring>
{
    template<typename TIn>
    static auto convert(const TIn& s) { return str::to_wstring(s); }
};

template<>
struct converter<std::string>
{
    template<typename TIn>
    static auto convert(const TIn& s) { return str::to_utf8(s); }
};

template<>
struct converter<UChar*>
{
    template<typename TIn>
    static auto convert(const TIn& s) { return str::to_icu(s); }
    static auto convert(str::UCharBuffer&& s) { return std::move(s); }
};

} // namespace detail

template<typename TOut, typename TIn>
inline auto to(const TIn& s)
{
    return detail::converter<TOut>::convert(s);
}

template<typename TOut, typename TIn>
inline auto to(TIn&& s)
{
    return detail::converter<TOut>::convert(std::move(s));
}

} // namespace str

#endif // Poedit_str_helpers_h
