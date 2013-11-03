/*
 *  This file is part of Poedit (http://www.poedit.net)
 *
 *  Copyright (C) 2013 Vaclav Slavik
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

#include "languagectrl.h"

IMPLEMENT_DYNAMIC_CLASS(LanguageCtrl, wxComboBox)

void LanguageCtrl::SetLang(const Language& lang)
{
    if (!m_inited)
        Init();

    SetValue(lang.FormatForRoundtrip());
}

Language LanguageCtrl::GetLang() const
{
    return Language::TryParse(GetValue());
}

void LanguageCtrl::Init()
{
    SetHint(_("Language Code or Name (e.g. en_GB)"));

    for (auto x: Language::AllFormattedNames())
        Append(x);

    m_inited = true;
}

#ifdef __WXMSW__
wxSize LanguageCtrl::DoGetBestSize() const
{
    // wxComboBox's implementation is insanely slow, at least on MSW.
    // Hardcode a value instead, it doesn't matter for Poedit's use anyway,
    // this control's best size is not the determining factor.
    return GetSizeFromTextSize(100);
}
#endif