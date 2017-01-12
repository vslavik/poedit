/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 2014-2017 Vaclav Slavik
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

#ifndef Poedit_spellchecking_h
#define Poedit_spellchecking_h

#include <wx/defs.h>

#ifdef __WXMSW__
    #include <wx/platinfo.h>
#endif

#include <wx/textctrl.h>

#include "language.h"

inline bool IsSpellcheckingAvailable()
{
#ifdef __WXMSW__
    return wxPlatformInfo::Get().CheckOSVersion(6,2);
#else
    return true;
#endif
}

#ifdef __WXOSX__
// Set the global spellchecking language
bool SetSpellcheckerLang(const wxString& lang);
#endif

// Does any initialization needed to be able to use spellchecker with the control later.
#ifdef __WXMSW__
void PrepareTextCtrlForSpellchecker(wxTextCtrl *text);
#endif

// Init given text control to do (or not) spellchecking for given language
bool InitTextCtrlSpellchecker(wxTextCtrl *text, bool enable, const Language& lang);

#ifndef __WXMSW__
// Show help about how to add more dictionaries for spellchecking.
void ShowSpellcheckerHelp();
#endif

#endif // Poedit_spellchecking_h
