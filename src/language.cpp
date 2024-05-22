/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 2013-2024 Vaclav Slavik
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

#include "str_helpers.h"
#include "unicode_helpers.h"

#include <cctype>
#include <algorithm>
#include <unordered_map>
#include <mutex>
#include <memory>
#include <regex>
#include <set>

#include <boost/algorithm/string.hpp>

#include <unicode/utypes.h>

#include <wx/filename.h>

#include "pluralforms/pl_evaluate.h"

#ifdef HAVE_CLD2
    #ifdef HAVE_CLD2_PUBLIC_COMPACT_LANG_DET_H
        #include <cld2/public/compact_lang_det.h>
        #include <cld2/public/encodings.h>
    #else
        #include "public/compact_lang_det.h"
        #include "public/encodings.h"
    #endif
#endif

namespace
{

// see http://www.gnu.org/software/gettext/manual/html_node/Header-Entry.html
// for description of permitted formats
const std::wregex RE_LANG_CODE(L"([a-z]){2,3}(_([A-Z]{2}|[0-9]{3}))?(@[a-z]+)?");

// a more permissive variant of the same that TryNormalize() would fix
const std::wregex RE_LANG_CODE_PERMISSIVE(L"([a-zA-Z]){2,3}([_-]([a-zA-Z]{2}|[0-9]{3}))?(@[a-zA-Z]+)?");

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
    for (const char * const* i = uloc_getISOLanguages(); *i != nullptr; ++i)
    {
        if (strcmp(test, *i) == 0)
            return true;
    }
    return false;
}

bool IsISOCountry(const std::string& s)
{
    const char *test = s.c_str();
    for (const char * const* i = uloc_getISOCountries(); *i != nullptr; ++i)
    {
        if (strcmp(test, *i) == 0)
            return true;
    }
    return false;
}

// Get locale display name or at least language
template<typename T>
auto GetDisplayNameOrLanguage(const char *locale, const char *displayLocale)
{
    UErrorCode err = U_ZERO_ERROR;
    UChar buf[512] = {0};
    uloc_getDisplayName(locale, displayLocale, buf, std::size(buf), &err);
    if (U_FAILURE(err) || str::empty(buf))
    {
        err = U_ZERO_ERROR;
        uloc_getDisplayLanguage(locale, displayLocale, buf, std::size(buf), &err);
    }

    return str::to<T>(buf);
}


// Mapping of names to their respective ISO codes.
struct DisplayNamesData
{
    typedef std::unordered_map<std::u16string, std::string> Map;
    Map names, namesEng;
    std::vector<std::wstring> sortedNames;
};

std::once_flag of_namesList;

const DisplayNamesData& GetDisplayNamesData()
{
    static DisplayNamesData data;

    std::call_once(of_namesList, [=]{
        std::set<std::string> foundCodes;

        int32_t count = uloc_countAvailable();
        data.sortedNames.reserve(count);

        char language[128] = {0};
        char script[128] = {0};
        char country[128] = {0};
        char variant[128] = {0};

        for (int i = 0; i < count; i++)
        {
            const char *locale = uloc_getAvailable(i);

            UErrorCode err = U_ZERO_ERROR;
            uloc_getLanguage(locale, language, std::size(language), &err);
            uloc_getScript(locale, script, std::size(script), &err);
            uloc_getCountry(locale, country, std::size(country), &err);
            uloc_getVariant(locale, variant, std::size(variant), &err);

            // TODO: for now, ignore variants here and in FormatForRoundtrip(),
            //       because translating them between gettext and ICU is nontrivial
            if (!str::empty(variant))
                continue;

            UChar buf[512] = {0};
            err = U_ZERO_ERROR;
            uloc_getDisplayName(locale, nullptr, buf, std::size(buf), &err);

            data.sortedNames.emplace_back(str::to_wstring(buf));
            auto foldedName = unicode::fold_case_to_type<std::u16string>(buf);

            if (strcmp(language, "zh") == 0 && *country == '\0')
            {
                if (strcmp(script, "Hans") == 0)
                    strncpy(country, "CN", std::size(country));
                else if (strcmp(script, "Hant") == 0)
                    strncpy(country, "TW", std::size(country));
            }

            std::string code(language);
            if (*country != '\0')
            {
                code += '_';
                code += country;
            }
            if (*script != '\0')
            {
                if (strcmp(script, "Latn") == 0)
                {
                    code += "@latin";
                }
                else if (strcmp(script, "Cyrl") == 0)
                {
                    // add @cyrillic only if it's not the default already
                    if (strcmp(language, "sr") != 0)
                        code += "@cyrillic";
                }
            }

            foundCodes.insert(code);
            data.names[foldedName] = code;
 
            err = U_ZERO_ERROR;
            uloc_getDisplayName(locale, ULOC_ENGLISH, buf, std::size(buf), &err);
            auto foldedEngName = unicode::fold_case_to_type<std::u16string>(buf);

            data.namesEng[foldedEngName] = code;
        }

        // add languages that are not listed as locales in ICU:
        for (const char * const* i = uloc_getISOLanguages(); *i != nullptr; ++i)
        {
            const char *code = *i;
            if (foundCodes.find(code) != foundCodes.end())
                continue;

            UErrorCode err = U_ZERO_ERROR;
            uloc_getLanguage(code, language, std::size(language), &err);
            if (U_FAILURE(err) || strcmp(code, language) != 0)
                continue; // e.g. 'und' for undetermined language

            auto isoName = GetDisplayNameOrLanguage<std::wstring>(code, nullptr);
            if (isoName.empty())
                continue;

            data.sortedNames.push_back(isoName);
            data.names[unicode::fold_case_to_type<std::u16string>(isoName)] = code;

            auto isoEngName = GetDisplayNameOrLanguage<std::wstring>(code, ULOC_ENGLISH);
            if (!isoEngName.empty())
                data.namesEng[unicode::fold_case_to_type<std::u16string>(isoEngName)] = code;
        }

        // sort the names alphabetically for data.sortedNames:
        unicode::Collator coll(unicode::Collator::case_insensitive);
        std::sort(data.sortedNames.begin(), data.sortedNames.end(), std::ref(coll));
    });

    return data;
}

