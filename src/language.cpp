/*
 *  This file is part of Poedit (http://poedit.net)
 *
 *  Copyright (C) 2013-2014 Vaclav Slavik
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
#include <algorithm>
#include <unordered_map>
#include <mutex>
#include <memory>

#include <unicode/locid.h>
#include <unicode/coll.h>

#include <wx/filename.h>

// GCC's libstdc++ didn't have functional std::regex implementation until 4.9
#if (defined(__GNUC__) && !defined(__clang__) && !wxCHECK_GCC_VERSION(4,9))
    #include <boost/regex.hpp>
    using boost::wregex;
    using boost::regex_match;
#else
    #include <regex>
    using std::wregex;
    using std::regex_match;
#endif

namespace
{

// see http://www.gnu.org/software/gettext/manual/html_node/Header-Entry.html
// for description of permitted formats
const wregex RE_LANG_CODE(L"([a-z]){2,3}(_[A-Z]{2})?(@[a-z]+)?");

// a more permissive variant of the same that TryNormalize() would fix
const wregex RE_LANG_CODE_PERMISSIVE(L"([a-zA-Z]){2,3}([_-][a-zA-Z]{2})?(@[a-zA-Z]+)?");

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

bool IsISOLanguage(const std::string& s)
{
    const char *test = s.c_str();
    for (const char * const* i = icu::Locale::getISOLanguages(); *i != nullptr; ++i)
    {
        if (strcmp(test, *i) == 0)
            return true;
    }
    return false;
}

bool IsISOCountry(const std::string& s)
{
    const char *test = s.c_str();
    for (const char * const* i = icu::Locale::getISOCountries(); *i != nullptr; ++i)
    {
        if (strcmp(test, *i) == 0)
            return true;
    }
    return false;
}

// Mapping of names to their respective ISO codes.
struct DisplayNamesData
{
    typedef std::unordered_map<std::wstring, std::string> Map;
    Map names, namesEng;
    std::vector<std::wstring> sortedNames;
};

std::once_flag of_namesList;

const DisplayNamesData& GetDisplayNamesData()
{
    static DisplayNamesData data;

    std::call_once(of_namesList, [=]{
        auto locEng = icu::Locale::getEnglish();
        std::vector<icu::UnicodeString> names;

        int32_t count;
        const icu::Locale *loc = icu::Locale::getAvailableLocales(count);
        names.reserve(count);
        for (int i = 0; i < count; i++, loc++)
        {
            // TODO: for now, ignore variants here and in FormatForRoundtrip(),
            //       because translating them between gettext and ICU is nontrivial
            auto variant = loc->getVariant();
            if (variant != nullptr && *variant != '\0')
                continue;

            icu::UnicodeString s;
            loc->getDisplayName(s);

            names.push_back(s);

            s.foldCase();
            data.names[StdFromIcuStr(s)] = loc->getName();

            loc->getDisplayName(locEng, s);
            s.foldCase();
            data.namesEng[StdFromIcuStr(s)] = loc->getName();
        }

        // sort the names alphabetically for data.sortedNames:
        UErrorCode err = U_ZERO_ERROR;
        std::unique_ptr<icu::Collator> coll(icu::Collator::createInstance(err));
        if (coll)
        {
            coll->setStrength(icu::Collator::SECONDARY); // case insensitive

            std::sort(names.begin(), names.end(),
                      [&coll](const icu::UnicodeString& a, const icu::UnicodeString& b){
                          UErrorCode e = U_ZERO_ERROR;
                          return coll->compare(a, b, e) == UCOL_LESS;
                      });
        }
        else
        {
            std::sort(names.begin(), names.end());
        }

        // convert into std::wstring
        data.sortedNames.reserve(names.size());
        for (auto s: names)
            data.sortedNames.push_back(StdFromIcuStr(s));
    });

    return data;
}

} // anonymous namespace


bool Language::IsValidCode(const std::wstring& s)
{
    return regex_match(s, RE_LANG_CODE);
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

    // Is it a standard language code?
    if (regex_match(s, RE_LANG_CODE_PERMISSIVE))
    {
        std::wstring s2(s);
        TryNormalize(s2);
        if (IsValidCode(s2))
            return Language(s2);
    }

    // If not, perhaps it's a human-readable name (perhaps coming from the language control)?
    auto names = GetDisplayNamesData();
    icu::UnicodeString s_icu = ToIcuStr(s);
    s_icu.foldCase();
    std::wstring folded = StdFromIcuStr(s_icu);
    auto i = names.names.find(folded);
    if (i != names.names.end())
        return Language(i->second);

    // Maybe it was in English?
    i = names.namesEng.find(folded);
    if (i != names.namesEng.end())
        return Language(i->second);

    return Language(); // invalid
}


Language Language::TryParseWithValidation(const std::wstring& s)
{
    Language lang = Language::TryParse(s);
    if (!lang.IsValid())
        return Language(); // invalid

    if (!IsISOLanguage(lang.Lang()))
        return Language(); // invalid

    auto country = lang.Country();
    if (!country.empty() && !IsISOCountry(country))
        return Language(); // invalid

    return lang;
}


Language Language::FromLegacyNames(const std::string& lang, const std::string& country)
{
    if (lang.empty())
        return Language(); // invalid

    #include "language_impl_legacy.h"

    std::string code;
    auto i = isoLanguages.find(lang);
    if ( i != isoLanguages.end() )
        code = i->second;
    else
        return Language(); // invalid

    if (!country.empty())
    {
        auto iC = isoCountries.find(country);
        if ( iC != isoCountries.end() )
            code += "_" + iC->second;
    }

    return Language(code);
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

wxString Language::FormatForRoundtrip() const
{
    // TODO: Can't show variants nicely yet, not standardized
    if (!Variant().empty())
        return m_code;

    wxString disp = DisplayName();
    // ICU isn't 100% reliable, some of the display names it produces
    // (e.g. "Chinese (China)" aren't in the list of known locale names
    // (here because zh-Trans is preferred to zh_CN). So make sure it can
    // be parsed back first.
    if (TryParse(disp.ToStdWstring()).IsValid())
        return disp;
    else
        return m_code;
}


const std::vector<std::wstring>& Language::AllFormattedNames()
{
    return GetDisplayNamesData().sortedNames;
}


Language Language::TryGuessFromFilename(const wxString& filename)
{
    wxFileName fn(filename);
    fn.MakeAbsolute();

    // Try matching the filename first:
    //  - entire name
    //  - suffix (foo.cs_CZ.po, wordpressTheme-cs_CZ.po)
    //  - directory name (cs_CZ, cs.lproj, cs/LC_MESSAGES)
    std::wstring name = fn.GetName().ToStdWstring();
    Language lang = Language::TryParseWithValidation(name);
            if (lang.IsValid())
                return lang;

    size_t pos = name.find_first_of(L".-_");
    while (pos != wxString::npos)
    {
        auto part = name.substr(pos+1);
        lang = Language::TryParseWithValidation(part);
        if (lang.IsValid())
            return lang;
         pos = name.find_first_of(L".-_",  pos+1);
    }

    auto dirs = fn.GetDirs();
    if (!dirs.empty())
    {
        auto d = dirs.rbegin();
        if (d->IsSameAs("LC_MESSAGES", /*caseSensitive=*/false))
        {
            if (++d == dirs.rend())
                return Language(); // failed to match
        }
        wxString rest;
        if (d->EndsWith(".lproj", &rest))
            return Language::TryParseWithValidation(rest.ToStdWstring());
        else
            return Language::TryParseWithValidation(d->ToStdWstring());
    }

    return Language(); // failed to match
}
