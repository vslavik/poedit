/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 2014-2020 Vaclav Slavik
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
                    case 'n':
                    case 'r':
                    case 't':
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
    /// Ctor. Notice that @a re is a reference and must outlive the highlighter!
    RegexSyntaxHighlighter(std::wregex& re, TextKind kind) : m_re(re), m_kind(kind) {}

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
    std::wregex& m_re;
    TextKind m_kind;
};


std::wregex RE_HTML_MARKUP(LR"((<\/?[a-zA-Z0-9:-]+(\s+[-:\w]+(=([-:\w+]|"[^"]*"|'[^']*'))?)*\s*\/?>)|(&[^ ;]+;))",
                           std::regex_constants::ECMAScript | std::regex_constants::optimize);

// php-format per http://php.net/manual/en/function.sprintf.php plus positionals
std::wregex RE_PHP_FORMAT(LR"(%(\d+\$)?[-+]{0,2}([ 0]|'.)?-?\d*(\..?\d+)?[%bcdeEfFgGosuxX])",
                          std::regex_constants::ECMAScript | std::regex_constants::optimize);

// c-format per http://en.cppreference.com/w/cpp/io/c/fprintf,
//              http://pubs.opengroup.org/onlinepubs/9699919799/functions/fprintf.html
std::wregex RE_C_FORMAT(LR"(%(\d+\$)?[-+ #0]{0,5}(\d+|\*)?(\.(\d+|\*))?(hh|ll|[hljztL])?[%csdioxXufFeEaAgGnp])",
                        std::regex_constants::ECMAScript | std::regex_constants::optimize);

// ruby-format per https://ruby-doc.org/core-2.7.1/Kernel.html#method-i-sprintf
std::wregex RE_RUBY_FORMAT(LR"(%(\d+\$)?[-+ #0]{0,5}(\d+|\*)?(\.(\d+|\*))?(hh|ll|[hljztL])?[%csdioxXufFeEaAgGnp])",
                           std::regex_constants::ECMAScript | std::regex_constants::optimize);

// variables expansion for %foo% (Twig), %{foo} (Ruby), {foo} and {{foo}}
std::wregex RE_COMMON_PLACEHOLDERS(LR"((%[0-9a-zA-Z_.-]+%)|(%?\{[0-9a-zA-Z_.-]+\})|(\{\{[0-9a-zA-Z_.-]+\}\}))",
                    std::regex_constants::ECMAScript | std::regex_constants::optimize);

} // anonymous namespace


SyntaxHighlighterPtr SyntaxHighlighter::ForItem(const CatalogItem& item)
{
    auto formatFlag = item.GetFormatFlag();
    bool needsHTML = std::regex_search(str::to_wstring(item.GetString()), RE_HTML_MARKUP);
    bool needsPlaceholders = std::regex_search(str::to_wstring(item.GetString()), RE_COMMON_PLACEHOLDERS);

    static auto basic = std::make_shared<BasicSyntaxHighlighter>();
    if (!needsHTML && !needsPlaceholders && formatFlag.empty())
        return basic;

    auto all = std::make_shared<CompositeSyntaxHighlighter>();

    // HTML goes first, has lowest priority than special-purpose stuff like format strings:
    if (needsHTML)
    {
        static auto html = std::make_shared<RegexSyntaxHighlighter>(RE_HTML_MARKUP, TextKind::Markup);
        all->Add(html);
    }

    if (needsPlaceholders)
    {
        // If no format specified, heuristically apply highlighting of common variable markers
        static auto placeholders = std::make_shared<RegexSyntaxHighlighter>(RE_COMMON_PLACEHOLDERS, TextKind::Format);
        all->Add(placeholders);
    }

    // TODO: more/all languages
    if (formatFlag == "php")
    {
        static auto php_format = std::make_shared<RegexSyntaxHighlighter>(RE_PHP_FORMAT, TextKind::Format);
        all->Add(php_format);
    }
    else if (formatFlag == "c")
    {
        static auto c_format = std::make_shared<RegexSyntaxHighlighter>(RE_C_FORMAT, TextKind::Format);
        all->Add(c_format);
    }
    else if (formatFlag == "ruby")
    {
        static auto ruby_format = std::make_shared<RegexSyntaxHighlighter>(RE_RUBY_FORMAT, TextKind::Format);
        all->Add(ruby_format);
    }

    // basic highlighting has highest priority, so should come last in the order:
    all->Add(basic);

    return all;
}
