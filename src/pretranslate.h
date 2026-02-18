/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 1999-2026 Vaclav Slavik
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
#include "concurrency.h"

#include <functional>


/// Flags for pre-translation functions
enum PreTranslateFlags
{
    PreTranslate_OnlyExact       = 0x01,
    PreTranslate_ExactNotFuzzy   = 0x02,
    PreTranslate_OnlyGoodQuality = 0x04
};

/// Options passed to pre-translation functions
struct PreTranslateOptions
{
    explicit PreTranslateOptions(int flags_ = 0) : flags(flags_) {}

    /// Flags, a combination of PreTranslateFlags values
    int flags;
};

// semi-private namespace for pretranslate_ui.*:
namespace pretranslate
{

enum class ResType
{
    None = 0,  // no matches
    Rejected,  // found matches, but rejected by settings
    Fuzzy,     // approximate match
    Exact      // exact match
};

inline bool translated(ResType r) { return r >= ResType::Fuzzy; }


struct Stats
{
    int input_strings_count = 0;
    int total = 0;
    int matched = 0;
    int exact = 0;
    int fuzzy = 0;
    int errors = 0;

    explicit operator bool() const { return matched > 0; }

    void add(ResType r)
    {
        total++;
        if (translated(r))
            matched++;
        if (r == ResType::Exact)
            exact++;
        else if (r == ResType::Fuzzy)
            fuzzy++;
    }
};

Stats PreTranslateCatalog(CatalogPtr catalog,
                          const CatalogItemArray& range,
                          PreTranslateOptions options,
                          dispatch::cancellation_token_ptr cancellation_token);

} // namespace pretranslate

#endif // Poedit_pretranslate_h
