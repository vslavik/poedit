/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 2017-2026 Vaclav Slavik
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

#include "qa_checks.h"

#include "syntaxhighlighter.h"

#include <regex>
#include <set>
#include <unicode/uchar.h>
#include <wx/translation.h>


// -------------------------------------------------------------
// QACheck implementations
// -------------------------------------------------------------

namespace QA
{

#define QA_ENUM_ALL_CHECKS(m)       \
    m(QA::Placeholders);            \
    m(QA::NotAllPlurals);           \
    m(QA::CaseMismatch);            \
    m(QA::WhitespaceMismatch);      \
    m(QA::PunctuationMismatch);     \
    /* end */


#define QA_METADATA(symbolicName, description)                      \
    static const char *GetId() { return symbolicName; }             \
    static wxString GetDescription() { return description; }        \
    const char *GetCheckId() const override { return GetId(); }


std::wregex RE_POSITIONAL_FORMAT(LR"(^%[0-9]\$(.*))", std::regex_constants::ECMAScript | std::regex_constants::optimize);

class Placeholders : public QACheck
{
public:
    QA_METADATA("placeholders", _("Placeholders correctness"))

    Placeholders(Language /*lang*/) {}

    bool CheckItem(CatalogItemPtr item) override
    {
        // this check is expensive, so make sure to run it on fully translated items only
        if (!item->IsTranslated())
            return false;

        auto syntax = SyntaxHighlighter::ForItem(*item, SyntaxHighlighter::Placeholder, SyntaxHighlighter::EnforceFormatTag);
        if (!syntax)
            return false;

        PlaceholdersSet phSource;
        ExtractPlaceholders(phSource, syntax, item->GetString());

        if (item->HasPlural())
        {
            ExtractPlaceholders(phSource, syntax, item->GetPluralString());
            int index = 0;
            for (auto& t: item->GetTranslations())
            {
                if (CheckPlaceholders(phSource, syntax, item, t, index++))
                    return true;
            }
            return false;
        }
        else
        {
            return CheckPlaceholders(phSource, syntax, item, item->GetTranslation());
        }

        return false;
    }

private:
    // TODO: use std::string_view (C++17)
    typedef std::set<std::wstring> PlaceholdersSet;

    bool CheckPlaceholders(const PlaceholdersSet& phSource, SyntaxHighlighterPtr syntax, CatalogItemPtr item, const wxString& str, int pluralIndex = -1)
    {
        PlaceholdersSet phTrans;
        ExtractPlaceholders(phTrans, syntax, str);

        // Check the all placeholders are used in translation:
        for (auto& ph: phSource)
        {
            if (phTrans.find(ph) == phTrans.end())
            {
                // as a special case, allow errors in 1st plural form, because people tend to translate e.g.
                // "%d items" as "One item" for n=1
                if (pluralIndex != 0)
                {
                    item->SetIssue(CatalogItem::Issue::Warning,
                                   wxString::Format(_(L"Placeholder “%s” is missing from translation."), ph));
                    return true;
                }
            }
        }

        // And the other way around, that there are no superfluous ones:
        for (auto& ph: phTrans)
        {
            if (phSource.find(ph) == phSource.end())
            {
                item->SetIssue(CatalogItem::Issue::Warning,
                               wxString::Format(_(L"Superfluous placeholder “%s” that isn’t in source text."), ph));
                return true;
            }
        }

        return false;
    }

    void ExtractPlaceholders(PlaceholdersSet& ph, SyntaxHighlighterPtr syntax, const wxString& str)
    {
        const std::wstring text(str.ToStdWstring());
        syntax->Highlight(text, [&text, &ph, syntax](int a, int b, SyntaxHighlighter::TextKind kind){
            if (kind != SyntaxHighlighter::Placeholder)
                return;

            auto x = text.substr(a, b - a);
            if (x == L"%%")
                return;

            // filter out reordering of positional arguments by tracking them as unordered;
            // e.g. %1$s is translated into %s
            std::wsmatch m;
            if (std::regex_match(x, m, RE_POSITIONAL_FORMAT))
            {
                x = std::wstring(L"%") + std::wstring(m[1]);
            }

            ph.insert(x);
        });
    }
};


class NotAllPlurals : public QACheck
{
public:
    QA_METADATA("allplurals", _("Plural form translations"))