std::string DoGetLanguageTag(const Language& lang)
{
    if (lang.Code() == "zh_CN")
         return "zh-Hans";
    else if (lang.Code() == "zh_TW")
         return "zh-Hant";

    auto l = lang.Lang();
    auto c = lang.Country();
    auto v = lang.Variant();

    std::string tag = l;

    if (v == "latin")
    {
        tag += "-Latn";
        v.clear();
    }
    else if (v == "cyrillic")
    {
        tag += "-Cyrl";
        v.clear();
    }

    if (!c.empty())
        tag += "-" + c;

    if (!v.empty())
    {
        // Encode variant that wasn't special-handled as a private use subtag, see
        // https://tools.ietf.org/html/rfc5646#section-2.2.7 (e.g. "de-DE-x-formal")
        tag += "-x-" + v;
    }

    return tag;
}

inline bool DoIsRTL(const Language& lang)
{
    auto locale = lang.IcuLocaleName();
    return uloc_isRightToLeft(locale.c_str());
}

} // anonymous namespace


void Language::Init(const std::string& code)
{
    m_code = code;

    if (IsValid())
    {
        m_tag = DoGetLanguageTag(*this);
        m_icuLocale = m_tag;

        char locale[512];
        UErrorCode status = U_ZERO_ERROR;
        uloc_forLanguageTag(m_tag.c_str(), locale, 512, NULL, &status);
        if (U_SUCCESS(status))
            m_icuLocale = locale;

        m_direction = DoIsRTL(*this) ? TextDirection::RTL : TextDirection::LTR;
    }
    else
    {
        m_tag.clear();
        m_icuLocale.clear();
        m_direction = TextDirection::LTR;
    }
}

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

    const size_t endpos = m_code.rfind('@');
    if (endpos == std::string::npos)
        return m_code.substr(pos+1);
    else
        return m_code.substr(pos+1, endpos - (pos+1));
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
        return m_code.substr(pos + 1);
}

Language Language::MinimizeSubtags() const
{
    if (m_icuLocale.empty())
        return *this;

    char minimized[512];
    UErrorCode status = U_ZERO_ERROR;
    uloc_minimizeSubtags(m_icuLocale.c_str(), minimized, 512, &status);
    if (U_FAILURE(status))
        return *this;

    char tag[512];
    uloc_toLanguageTag(minimized, tag, 512, /*strict=*/1, &status);
    if (U_FAILURE(status))
        return *this;

    if (strcmp(tag, "zh") == 0)
        return Language::FromLanguageTag("zh-Hans");

    return Language::FromLanguageTag(tag);
}

