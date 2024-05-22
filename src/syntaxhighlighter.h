/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 2014-2024 Vaclav Slavik
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

#ifndef Poedit_syntaxhighlighter_h
#define Poedit_syntaxhighlighter_h

#include <functional>
#include <memory>
#include <string>

class CatalogItem;

class SyntaxHighlighter;
typedef std::shared_ptr<SyntaxHighlighter> SyntaxHighlighterPtr;

/**
    Highlights parts of translation (or source) text that have special meaning.
    
    Highlighted parts include e.g. C-style escape sequences (\n etc.),
    leading/trailing whitespace or format string parts.
 */
class SyntaxHighlighter
{
public:
    virtual ~SyntaxHighlighter() {}

    // Kind of the element to highlight
    enum TextKind
    {
        LeadingWhitespace  = 0x0001,
        Escape             = 0x0002,
        Markup             = 0x0004,
        Placeholder        = 0x0008
    };

    // Flags for ForItem
    enum
    {
        EnforceFormatTag   = 0x0001
    };

    typedef std::function<void(int,int,TextKind)> CallbackType;

    /**
        Perform highlighting in given text.
        
        The @a highlight function is called once for every range that should
        be highlighted, with the range boundaries (start, end offsets) and highlight kind
        as its arguments.
     */
    virtual void Highlight(const std::wstring& s, const CallbackType& highlight) = 0;

    /**
        Return highlighter suitable for given translation item.

        @param item      Translation item to highlight
        @param kindsMask Optionally specify only a subset of highlighters as TextKind or-combination
        @param flags     Optional flags modifying behavior, e.g. EnforceFormatTag
     */
    static SyntaxHighlighterPtr ForItem(const CatalogItem& item, int kindsMask = 0xffff, int flags = 0);
};

#endif // Poedit_syntaxhighlighter_h
