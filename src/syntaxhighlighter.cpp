/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 2014-2016 Vaclav Slavik
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

        for (auto i = s.begin(); i != s.end(); ++i)
        {
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
    RegexSyntaxHighlighter(std::wregex& re) : m_re(re) {}

    void Highlight(const std::wstring& s, const CallbackType& highlight) override
    {
        std::wsregex_iterator next(s.begin(), s.end(), m_re);
        std::wsregex_iterator end;
        while (next != end)
        {
            auto match = *next++;
            if (match.empty())
                continue;
            int pos = static_cast<int>(match.position());
            highlight(pos, pos + static_cast<int>(match.length()), TextKind::Markup);
        }
    }

private:
    std::wregex& m_re;
};


std::wregex RE_HTML_MARKUP(L"<\\/?[a-zA-Z]+(\\s+\\w+(=(\\w+|(\"|').*(\"|')))?)*\\s*\\/?>",
                           std::regex_constants::ECMAScript | std::regex_constants::optimize);

} // anonymous namespace


SyntaxHighlighterPtr SyntaxHighlighter::ForItem(const CatalogItem& item)
{
    bool needsHTML = item.GetString().Contains('<');

    static auto basic = std::make_shared<BasicSyntaxHighlighter>();
    if (!needsHTML)
        return basic;

    auto all = std::make_shared<CompositeSyntaxHighlighter>();

    if (needsHTML)
    {
        static auto html = std::make_shared<RegexSyntaxHighlighter>(RE_HTML_MARKUP);
        all->Add(html);
    }

    // basic higlighting has highest priority, so should come last in the order:
    all->Add(basic);

    return all;
}
