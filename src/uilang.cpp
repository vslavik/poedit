/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 2003-2025 Vaclav Slavik
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

#include "uilang.h"

#include "language.h"
#include "str_helpers.h"

#include <wx/config.h>
#include <wx/uilocale.h>

#include <unicode/uloc.h>


namespace
{

// Return whether uloc_acceptLanguage() is working correctly
inline bool icu_has_correct_accept_language()
{
    // ICU 67.1 reimplemented uloc_acceptLanguage() to use the same algorithm as LocaleMatcher:
    // https://unicode-org.atlassian.net/browse/ICU-20700
    // Without this, it couldn't be reliably used to determine the best language from OS-provided
    // list of locales that might be too specific (e.g. cs-CZ). Unfortunately, Windows 10 shipped
    // with ICU 64.2, so we need to handle older versions at least somehow too.
    UVersionInfo version;
    u_getVersion(version);
    auto major = version[0];
    auto minor = version[1];
    return major > 67 || (major == 67 && minor >= 1);
}

template<typename T>
inline void to_vectors(T&& list, std::vector<std::string>& strings, std::vector<const char*>& cstrings)
{
    strings.reserve(list.size());
    cstrings.reserve(list.size());
    for (const auto& s: list)
    {
        strings.push_back(str::to_utf8(s));
        cstrings.push_back(strings.back().c_str());
    }
}

// converts to POSIX-like locale, is idempotent
inline wxString as_posix(const wxString& tag)
{
    wxString s(tag);
    s.Replace("-", "_");
    s.Replace("zh_Hans", "zh_CN");
    s.Replace("zh_Hant", "zh_TW");
    s.Replace("_Latn", "@latin");
    return s;
}

// converts to language tag, is idempotent
inline wxString as_tag(const wxString& posix)
{
    wxString s(posix);
    s.Replace("_", "-");
    s.Replace("zh-CN", "zh-Hans");
    s.Replace("zh-TW", "zh-Hant");
    s.Replace("@latin", "-Latn");
    return s;
}


auto get_preferred_languages()
{
    auto langs = wxUILocale::GetPreferredUILanguages();

    if (!icu_has_correct_accept_language())
    {
        // Windows 10's ICU won't accept "cs" translation if "cs-CZ" is in preferred list, so
        // patch up the list by adding "base" languages to the end of the list too, much like
        // wx-3.2's implementation does.
        auto size = langs.size();
        for (size_t i = 0; i < size; ++i)
        {
            auto& lang = langs[i];
            lang = as_tag(lang);

            auto pos = lang.rfind('-');
            if (pos != wxString::npos)
            {
                auto lang2(lang);
                while (true)
                {
                    lang2 = lang2.substr(0, pos);
                    langs.push_back(lang2);
                    if (lang2 == "sr-Cyrl")
						langs.push_back("sr");
                    if ((pos = lang.rfind('-')) == wxString::npos)
                        break;
                }
            }
        }
    }

    return langs;
}


} // anonymous namespace


/**
    Customized loader for translations.

    The primary purpose of this class is to overcome wx bugs or shortcomings:

    - https://github.com/wxWidgets/wxWidgets/pull/24297
    - https://github.com/wxWidgets/wxWidgets/pull/24804

    Note that this relies on specific knowledge of Poedit's shipping data, it
    is _not_ a universal replacement!
 */
Language PoeditTranslationsLoader::DetermineBestUILanguage() const
{
    std::vector<std::string> available, preferred;
    std::vector<const char*> cavailable, cpreferred;
    to_vectors(GetAvailableTranslations("poedit"), available, cavailable);
    to_vectors(get_preferred_languages(), preferred, cpreferred);

    char best[ULOC_FULLNAME_CAPACITY];
    UAcceptResult result;
    UErrorCode status = U_ZERO_ERROR;
    UEnumeration *enumLangs = uenum_openCharStringsEnumeration(cavailable.data(), (int32_t)cavailable.size(), &status);
    if (U_FAILURE(status))
        return Language::English();

    status = U_ZERO_ERROR;
    uloc_acceptLanguage(best, std::size(best), &result, cpreferred.data(), (int32_t)cpreferred.size(), enumLangs, &status);
    uenum_close(enumLangs);
    if (U_FAILURE(status) || result == ULOC_ACCEPT_FAILED)
        return Language::English();

    char tag[ULOC_FULLNAME_CAPACITY];
    status = U_ZERO_ERROR;
    uloc_toLanguageTag(best, tag, std::size(tag), false, &status);
    if (U_FAILURE(status))
        return Language::English();

    return Language::FromLanguageTag(tag);
}


wxArrayString PoeditTranslationsLoader::GetAvailableTranslations(const wxString& domain) const
{
    auto all = wxFileTranslationsLoader::GetAvailableTranslations(domain);

    for (auto& lang: all)
        lang = as_tag(lang);
    all.push_back("en");

    return all;
}


wxMsgCatalog *PoeditTranslationsLoader::LoadCatalog(const wxString& domain, const wxString& lang_)
{
#ifdef __WXOSX__
    auto lang = (domain == "poedit-ota") ? as_posix(lang_) : as_tag(lang_);
#else
    auto lang = as_posix(lang_);
#endif

    return wxFileTranslationsLoader::LoadCatalog(domain, lang);
}


#if NEED_CHOOSELANG_UI

static void SaveUILanguage(const wxString& lang)
{
    if (lang.empty())
        wxConfig::Get()->Write("ui_language", "default");
    else
        wxConfig::Get()->Write("ui_language", as_tag(lang));
}


Language GetUILanguage()
{
    std::string lng = str::to_utf8(as_tag(wxConfig::Get()->Read("ui_language")));
    if (lng.empty() || lng == "default")
        return Language();

    auto lang = Language::FromLanguageTag(lng);
    if (!lang)
        lang = Language::TryParse(lng); // backward compatibility

    auto all = wxTranslations::Get()->GetAvailableTranslations("poedit");
    if (all.Index(lang.LanguageTag(), false) == wxNOT_FOUND)
        return Language();

    return lang;
}


static bool ChooseLanguage(wxString *value)
{
    wxArrayString langs;
    wxArrayString arr;

    {
        wxBusyCursor bcur;
        langs = wxTranslations::Get()->GetAvailableTranslations("poedit");
        langs.Sort();

        arr.push_back(_("(Use default language)"));
        for (auto i : langs)
        {
            auto lang = Language::TryParse(i.ToStdWstring());
            arr.push_back(lang.DisplayNameInItself() + L"  —  " + lang.DisplayName());
        }
    }

    auto current = GetUILanguage();
    int choice = current ? 0 : langs.Index(current.LanguageTag()) + 1;

    choice = wxGetSingleChoiceIndex(_("Select your preferred language"), _("Language selection"), arr, choice);
    if ( choice == -1 )
        return false;

    if ( choice == 0 )
        *value = "";
    else
        *value = langs[choice-1];
    return true;
}

void ChangeUILanguage()
{
    wxString lang;
    if ( !ChooseLanguage(&lang) )
        return;
    SaveUILanguage(lang);
    wxMessageBox(_("You must restart Poedit for this change to take effect."),
                 "Poedit",
                 wxOK | wxCENTRE | wxICON_INFORMATION);
}

#endif // NEED_CHOOSELANG_UI
