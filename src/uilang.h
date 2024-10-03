/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 2003-2024 Vaclav Slavik
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

#ifndef Poedit_uilang_h
#define Poedit_uilang_h

#include "language.h"

#include <wx/intl.h>
#include <wx/string.h>
#include <wx/translation.h>

#ifdef __WXMSW__
    #define NEED_CHOOSELANG_UI 1
#else
    #define NEED_CHOOSELANG_UI 0
#endif


/**
    Customized loader for translations.

    The primary purpose of this class is to overcome wx bugs or shortcomings:

    - https://github.com/wxWidgets/wxWidgets/pull/24297
    - https://github.com/wxWidgets/wxWidgets/pull/24804

    Note that this relies on specific knowledge of Poedit's shipping data, it
    is _not_ a universal replacement!
 */
class PoeditTranslationsLoader : public wxFileTranslationsLoader
{
public:
    /**
        Use ICU to determine UI languages; replaces wxTranslations::GetBestTranslation().

        Always returns a valid language (using English as fallback).
     */
    Language DetermineBestUILanguage() const;

    // overrides to use language tags:
    wxArrayString GetAvailableTranslations(const wxString& domain) const override;
    wxMsgCatalog *LoadCatalog(const wxString& domain, const wxString& lang_) override;
};


#if NEED_CHOOSELANG_UI

/// Let the user change UI language
void ChangeUILanguage();

/** Return currently chosen language. Calls ChooseLanguage if necessary. */
Language GetUILanguage();

#endif

#endif // Poedit_uilang_h
