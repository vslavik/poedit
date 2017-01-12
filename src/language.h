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

#ifndef Poedit_language_h
#define Poedit_language_h

#include <wx/string.h>
#include <string>
#include <vector>
#include <unicode/locid.h>

/// Language's text writing direction
enum class TextDirection
{
    LTR,
    RTL
};


/// Representation of translation's language.
class Language
{
public:
    Language() : m_direction(TextDirection::LTR) {}

    bool IsValid() const { return !m_code.empty(); }
    const std::string& Code() const { return m_code; }
    std::wstring WCode() const { return std::wstring(m_code.begin(), m_code.end()); }

    /// Returns language part (cs)
    std::string Lang() const;
    /// Returns country part (CZ, may be empty)
    std::string Country() const;
    /// Returns language+country parts, without the variant
    std::string LangAndCountry() const;
    /// Returns optional variant (after @, e.g. 'latin', typically empty)
    std::string Variant() const;

    /// Return language tag for the language, per BCP 47, e.g. en-US or sr-Latn
    std::string LanguageTag() const { return m_tag; }

    /// Returns name of the locale suitable for ICU
    std::string IcuLocaleName() const { return LanguageTag(); }
    /// Returns ICU equivalent of the language info
    icu::Locale ToIcu() const;

    /// Returns name of this language suitable for display to the user in current UI language
    wxString DisplayName() const;

    /// Like DisplayName(), but shorted (no country/variant).
    wxString LanguageDisplayName() const;

    /// Returns name of this language in itself
    wxString DisplayNameInItself() const;

    /**
        Human-readable (if possible) form usable for round-tripping, i.e.
        understood by TryParse(). Typically "language (country)" in UI language.
        
        @see AllFormattedNames()
     */
    wxString FormatForRoundtrip() const;

    /**
        Return all formatted language names known, in sorted order.
        
        @see FormatForRoundtrip()
     */
    static const std::vector<std::wstring>& AllFormattedNames();

    /**
        Return appropriate plural form for this language.

        @return Plural form expression suitable for directly using in the
                gettext header or empty string if no record was found.
     */
    std::string DefaultPluralFormsExpr() const;

    /// Returns language's text writing direction
    TextDirection Direction() const { return m_direction; }

    /// Returns true if the language is written right-to-left.
    bool IsRTL() const { return m_direction == TextDirection::RTL; }

    /**
        Tries to parse the string as language identification.

        Accepts various forms:
         - standard code (cs, cs_CZ, cs_CZ@latin, ...)
         - ditto with nonstandard capitalization
         - language name in English or current UI language
         - ditto for "language (country)"

        Returned language instance is either invalid if the value couldn't
        be parsed or a language with normalized language code.

        @note
        This function does *not* validate language codes: if @a s has standard
        form, the codes are assumed to be valid.  Use TryParseWithValidation()
        if you are not sure.
     */
    static Language TryParse(const std::wstring& s);
    static Language TryParse(const std::string& s)
        { return TryParse(std::wstring(s.begin(), s.end())); }

    /**
        Like TryParse(), but only accepts language codes if they are known
        valid ISO 639/3166 codes.
     */
    static Language TryParseWithValidation(const std::wstring& s);

    /**
        Tries to create the language from Poedit's legacy X-Poedit-Language and
        X-Poedit-Country headers.
     */
    static Language FromLegacyNames(const std::string& lang, const std::string& country);

    /**
        Try to guess the language from filename, if the filename follows some
        commonly used naming pattern.
     */
    static Language TryGuessFromFilename(const wxString& filename);

    /**
        Try to detect the language from UTF-8 text.
        
        Pass @a probableLanguage if the result is likely known, e.g. English
        for source texts.
     */
    static Language TryDetectFromText(const char *buffer, size_t len,
                                      Language probableLanguage = Language());

    /// Returns object for English language
    static Language English() { return Language("en"); }

    /**
        Checks if @a s has the form of language code.
     */
    static bool IsValidCode(const std::wstring& s);

    bool operator==(const Language& other) const { return m_code == other.m_code; }
    bool operator!=(const Language& other) const { return m_code != other.m_code; }
    bool operator<(const Language& other) const { return m_code < other.m_code; }

private:
    Language(const std::string& code) { Init(code); }
    Language(const std::wstring& code) { Init(std::string(code.begin(), code.end())); }
    void Init(const std::string& code);

private:
    std::string m_code;
    std::string m_tag;
    TextDirection m_direction;
};

#endif // Poedit_language_h