    NotAllPlurals(Language /*lang*/) {}

    bool CheckItem(CatalogItemPtr item) override
    {
        if (!item->HasPlural())
            return false;

        bool foundTranslated = false;
        bool foundEmpty = false;
        for (auto& s: item->GetTranslations())
        {
            if (s.empty())
                foundEmpty = true;
            else
                foundTranslated = true;
        }

        if (foundEmpty && foundTranslated)
        {
            item->SetIssue(CatalogItem::Issue::Warning, _("Not all plural forms are translated."));
            return true;
        }

        return false;
    }
};


class CaseMismatch : public QACheck
{
public:
    QA_METADATA("case", _("Inconsistent upper/lower case"))

    CaseMismatch(Language lang) : m_lang(lang.Lang())
    {
        m_shouldCheck = (m_lang != "zh" && m_lang != "ja" && m_lang != "ka");
    }

    bool CheckString(CatalogItemPtr item, const wxString& source, const wxString& translation) override
    {
        if (!m_shouldCheck || source.length() < 2)
            return false;

        // Detect that the source string is a sentence: should have 1st letter uppercase and 2nd lowercase,
        // as checking just the 1st letter would lead to false positives (consider e.g. "MSP430 built-in"):
        if (u_isupper(source[0]) && u_islower(source[1]) && u_islower(translation[0]))
        {
            item->SetIssue(CatalogItem::Issue::Warning, _("The translation should start as a sentence."));
            return true;
        }

        if (u_islower(source[0]) && u_isupper(translation[0]))
        {
            if (m_lang != "de")
            {
                item->SetIssue(CatalogItem::Issue::Warning, _("The translation should start with a lowercase character."));
                return true;
            }
            // else: German nouns start uppercased, this would cause too many false positives
        }

        return false;
    }

private:
    bool m_shouldCheck;
    std::string m_lang;
};


class WhitespaceMismatch : public QACheck
{
public:
    QA_METADATA("whitespace", _("Inconsistent whitespace"))

    WhitespaceMismatch(Language lang)
    {
        auto l = lang.Lang();
        // Space is used sparingly in these languages and e.g. not present after sentence-ending
        // period. Checking trailing/leading space is therefore often a false positive.
        m_checkSpaceInTranslation = (l != "zh" && l != "ja");
    }

    bool CheckString(CatalogItemPtr item, const wxString& source, const wxString& translation) override
    {
        if (m_checkSpaceInTranslation && u_isspace(source[0]) && !u_isspace(translation[0]))
        {
            item->SetIssue(CatalogItem::Issue::Warning, _(L"The translation doesn’t start with a space."));
            return true;
        }

        if (!u_isspace(source[0]) && u_isspace(translation[0]))
        {
            item->SetIssue(CatalogItem::Issue::Warning, _(L"The translation starts with a space, but the source text doesn’t."));
            return true;
        }

        if (source.Last() == '\n' && translation.Last() != '\n')
        {
            item->SetIssue(CatalogItem::Issue::Warning, _(L"The translation is missing a newline at the end."));
            return true;
        }

        if (source.Last() != '\n' && translation.Last() == '\n')
        {
            item->SetIssue(CatalogItem::Issue::Warning, _(L"The translation ends with a newline, but the source text doesn’t."));
            return true;
        }

        if (m_checkSpaceInTranslation && u_isspace(source.Last()) && !u_isspace(translation.Last()))
        {
            item->SetIssue(CatalogItem::Issue::Warning, _(L"The translation is missing a space at the end."));
            return true;
        }

        if (!u_isspace(source.Last()) && u_isspace(translation.Last()))
        {
            item->SetIssue(CatalogItem::Issue::Warning, _(L"The translation ends with a space, but the source text doesn’t."));
            return true;
        }

        return false;
    }

private:
    bool m_checkSpaceInTranslation;
};


class PunctuationMismatch : public QACheck
{
public:
    QA_METADATA("punctuation", _("Punctuation checks"));

    PunctuationMismatch(Language lang) : m_lang(lang.Lang())
    {
    }

