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

#include <unicode/uchar.h>

void SyntaxHighlighter::Highlight(const std::wstring& s, SyntaxHighlighter::CallbackType highlight)
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
