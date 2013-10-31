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

#include "language.h"

#include "icuhelpers.h"

#include <cctype>
#include <regex>
#include <algorithm>
#include <unordered_map>

namespace
{

// see http://www.gnu.org/software/gettext/manual/html_node/Header-Entry.html
// for description of permitted formats
const std::wregex RE_LANG_CODE(L"([a-z]){2,3}(_[A-Z]{2})?(@[a-z]+)?");

// a more permissive variant of the same that TryNormalize() would fix
const std::wregex RE_LANG_CODE_PERMISSIVE(L"([a-zA-Z]){2,3}([_-][a-zA-Z]{2})?(@[a-zA-Z]+)?");

// try some normalizations: s/-/_/, case adjustments
void TryNormalize(std::wstring& s)
{
    auto begin = s.begin();
    auto end = s.end();

    size_t pos = s.rfind('@');
    if (pos != std::wstring::npos)
    {
        for (auto x = begin + pos; x != end; ++x)
            *x = std::tolower(*x);
        end = begin + pos;
    }

    bool upper = false;
    for (auto x = begin; x != end; ++x)
    {
        if (*x == '-')
            *x = '_';
        if (*x == '_')
            upper = true;
        else if (std::isupper(*x) && !upper)
            *x = std::tolower(*x);
        else if (std::islower(*x) && upper)
            *x = std::toupper(*x);
    }
}

} // anonymous namespace


bool Language::IsValidCode(const std::wstring& s)
{
    return std::regex_match(s, RE_LANG_CODE);
}

std::string Language::Lang() const
{
    return m_code.substr(0, m_code.find_first_of("_@"));
}

std::string Language::Country() const
{
    const size_t pos = m_code.find('_');
    if (pos == std::string::npos)
        return std::string();
    else
        return m_code.substr(pos+1, m_code.rfind('@'));
}

std::string Language::LangAndCountry() const
{
    return m_code.substr(0, m_code.rfind('@'));
}

std::string Language::Variant() const
{
    const size_t pos = m_code.rfind('@');
    if (pos == std::string::npos)
        return std::string();
    else
        return m_code.substr(0, pos);
}


Language Language::TryParse(const std::wstring& s)
{
    if (IsValidCode(s))
        return Language(s);

    if (std::regex_match(s, RE_LANG_CODE_PERMISSIVE))
    {
        std::wstring s2(s);
        TryNormalize(s2);
        if (IsValidCode(s2))
            return Language(s2);
    }

    return Language(); // invalid
}


std::string Language::DefaultPluralFormsExpr() const
{
    if (!IsValid())
        return std::string();

    static const std::unordered_map<std::string, std::string> forms = {
        #include "language_impl_plurals.h"
    };

    auto i = forms.find(m_code);
    if ( i != forms.end() )
        return i->second;

    i = forms.find(LangAndCountry());
    if ( i != forms.end() )
        return i->second;

    i = forms.find(Lang());
    if ( i != forms.end() )
        return i->second;

    return std::string();
}


icu::Locale Language::ToIcu() const
{
    if (IsValid())
        return icu::Locale(LangAndCountry().c_str());
    else
        return icu::Locale::getEnglish();
}


wxString Language::DisplayName() const
{
    icu::UnicodeString s;
    ToIcu().getDisplayName(s);
    return FromIcuStr(s);
}

wxString Language::DisplayNameInItself() const
{
    auto loc = ToIcu();
    icu::UnicodeString s;
    loc.getDisplayName(loc, s);
    return FromIcuStr(s);
}