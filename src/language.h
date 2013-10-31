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

#ifndef Poedit_language_h
#define Poedit_language_h

#include <wx/string.h>
#include <string>
#include <unicode/locid.h>

/// Representation of translation's language.
class Language
{
public:
    Language() {}

    bool IsValid() const { return !m_code.empty(); }
    const std::string& Code() const { return m_code; }

    /// Returns language part (cs)
    std::string Lang() const;
    /// Returns country part (CZ, may be empty)
    std::string Country() const;
    /// Returns language+country parts, without the variant
    std::string LangAndCountry() const;
    /// Returns optional variant (after @, e.g. 'latin', typically empty)
    std::string Variant() const;

    /// Returns ICU equivalent of the language info
    icu::Locale ToIcu() const;

    /// Returns name of this language suitable for display to the user in current UI language
    wxString DisplayName() const;

    /// Returns name of this language in itself
    wxString DisplayNameInItself() const;

    /**
        Return appropriate plural form for this language.

        @return Plural form expression suitable for directly using in the
                gettext header or empty string if no record was found.
     */
    std::string DefaultPluralFormsExpr() const;

    /**
        Tries to parse the string as language identification.
        
        Accepts various forms:
         - standard code (cs, cs_CZ, cs_CZ@latin, ...)
         - ditto with nonstandard capitalization
         - language name in English or current UI language
         - ditto for "language (country)"

        Returned language instance is either invalid if the value couldn't
        be parsed or a language with normalized language code.
     */
    static Language TryParse(const std::wstring& s);

    /**
        Checks if @a s has the form of language code.
     */
    static bool IsValidCode(const std::wstring& s);

    bool operator==(const Language& other) const { return m_code == other.m_code; }
    bool operator!=(const Language& other) const { return m_code != other.m_code; }

protected:
    Language(const std::string& code) : m_code(code) {}
    Language(const std::wstring& code) : m_code(code.begin(), code.end()) {}

private:
    std::string m_code;
};

#endif // Poedit_language_h
