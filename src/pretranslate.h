/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 1999-2017 Vaclav Slavik
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

#ifndef Poedit_pretranslate_h
#define Poedit_pretranslate_h

#include "catalog.h"
#include "edlistctrl.h"

#include <wx/window.h>

#include <functional>


/// Flags for pre-translation functions
enum PreTranslateFlags
{
    PreTranslate_OnlyExact       = 0x01,
    PreTranslate_ExactNotFuzzy   = 0x02,
    PreTranslate_OnlyGoodQuality = 0x04
};

/**
    Pre-translate a range of items.
    
    If not nullptr, report # of pre-translated items in @a matchesCount.
    
    Returns true if any changes were made.
 */
template<typename T>
bool PreTranslateCatalog(wxWindow *window, CatalogPtr catalog, const T& range, int flags, int *matchesCount);

/**
    Pre-translate all items in the catalog.
    
    If not nullptr, report # of pre-translated items in @a matchesCount

    Returns true if any changes were made.
 */
bool PreTranslateCatalog(wxWindow *window, CatalogPtr catalog, int flags, int *matchesCount);

/**
    Show UI for choosing pre-translation choices, then proceed with
    pre-translation unless cancelled (in which case false is returned).

    @param parent   Parent window
    @param list     List to take selection from
    @param catalog  Catalog to translate

    Returns true if any changes were made.
 */
void PreTranslateWithUI(wxWindow *window, PoeditListCtrl *list,
                        CatalogPtr catalog,
                        std::function<void()> onChangesMade);

#endif // Poedit_pretranslate_h