    bool CheckString(CatalogItemPtr item, const wxString& source_, const wxString& translation) override
    {
        if (m_lang == "th" || m_lang == "lo" || m_lang == "km" || m_lang == "my")
        {
            // For Thai, Lao, Khmer and Burmese,
            // the punctuation rules are so different that these checks don't
            // apply at all (with the possible exception of quote marks - TODO).
            // It's better to skip them than to spam the user with bogus warnings
            // on _everything_.
            // See https://www.ccjk.com/punctuation-rule-for-bahasa-vietnamese-and-thai/
            return false;
        }

        auto source(source_);
        if (m_lang == "zh" || m_lang == "ja")
        {
            // Space is used sparingly in these languages andd e.g. not present after sentence-ending
            // period, so strip it from the source if present and check punctuation w/o it.
            if (u_isspace(source.Last()) && !u_isspace(translation.Last()))
                source.Trim(/*fromRight:*/true);
        }

        const UChar32 s_last = source.Last();
        const UChar32 t_last = translation.Last();
        const bool s_punct = IsPunctuation(s_last);
        const bool t_punct = IsPunctuation(t_last);

        if (u_getIntPropertyValue(s_last, UCHAR_BIDI_PAIRED_BRACKET_TYPE) == U_BPT_CLOSE ||
            u_getIntPropertyValue(t_last, UCHAR_BIDI_PAIRED_BRACKET_TYPE) == U_BPT_CLOSE)
        {
            // too many reordering related false positives for brackets
            // e.g. "your {site} account" -> "váš účet na {site}"
            if ((wchar_t)u_getBidiPairedBracket(s_last) != (wchar_t)source[0])
            {
                return false;
            }
            else
            {
                // OTOH, it's desirable to check strings fully enclosed in brackets like "(unsaved)"
                if (source.find_first_of((wchar_t)s_last, 1) != source.size() - 1)
                {
                    // it's more complicated, possibly something like "your {foo} on {bar}"
                    return false;
                }
            }
        }

        if (u_hasBinaryProperty(s_last, UCHAR_QUOTATION_MARK) || (!s_punct && u_hasBinaryProperty(t_last, UCHAR_QUOTATION_MARK)))
        {
            // quoted fragments can move around, e.g., so ignore quotes in reporting:
            //      >> Invalid value for ‘{fieldName}’​ field
            //      >> Valor inválido para el campo ‘{fieldName}’
            // TODO: count quote characters to check if used correctly in translation; don't check position
            return false;
        }

        if (s_punct && !t_punct)
        {
            item->SetIssue(CatalogItem::Issue::Warning,
                           wxString::Format(_(L"The translation should end with “%s”."), wxString(wxUniChar(s_last))));
            return true;
        }
        else if (!s_punct && t_punct)
        {
            if (t_last == '.' && (source.ends_with("st") || source.ends_with("nd") || source.ends_with("th")))
            {
                // special-case English ordinals to languages that use [number].
            }
            else
            {
                item->SetIssue(CatalogItem::Issue::Warning,
                               wxString::Format(_(L"The translation should not end with “%s”."), wxString(wxUniChar(t_last))));
                return true;
            }
        }
        else if (s_punct && t_punct && s_last != t_last)
        {
            if (IsEquivalent(L'…', t_last) && source.ends_with("..."))
            {
                // as a special case, allow translating ... (3 dots) as … (ellipsis)
            }
            else if (u_hasBinaryProperty(s_last, UCHAR_QUOTATION_MARK) && u_hasBinaryProperty(t_last, UCHAR_QUOTATION_MARK))
            {
                // don't check for correct quotes for now, accept any quotations marks as equal
            }
            else if (IsEquivalent(s_last, t_last))
            {
                // some characters are mostly equivalent and we shouldn't warn about them
            }
            else
            {
                item->SetIssue(CatalogItem::Issue::Warning,
                               wxString::Format(_(L"The translation ends with “%s”, but the source text ends with “%s”."),
                                                wxString(wxUniChar(t_last)), wxString(wxUniChar(s_last))));
                return true;
            }
        }

        return false;
    }

private:
    bool IsPunctuation(UChar32 c) const
    {
        return u_hasBinaryProperty(c, UCHAR_TERMINAL_PUNCTUATION) ||
               u_hasBinaryProperty(c, UCHAR_QUOTATION_MARK) ||
               c == L'…' ||  // somehow U+2026 ellipsis is not terminal punctuation
               c == L'⋯';    // ...or Chinese U+22EF
    }

