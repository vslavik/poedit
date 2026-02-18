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

#include "pretranslate.h"

#include "configuration.h"
#include "progress.h"
#include "str_helpers.h"
#include "tm/transmem.h"

#include <wx/stopwatch.h>

#include <atomic>
#include <deque>
#include <mutex>


namespace pretranslate
{

Stats PreTranslateCatalog(CatalogPtr catalog, const CatalogItemArray& range, PreTranslateOptions options, dispatch::cancellation_token_ptr cancellation_token)
{
    wxStopWatch sw;

    if (range.empty())
        return {};

    const bool use_local_tm = Config::UseTM();
    if (!use_local_tm)
        return {};

    TranslationMemory& tm = TranslationMemory::Get();
    auto srclang = catalog->GetSourceLanguage();
    auto lang = catalog->GetLanguage();
    const auto flags = options.flags;

    Progress top_progress(1);
    top_progress.message(_(L"Preparing strings…"));

    // Function to apply fetched suggestions to a catalog item:
    auto process_results = [=](CatalogItemPtr dt, unsigned index, const SuggestionsList& results) -> ResType
    {
        if (results.empty())
            return ResType::None;
        auto& res = results.front();
        if ((flags & PreTranslate_OnlyExact) && !res.IsExactMatch())
            return ResType::Rejected;

        if ((flags & PreTranslate_OnlyGoodQuality) && res.score < 0.80)
            return ResType::None;

        dt->SetTranslation(res.text, index);
        dt->SetPreTranslated(true);

        bool isFuzzy = true;
        if (res.IsExactMatch() && (flags & PreTranslate_ExactNotFuzzy))
        {
            if (results.size() > 1 && results[1].IsExactMatch())
            {
                // more than one exact match is ambiguous, so keep it flagged for review
            }
            else
            {
                isFuzzy = false;
            }
        }
        dt->SetFuzzy(isFuzzy);

        return res.IsExactMatch() ? ResType::Exact : ResType::Fuzzy;
    };

    Stats stats;

    std::vector<dispatch::future<ResType>> operations;
    for (auto dt: range)
    {
        if (dt->IsTranslated() && !dt->IsFuzzy())
            continue;

        stats.input_strings_count++;

        operations.push_back(dispatch::async([=,&tm]() -> ResType
        {
            if (cancellation_token->is_cancelled())
                return {};

            auto results = tm.Search(srclang, lang, str::to_wstring(dt->GetString()));
            auto rt = process_results(dt, 0, results);

            if (translated(rt) && dt->HasPlural())
            {
                switch (lang.nplurals())
                {
                    case 2:  // "simple" English-like plurals
                    {
                        auto results_plural = tm.Search(srclang, lang, str::to_wstring(dt->GetPluralString()));
                        process_results(dt, 1, results_plural);
                    }
                    case 1:  // nothing else to do
                    default: // not supported
                        break;
                }
            }

            return rt;
        }));
    }

    {
        Progress progress((int)operations.size());
        progress.message(_(L"Pre-translating from translation memory…"));

        for (auto& op: operations)
        {
            if (cancellation_token->is_cancelled())
                break;

            auto rt = op.get();
            stats.add(rt);
            if (translated(rt))
            {
                progress.message(wxString::Format(wxPLURAL("Pre-translated %u string", "Pre-translated %u strings", stats.matched), stats.matched));
            }
            progress.increment();
        }
    }

    wxLogTrace("poedit", "Pre-translation completed in %ld ms", sw.Time());
    return stats;
}

} // namespace pretranslate
