/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 2003-2017 Vaclav Slavik
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

#ifndef _CHOOSELANG_H_
#define _CHOOSELANG_H_

#include <wx/string.h>
#include <wx/intl.h>

#ifdef __WXMSW__
    #define NEED_CHOOSELANG_UI 1
#else
    #define NEED_CHOOSELANG_UI 0
#endif

#if NEED_CHOOSELANG_UI

/// Let the user change UI language
void ChangeUILanguage();

/** Return currently chosen language. Calls ChooseLanguage if necessary. */
wxString GetUILanguage();

#endif // NEED_CHOOSELANG_UI

#endif
