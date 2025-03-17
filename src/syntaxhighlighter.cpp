/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 2014-2025 Vaclav Slavik
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

#include "syntaxhighlighter.h"

#include "catalog.h"
#include "str_helpers.h"

#include <unicode/uchar.h>
#include <regex>

namespace
{

class BasicSyntaxHighlighter : public SyntaxHighlighter
{
public:
    void Highlight(const std::wstring& s, const CallbackType& highlight) override
    {
        if (s.empty())
            return;

        const int length = int(s.length());

        // Leading whitespace:
        for (auto i = s.begin(); i != s.end(); ++i)
        {
            if (!u_isblank(*i))
            {
                int wlen = int(i - s.begin());
                if (wlen)
                    highlight(0, wlen, LeadingWhitespace);
                break;
            }
        }

        // Trailing whitespace:
        for (auto i = s.rbegin(); i != s.rend(); ++i)
        {
            if (!u_isblank(*i))
            {
                int wlen = int(i - s.rbegin());
                if (wlen)
                    highlight(length - wlen, length, LeadingWhitespace);
                break;
            }
        }

        int blank_block_pos = -1;

        for (auto i = s.begin(); i != s.end(); ++i)
        {
            // Some special whitespace characters should always be highlighted:
            if (*i == 0x00A0 /*non-breakable space*/)
            {
                int pos = int(i - s.begin());
                highlight(pos, pos + 1, LeadingWhitespace);
            }

            // Duplicate whitespace (2+ spaces etc.):
            else if (u_isblank(*i))
            {
                if (blank_block_pos == -1)
                    blank_block_pos = int(i - s.begin());
            }
            else if (blank_block_pos != -1)
            {
                int endpos = int(i - s.begin());
                if (endpos - blank_block_pos >= 2)
                    highlight(blank_block_pos, endpos, LeadingWhitespace);
                blank_block_pos = -1;
            }

            // Escape sequences:
            if (*i == '\\')
            {
                int pos = int(i - s.begin());
                if (++i == s.end())
                    break;
                // Note: this must match AnyTranslatableTextCtrl::EscapePlainText()
                switch (*i)
                {
                    case '0':
                    case 'a':
                    case 'b':
                    case 'f':
                    case 'n':
                    case 'r':
                    case 't':
                    case 'v':
                    case '\\':
                        highlight(pos, pos + 2, Escape);
                        break;
                    default:
                        break;
                }
            }
        }
    }
};



/// Highlighter that runs multiple sub-highlighters
class CompositeSyntaxHighlighter : public SyntaxHighlighter
{
public:
    void Add(std::shared_ptr<SyntaxHighlighter> h) { m_sub.push_back(h); }

    void Highlight(const std::wstring& s, const CallbackType& highlight) override
    {
        for (auto h : m_sub)
            h->Highlight(s, highlight);
    }

private:
    std::vector<std::shared_ptr<SyntaxHighlighter>> m_sub;
};



/// Match regular expressions for highlighting
class RegexSyntaxHighlighter : public SyntaxHighlighter
{
public:
    static const std::wregex::flag_type flags = std::regex_constants::ECMAScript | std::regex_constants::optimize;

    RegexSyntaxHighlighter(const wchar_t *regex, TextKind kind) : m_re(regex, flags), m_kind(kind) {}
    RegexSyntaxHighlighter(const std::wregex& regex, TextKind kind) : m_re(regex), m_kind(kind) {}