    bool IsEquivalent(UChar32 src, UChar32 trans) const
    {
        if (src == trans)
            return true;

        if (m_lang == "zh" || m_lang == "ja")
        {
            // Chinese uses full-width punctuation.
            // See https://en.wikipedia.org/wiki/Chinese_punctuation
            switch (src)
            {
                case '.':
                    return trans == L'。';
                case ',':
                    return trans == L'，' || trans == L'、';
                case '!':
                    return trans == L'！';
                case '?':
                    return trans == L'？';
                case ':':
                    return trans == L'：';
                case ';':
                    return trans == L'；';
                case '(':
                    return trans == L'（';
                case ')':
                    return trans == L'）';
                case L'…':
                    return trans == L'⋯';
                default:
                    break;
            }
        }
        else if (m_lang == "ar" || m_lang == "fa" || m_lang == "ug")
        {
            // In Arabic (but not other RTL languages), some punctuation is mirrored.
            switch (src)
            {
                case ';':
                    return trans == L'؛';
                case '?':
                    return trans == L'؟';
                case ',':
                    return trans == L'،';
                default:
                    break;
            }
        }
        else if (m_lang == "el")
        {
            // In Greek, questions end with ';' and not '?'.
            if (src == '?')
                return trans == ';';
        }
        else if (m_lang == "hi")
        {
            // In Hindi, full stop is '|'.
            if (src == '.')
                return trans == L'।';
        }
        else if (m_lang == "hy")
        {
            // In Armenian, full stop is '։', which is often substituted with Latin ':'.
            if (src == '.')
                return trans == L'։' || trans == L':';
        }
        else if (m_lang == "ti" || m_lang == "am")
        {
            // Handle Geʽez script script (used by Amharic, Tigrinya and more languages) punctuation:
            switch (trans)
            {
                case L'።':
                    return src == '.';
                case L'፣':
                    return src == ',';
                case L'፤':
                    return src == ';';
                case L'፥':
                    return src == ':';
                case L'፧':
                    return src == '?';
                default:
                    break;

            }
        }

        return false;
    }

    std::string m_lang;
};


} // namespace QA


// -------------------------------------------------------------
// QACheck support code
// -------------------------------------------------------------

bool QACheck::CheckItem(CatalogItemPtr item)
{
    if (!item->GetTranslation().empty() && CheckString(item, item->GetString(), item->GetTranslation()))
        return true;

    if (item->HasPlural())
    {
        unsigned count = item->GetNumberOfTranslations();
        for (unsigned i = 1; i < count; i++)
        {
            auto t = item->GetTranslation(i);
            if (!t.empty() && CheckString(item, item->GetPluralString(), t))
                return true;
        }
    }

    return false;
}


bool QACheck::CheckString(CatalogItemPtr /*item*/, const wxString& /*source*/, const wxString& /*translation*/)
{
    wxFAIL_MSG("not implemented - must override CheckString OR CheckItem");
    return false;
}


// -------------------------------------------------------------
// QAChecker
// -------------------------------------------------------------

QAChecker::QAChecker()
{
}

QAChecker::~QAChecker()
{
}


std::shared_ptr<QAChecker> QAChecker::GetFor(Catalog& catalog)
{
    auto lang = catalog.GetLanguage();
    auto c = std::make_shared<QAChecker>();

    #define qa_instantiate(klass) c->AddCheck<klass>(lang);
    QA_ENUM_ALL_CHECKS(qa_instantiate);

    return c;
}

std::vector<std::pair<std::string, wxString>> QAChecker::GetMetadata()
{
    std::vector<std::pair<std::string, wxString>> m;

    #define qa_metadata(klass) m.emplace_back(klass::GetId(), klass::GetDescription())
    QA_ENUM_ALL_CHECKS(qa_metadata);

    return m;
}



int QAChecker::Check(Catalog& catalog)
{
    int issues = 0;

    for (auto& i: catalog.items())
        issues += Check(i);

    return issues;
}


int QAChecker::Check(CatalogItemPtr item)
{
    int issues = 0;

    for (auto& c: m_checks)
    {
        if (item->GetString().empty() || (item->HasPlural() && item->GetPluralString().empty()))
            continue;

        if (c->CheckItem(item))
        {
            issues++;
            // we only record single issue, so there's no point in continuing with other checks:
            break;
        }
    }

    return issues;
}
