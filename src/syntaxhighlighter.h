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

#ifndef Poedit_syntaxhighlighter_h
#define Poedit_syntaxhighlighter_h

#include <string>
#include <functional>

/**
    Highlights parts of translation (or source) text that have special meaning.
    
    Highlighted parts include e.g. C-style escape sequences (\n etc.),
    leading/trailing whitespace or format string parts.
 */
class SyntaxHighlighter
{
public:
    // Kind of the element to highlight
    enum TextKind
    {
        LeadingWhitespace,
        Escape
    };

    typedef std::function<void(int,int,TextKind)> CallbackType;

    /**
        Perform highlighting in given text.
        
        The @a highlight function is called once for every range that should
        be highlighted, with the range boundaries and highlight kind as its
        arguments.
     */
    void Highlight(const std::wstring& s, CallbackType highlight);
};


#endif // Poedit_syntaxhighlighter_h