    void Highlight(const std::wstring& s, const CallbackType& highlight) override
    {
        try
        {
            std::wsregex_iterator next(s.begin(), s.end(), m_re);
            std::wsregex_iterator end;
            while (next != end)
            {
                auto match = *next++;
                if (match.empty())
                    continue;
                int pos = static_cast<int>(match.position());
                highlight(pos, pos + static_cast<int>(match.length()), m_kind);
            }
        }
        catch (std::regex_error& e)
        {
            switch (e.code())
            {
                case std::regex_constants::error_complexity:
                case std::regex_constants::error_stack:
                    // MSVC version of std::regex in particular can fail to match
                    // e.g. HTML regex with backreferences on insanely large strings;
                    // in that case, don't highlight instead of failing outright.
                    return;
                default:
                    throw;
            }
        }
    }

private:
    std::wregex m_re;
    TextKind m_kind;
};


std::wregex REOBJ_HTML_MARKUP(LR"((<\/?[a-zA-Z0-9:-]+(\s+[-:\w]+(=([-:\w+]|"[^"]*"|'[^']*'))?)*\s*\/?>)|(&[^ ;]+;))",
                              RegexSyntaxHighlighter::flags);

// variables expansion for various template languages
std::wregex REOBJ_COMMON_PLACEHOLDERS(
                //
                //           |             |
                LR"(%[\w.-]+%|%?\{[\w.-]+\}|\{\{[\w.-]+\}\})",
                //      |    |      |      |        |
                //      |           |               |
                //      |           |               +----------------------- {{var}}
                //      |           |
                //      |           +--------------------------------------- %{var} (Ruby) and {var}
                //      |
                //      +--------------------------------------------------- %var% (Twig)
                //
                RegexSyntaxHighlighter::flags);
const wchar_t* COMMON_PLACEHOLDERS_TRIGGER_CHARS = L"{%";

// WebExtension-like $foo$ placeholders
std::wregex REOBJ_DOLLAR_PLACEHOLDERS(LR"(\$[A-Za-z0-9_]+\$)", RegexSyntaxHighlighter::flags);

// php-format per http://php.net/manual/en/function.sprintf.php plus positionals
const wchar_t* RE_PHP_FORMAT = LR"(%(\d+\$)?[-+]{0,2}([ 0]|'.)?-?\d*(\..?\d+)?[%bcdeEfFgGosuxX])";

// c-format per http://en.cppreference.com/w/cpp/io/c/fprintf,
//              http://pubs.opengroup.org/onlinepubs/9699919799/functions/fprintf.html
#define RE_C_FORMAT_BASE LR"(%(\d+\$)?[-+ #0]{0,5}(\d+|\*)?(\.(\d+|\*))?((hh|ll|[hljztL])?[%csdioxXufFeEaAgGnp]|<[A-Za-z0-9]+>))"
const wchar_t* RE_C_FORMAT = RE_C_FORMAT_BASE;
const wchar_t* RE_OBJC_FORMAT =  L"%@|" RE_C_FORMAT_BASE;
const wchar_t* RE_CXX20_OR_RUST_FORMAT = LR"((\{\{)|(\}\})|(\{[^}]*\}))";

// Python and Perl-libintl braces format (also covered by common placeholders above)
#define RE_BRACES LR"(\{[\w.-:,]+\})"

// python-format old style https://docs.python.org/2/library/stdtypes.html#string-formatting
//               new style https://docs.python.org/3/library/string.html#format-string-syntax
const wchar_t* RE_PYTHON_FORMAT = LR"((%(\(\w+\))?[-+ #0]?(\d+|\*)?(\.(\d+|\*))?[hlL]?[diouxXeEfFgGcrs%]))" // old style
                                  "|"
                                  RE_BRACES; // new style

// ruby-format per https://ruby-doc.org/core-2.7.1/Kernel.html#method-i-sprintf
const wchar_t* RE_RUBY_FORMAT = LR"(%(\d+\$)?[-+ #0]{0,5}(\d+|\*)?(\.(\d+|\*))?(hh|ll|[hljztL])?[%csdioxXufFeEaAgGnp])";

// Qt and KDE formats
const wchar_t* RE_QT_FORMAT = LR"(%L?(\d\d?|n))";

// Lua
const wchar_t* RE_LUA_FORMAT = LR"(%[- 0]*\d*(\.\d+)?[sqdiouXxAaEefGgc])";

// Pascal per https://www.freepascal.org/docs-html/rtl/sysutils/format.html
const wchar_t* RE_PASCAL_FORMAT = LR"(%(\*:|\d*:)?-?(\*|\d+)?(\.\*|\.\d+)?[dDuUxXeEfFgGnNmMsSpP])";


} // anonymous namespace