Language Language::TryParse(const std::wstring& s)
{
    if (s.empty())
        return Language(); // invalid

    if (IsValidCode(s))
        return Language(s);

    if (s == L"zh-Hans")
        return Language("zh_CN");
    else if (s == L"zh-Hant")
        return Language("zh_TW");

    // Is it a standard language code?
    if (std::regex_match(s, RE_LANG_CODE_PERMISSIVE))
    {
        std::wstring s2(s);
        TryNormalize(s2);
        if (IsValidCode(s2))
            return Language(s2);
    }

    // If not, perhaps it's a human-readable name (perhaps coming from the language control)?
    auto names = GetDisplayNamesData();
    auto folded = unicode::fold_case_to_type<std::u16string>(s);
    auto i = names.names.find(folded);
    if (i != names.names.end())
        return Language(i->second);

    // Maybe it was in English?
    i = names.namesEng.find(folded);
    if (i != names.namesEng.end())
        return Language(i->second);

    // Maybe it was a BCP 47 language tag?
    auto fromTag = FromLanguageTag(str::to_utf8(s));
    if (fromTag.IsValid())
        return fromTag;

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


Language Language::FromLanguageTag(const std::string& tag)
{
    if (tag.empty())
        return Language(); // invalid

    char locale[512];
    UErrorCode status = U_ZERO_ERROR;
    auto len = uloc_forLanguageTag(tag.c_str(), locale, 512, NULL, &status);
    if (U_FAILURE(status) || !len)
        return Language();

    Language lang;
    lang.m_tag = tag;
    lang.m_icuLocale = locale;

    char buf[512];
    if (uloc_getLanguage(locale, buf, 512, &status))
        lang.m_code = buf;
    if (uloc_getCountry(locale, buf, 512, &status))
        lang.m_code += "_" + std::string(buf);

    // ICU converts private use subtag into 'x' keyword, e.g. de-DE-x-formal => de_DE@x=formal
    static const std::regex re_private_subtag("@x=([^@]+)$");
    std::cmatch m;
    if (std::regex_search(locale, m, re_private_subtag))
        lang.m_code += "@" + m.str(1);

    lang.m_direction = DoIsRTL(lang) ? TextDirection::RTL : TextDirection::LTR;

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


PluralFormsExpr Language::DefaultPluralFormsExpr() const
{
    if (!IsValid())
        return PluralFormsExpr();

    static const std::unordered_map<std::string, PluralFormsExpr> forms = {
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

    // fall back to English-like singular+plural
    return PluralFormsExpr::English();
}


int Language::nplurals() const
{
    return DefaultPluralFormsExpr().nplurals();
}


wxString Language::DisplayName() const
{
    return GetDisplayNameOrLanguage<wxString>(m_icuLocale.c_str(), nullptr);
}

wxString Language::LanguageDisplayName() const
{
    UErrorCode err = U_ZERO_ERROR;
    UChar buf[512] = {0};
    uloc_getDisplayLanguage(m_icuLocale.c_str(), nullptr, buf, std::size(buf), &err);
    return str::to_wx(buf);
}

wxString Language::DisplayNameInItself() const
{
    auto name = GetDisplayNameOrLanguage<wxString>(m_icuLocale.c_str(), m_icuLocale.c_str());
    if (!name.empty())
        return name;

    // fall back to current locale's name, better than nothing
    return DisplayName();
}

wxString Language::FormatForRoundtrip() const
{
    // Can't show all variants nicely, but some common one can be
    auto v = Variant();
    if (!v.empty() && v != "latin" && v != "cyrillic")
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


Language Language::TryGuessFromFilename(const wxString& filename, wxString *wildcard)
{
    if (wildcard)
        wildcard->clear();

    wxFileName fn(filename);
    fn.MakeAbsolute();

    // Try matching the filename first:
    //  - entire name
    //  - suffix (foo.cs_CZ.po, wordpressTheme-cs_CZ.po)
    //  - directory name (cs_CZ, cs.lproj, cs/LC_MESSAGES)
    std::wstring name = fn.GetName().ToStdWstring();
    Language lang = Language::TryParseWithValidation(name);
    if (lang.IsValid())
    {
        if (wildcard)
        {
            fn.SetName("*");
            *wildcard = fn.GetFullPath();
        }
        return lang;
    }

    size_t pos = name.find_first_of(L".-_");
    while (pos != wxString::npos)
    {
        auto part = name.substr(pos+1);
        lang = Language::TryParseWithValidation(part);
        if (lang.IsValid())
        {
            if (wildcard)
            {
                name.replace(pos+1, std::wstring::npos, L"*");
                fn.SetName(name);
                *wildcard = fn.GetFullPath();
            }
            return lang;
        }
        pos = name.find_first_of(L".-_",  pos+1);
    }

    auto dirs = fn.GetDirs();
    if (!dirs.empty())
    {
        size_t i = dirs.size() - 1;
        if (dirs[i].IsSameAs("LC_MESSAGES", /*caseSensitive=*/false))
        {
            if (i == 0)
                return Language(); // failed to match
            --i;
        }
        wxString rest, wmatch;
        if (dirs[i].EndsWith(".lproj", &rest))
        {
            lang = Language::TryParseWithValidation(rest.ToStdWstring());
            wmatch = "*.lproj";
        }
        else
        {
            lang = Language::TryParseWithValidation(dirs[i].ToStdWstring());
            wmatch = "*";
        }
        if (lang.IsValid())
        {
            if (wildcard)
            {
                fn.RemoveDir(i);
                fn.InsertDir(i, wmatch);
                *wildcard = fn.GetFullPath();
            }
            return lang;
        }
    }

    return Language(); // failed to match
}


Language Language::TryDetectFromText(const char *buffer, size_t len, Language probableLanguage)
{
#ifdef HAVE_CLD2
    using namespace CLD2;

    CLDHints hints = {NULL, NULL, UNKNOWN_ENCODING, UNKNOWN_LANGUAGE};
    if (probableLanguage.IsValid())
    {
        if (probableLanguage.Lang() == "en")
            hints.language_hint = ENGLISH;
        else
            hints.language_hint = GetLanguageFromName(probableLanguage.LanguageTag().c_str());
    }

    // three best guesses; we don't care, but they must be passed in
    CLD2::Language language3[3];
    int percent3[3];
    double normalized_score3[3];
    // more result info:
    int text_bytes;
    bool is_reliable;

    auto lang = CLD2::ExtDetectLanguageSummary(
                        buffer, (int)len,
                        /*is_plain_text=*/false,
                        &hints,
                        /*flags=*/0,
                        language3, percent3, normalized_score3,
                        /*resultchunkvector=*/nullptr,
                        &text_bytes,
                        &is_reliable);

    if (!is_reliable &&
        language3[0] == lang &&
        language3[1] == UNKNOWN_LANGUAGE && language3[2] == UNKNOWN_LANGUAGE &&
        percent3[1] == 0 && percent3[2] == 0)
    {
        // supposedly unreliable, but no other alternatives detected, so use it
        is_reliable = true;
    }

    if (lang == UNKNOWN_LANGUAGE || !is_reliable)
        return Language();

    // CLD2 penalizes English in bilingual content in some cases as "boilerplate"
    // because it is tailored for the web. So e.g. 66% English, 33% Italian is
    // tagged as Italian.
    //
    // Poedit's bias is the opposite: English is almost always the correct answer
    // for PO source language. Fix this up manually.
    if (lang != language3[0] && language3[0] == CLD2::ENGLISH && language3[1] == lang)
        lang = language3[0];

    return Language::TryParse(LanguageCode(lang));
#else
    (void)buffer;
    (void)len;
    return probableLanguage;
#endif
}


PluralFormsExpr::PluralFormsExpr() : m_calcCreated(true)
{
}

PluralFormsExpr::PluralFormsExpr(const std::string& expr, int nplurals)
    : m_expr(expr), m_nplurals(nplurals), m_calcCreated(false)
{
}

PluralFormsExpr::~PluralFormsExpr()
{
}

int PluralFormsExpr::nplurals() const
{
    if (m_nplurals != -1)
        return m_nplurals;
    if (m_calc)
        return m_calc->nplurals();

    const std::regex re("^nplurals=([0-9]+)");
    std::smatch m;
    if (std::regex_match(m_expr, m, re))
        return std::stoi(m.str(1));
    else
        return -1;
}

std::shared_ptr<PluralFormsCalculator> PluralFormsExpr::calc() const
{
    auto self = const_cast<PluralFormsExpr*>(this);
    if (m_calcCreated)
        return m_calc;
    if (!m_expr.empty())
        self->m_calc = PluralFormsCalculator::make(m_expr.c_str());
    self->m_calcCreated = true;
    return m_calc;
}

bool PluralFormsExpr::operator==(const PluralFormsExpr& other) const
{
    if (m_expr == other.m_expr)
        return true;

    // do some normalization to avoid unnecessary complains when the only
    // differences are in whitespace for example:
    auto expr1 = boost::erase_all_copy(m_expr, " \t");
    auto expr2 = boost::erase_all_copy(other.m_expr, " \t");
    if (expr1 == expr2)
        return true;

    // failing that, compare the expressions semantically:
    auto calc1 = calc();
    auto calc2 = other.calc();

    if (!calc1 || !calc2)
        return false; // at least one is invalid _and_ the strings are different due to code above

    if (calc1->nplurals() != calc2->nplurals())
        return false;

    for (int i = 0; i < MAX_EXAMPLES_COUNT; i++)
    {
        if (calc1->evaluate(i) != calc2->evaluate(i))
            return false;
    }
    // both expressions are identical on all tested integers
    return true;
}

int PluralFormsExpr::evaluate_for_n(int n) const
{
    auto c = calc();
    return c ? c->evaluate(n) : 0;
}

PluralFormsExpr PluralFormsExpr::English()
{
    return PluralFormsExpr("nplurals=2; plural=(n != 1);", 2);
}
