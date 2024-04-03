/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 2016-2024 Vaclav Slavik
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

#include "str_helpers.h"

#include <wx/string.h>

#include <unicode/ucol.h>
#include <unicode/ubrk.h>


#ifdef __WXMSW__
    #define BIDI_NEEDS_DIRECTION_ON_EACH_LINE
    #define BIDI_PLATFORM_DOESNT_DETECT_DIRECTION
#endif


namespace unicode
{

/// Collator class - language-aware sorting
class Collator
{
public:
    enum mode
    {
        case_sensitive,
        case_insensitive
    };

    typedef UCollationResult result_type;

    /// Ctor for sorting using rules for given language.
    /// If the language is not provide, uses current UI language.
    Collator(const Language& language, mode m = case_sensitive);
    Collator(mode m = case_sensitive) : Collator(Language(), m) {}
    ~Collator();

    Collator(const Collator&) = delete;
    Collator& operator=(const Collator&) = delete;

    result_type compare(const wxString& a, const wxString& b) const
    {
#if wxUSE_UNICODE_UTF8
        UErrorCode err = U_ZERO_ERROR;
        return ucol_strcollUTF8(m_coll, a.wx_str(), -1, b.wx_str(), -1, &err);
#else
        return ucol_strcoll(m_coll, str::to_icu(a), -1, str::to_icu(b), -1);
#endif
    };

    result_type compare(const std::string& a, const std::string& b) const
    {
        UErrorCode err = U_ZERO_ERROR;
        return ucol_strcollUTF8(m_coll, a.c_str(), (int32_t)a.length(), b.c_str(), (int32_t)b.length(), &err);
    }

    result_type compare(const std::wstring& a, const std::wstring& b) const
    {
        return ucol_strcoll(m_coll, str::to_icu(a), -1, str::to_icu(b), -1);
    }

    template<typename T>
    bool operator()(const T& a, const T& b) const
    {
        return compare(a, b) == UCOL_LESS;
    }

private:
    UCollator *m_coll = nullptr;
};


/// Low-level thin wrapper around UBreakIterator
class BreakIterator
{
public:
    BreakIterator(UBreakIteratorType type, const Language& lang);
    ~BreakIterator()
    {
        if (m_bi)
            ubrk_close(m_bi);
    }

    /**
     Set the text to process.

     The lifetime of the text buffer must be longer than the lifetime of BreakIterator!
     */
    void set_text(const UChar *text)
    {
        UErrorCode err = U_ZERO_ERROR;
        ubrk_setText(m_bi, text, -1, &err);
    }

    /// Sets iterator to the beginning
    int32_t begin() { return ubrk_first(m_bi); }
    constexpr int32_t end() const { return UBRK_DONE; }

    /// Advances the iterator, returns character index of the next text boundary, or UBRK_DONE if all text boundaries have been returned.
    int32_t next() { return ubrk_next(m_bi); }
    int32_t previous() { return ubrk_previous(m_bi); }

    /// Return current rule status
    int32_t rule() const { return ubrk_getRuleStatus(m_bi); }

private:
    UBreakIterator *m_bi = nullptr;
};


/// Helper for writing data from ICU C API to string types, optimized to avoid copying in case of UTF-16 wchar_t
template<typename T>
struct UCharWriteBuffer
{
    UCharWriteBuffer(int32_t length) : m_data(str::UCharBuffer::owned(length)) {}

    UChar *data() { return m_data.data(); }
    int32_t capacity() { return m_data.capacity(); }

    auto output() { return str::to<T>(m_data); }

private:
    str::UCharBuffer m_data;
};

namespace detail
{

template<typename T>
struct UCharWriteBufferIntoString
{
    UCharWriteBufferIntoString(int32_t length) : m_data(length, L'\0') {}

    UChar *data() { return reinterpret_cast<UChar*>(m_data.data()); }
    int32_t capacity() { return (int32_t)m_data.length() + 1; }

    T output() { return std::move(m_data); }

private:
    T m_data;
};

} // namespace detail

template<>
struct UCharWriteBuffer<std::u16string> : public detail::UCharWriteBufferIntoString<std::u16string>
{
    using detail::UCharWriteBufferIntoString<std::u16string>::UCharWriteBufferIntoString;
};

#if SIZEOF_WCHAR_T == 2
template<>
struct UCharWriteBuffer<std::wstring> : public detail::UCharWriteBufferIntoString<std::wstring>
{
    using detail::UCharWriteBufferIntoString<std::wstring>::UCharWriteBufferIntoString;
};
#endif


/// Like fold_case(), but limited to buffers
inline bool fold_case(const UChar *input, UChar *output, int32_t capacity)
{
    UErrorCode err = U_ZERO_ERROR;
    u_strFoldCase(output, capacity, input, -1, U_FOLD_CASE_DEFAULT, &err);
    return U_SUCCESS(err);
}

/// Folds case Unicode-correctly, converting to a different output type
template<typename TOut, typename TIn>
inline auto fold_case_to_type(const TIn& str)
{
    UErrorCode err = U_ZERO_ERROR;
    auto input = str::to_icu(str);
    int32_t length = u_strFoldCase(nullptr, 0, input, -1, U_FOLD_CASE_DEFAULT, &err);

    UCharWriteBuffer<TOut> folded(length);
    fold_case(input, folded.data(), folded.capacity());
    return folded.output();
};

/// Folds case Unicode-correctly
template<typename T>
inline auto fold_case(const T& str)
{
    return fold_case_to_type<T, T>(str);
}

/// Upper-casing Unicode-correctly
template<typename T>
inline auto to_upper(const T& str)
{
    UErrorCode err = U_ZERO_ERROR;
    auto input = str::to_icu(str);
    int32_t length = u_strToUpper(nullptr, 0, input, -1, nullptr, &err);

    err = U_ZERO_ERROR;
    UCharWriteBuffer<T> transformed(length);
    u_strToUpper(transformed.data(), transformed.capacity(), input, -1, nullptr, &err);
    return transformed.output();
};

} // namespace unicode


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


/// Is the character a directional control character?
inline bool is_direction_mark(wchar_t c)
{
    return (c >= LRE && c <= RLO) ||
           (c >= LRI && c <= PDI) ||
           (c >= LRM && c <= RLM) ||
           (c == ALM);
}

/**
    Determine base direction of the text provided according to the
    Unicode Bidirectional Algorithm.
 */
TextDirection get_base_direction(const wxString& text);

/**
    Removes leading directional control characters that don't make sense for
    text in given language. For example, prefixing RTL text with RLE is redundant;
    prefixing with LRE is not.

    This function exists primarily to solve issues with text controls when
    editing text in language different from the UI's language.
 */
wxString strip_pointless_control_chars(const wxString& text, TextDirection dir);

/**
    Remove leading directional control characters.

    For use if the text has known direction or can't have control characters. 
 */
wxString strip_control_chars(const wxString& text);

/**
    Prepend directional mark to text, for display purposes on platforms that
    don't detect text's directionality on their own or when showing text in
    different directionality.
 */
wxString mark_direction(const wxString& text, TextDirection dir);

inline wxString mark_direction(const wxString& text, const Language& lang)
    { return mark_direction(text, lang.Direction()); }

inline wxString mark_direction(const wxString& text)
    { return mark_direction(text, get_base_direction(text)); }

#ifdef BIDI_PLATFORM_DOESNT_DETECT_DIRECTION
inline wxString platform_mark_direction(const wxString& text)
    { return mark_direction(text); }
#else
inline wxString platform_mark_direction(const wxString& text)
    { return text; }
#endif

} // namespace bidi

#endif // Poedit_unicode_helpers_h