SyntaxHighlighterPtr SyntaxHighlighter::ForItem(const CatalogItem& item, int kindsMask, int flags)
{
    auto fmt = item.GetFormatFlag();
    if (fmt.empty())
        fmt = item.GetInternalFormatFlag();

    bool needsHTML = (kindsMask & Markup);
    if (needsHTML)
    {
        needsHTML = false;

        str::wstring_conv_t str1 = str::to_wstring(item.GetString());
        if (str1.find(L"<") != std::wstring::npos && std::regex_search(str1, REOBJ_HTML_MARKUP))
        {
            needsHTML = true;
        }
        else if (item.HasPlural())
        {
            str::wstring_conv_t strp = str::to_wstring(item.GetString());
            if (strp.find(L"<") != std::wstring::npos && std::regex_search(strp, REOBJ_HTML_MARKUP))
            {
                needsHTML = true;
            }
        }
    }
    bool needsGenericPlaceholders = (kindsMask & Placeholder);
    if (needsGenericPlaceholders)
    {
        if ((flags & EnforceFormatTag) && !fmt.empty())
        {
            // only use generic placeholders if no explicit format was provided, see https://github.com/vslavik/poedit/issues/777
            needsGenericPlaceholders = false;
        }
        else
        {
            needsGenericPlaceholders = false;

            str::wstring_conv_t str1 = str::to_wstring(item.GetString());
            if (str1.find_first_of(COMMON_PLACEHOLDERS_TRIGGER_CHARS) != std::wstring::npos && std::regex_search(str1, REOBJ_COMMON_PLACEHOLDERS))
            {
				needsGenericPlaceholders = true;
			}
            else if (item.HasPlural())
            {
                str::wstring_conv_t strp = str::to_wstring(item.GetString());
                if (strp.find_first_of(COMMON_PLACEHOLDERS_TRIGGER_CHARS) != std::wstring::npos && std::regex_search(strp, REOBJ_COMMON_PLACEHOLDERS))
                {
                    needsGenericPlaceholders = true;
                }
            }
        }
    }

    static auto basic = std::make_shared<BasicSyntaxHighlighter>();
    if (!needsHTML && !needsGenericPlaceholders && fmt.empty())
    {
        if (kindsMask & (LeadingWhitespace | Escape))
            return basic;
        else
            return nullptr;
    }

    auto all = std::make_shared<CompositeSyntaxHighlighter>();

    // HTML goes first, has lowest priority than special-purpose stuff like format strings:
    if (needsHTML)
    {
        static auto html = std::make_shared<RegexSyntaxHighlighter>(REOBJ_HTML_MARKUP, TextKind::Markup);
        all->Add(html);
    }

    if (needsGenericPlaceholders)
    {
        // If no format specified, heuristically apply highlighting of common variable markers
        static auto placeholders = std::make_shared<RegexSyntaxHighlighter>(REOBJ_COMMON_PLACEHOLDERS, TextKind::Placeholder);
        all->Add(placeholders);
    }

    if (!fmt.empty() && (kindsMask & Placeholder))
    {
        if (fmt == "php")
        {
            static auto php_format = std::make_shared<RegexSyntaxHighlighter>(RE_PHP_FORMAT, TextKind::Placeholder);
            all->Add(php_format);
        }
        else if (fmt == "c")
        {
            static auto c_format = std::make_shared<RegexSyntaxHighlighter>(RE_C_FORMAT, TextKind::Placeholder);
            all->Add(c_format);
        }
        else if (fmt == "c++" || fmt == "rust")
        {
            static auto cxx_rust_format = std::make_shared<RegexSyntaxHighlighter>(RE_CXX20_OR_RUST_FORMAT, TextKind::Placeholder);
            all->Add(cxx_rust_format);
        }
        else if (fmt == "python")
        {
            static auto python_format = std::make_shared<RegexSyntaxHighlighter>(RE_PYTHON_FORMAT, TextKind::Placeholder);
            all->Add(python_format);
        }
        else if (fmt == "ruby")
        {
            static auto ruby_format = std::make_shared<RegexSyntaxHighlighter>(RE_RUBY_FORMAT, TextKind::Placeholder);
            all->Add(ruby_format);
        }
        else if (fmt == "objc")
        {
            static auto objc_format = std::make_shared<RegexSyntaxHighlighter>(RE_OBJC_FORMAT, TextKind::Placeholder);
            all->Add(objc_format);
        }
        else if (fmt == "qt" || fmt == "qt-plural" || fmt == "kde" || fmt == "kde-kuit")
        {
            static auto qt_format = std::make_shared<RegexSyntaxHighlighter>(RE_QT_FORMAT, TextKind::Placeholder);
            all->Add(qt_format);
        }
        else if (fmt == "lua")
        {
            static auto lua_format = std::make_shared<RegexSyntaxHighlighter>(RE_LUA_FORMAT, TextKind::Placeholder);
            all->Add(lua_format);
        }
        else if (fmt == "csharp" || fmt == "perl-brace" || fmt == "python-brace")
        {
            static auto brace_format = std::make_shared<RegexSyntaxHighlighter>(RE_BRACES, TextKind::Placeholder);
            all->Add(brace_format);
        }
        else if (fmt == "object-pascal")
        {
            static auto pascal_format = std::make_shared<RegexSyntaxHighlighter>(RE_PASCAL_FORMAT, TextKind::Placeholder);
            all->Add(pascal_format);
        }
        else if (fmt == "ph-dollars")
        {
            static auto dollars_format = std::make_shared<RegexSyntaxHighlighter>(REOBJ_DOLLAR_PLACEHOLDERS, TextKind::Placeholder);
            all->Add(dollars_format);
        }
    }

    // basic highlighting has highest priority, so should come last in the order:
    if (kindsMask & (LeadingWhitespace | Escape))
        all->Add(basic);

    return all;
}
