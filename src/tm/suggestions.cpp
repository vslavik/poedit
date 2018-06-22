/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 2014-2018 Vaclav Slavik
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

#include "suggestions.h"

#include "concurrency.h"
#include "transmem.h"


class SuggestionsProviderImpl
{
public:
    SuggestionsProviderImpl() {}

    dispatch::future<SuggestionsList> SuggestTranslation(SuggestionsBackend& backend, const SuggestionQuery&& q)
    {
        auto bck = &backend;
        return dispatch::async([=]{
            // don't bother asking the backend if the language or query is invalid:
            if (!q.srclang.IsValid() || !q.lang.IsValid() || q.srclang == q.lang || q.source.empty())
            {
                return dispatch::make_ready_future(SuggestionsList());
            }

            // query the backend:
            return bck->SuggestTranslation(std::move(q));
        });
    }
};



SuggestionsProvider::SuggestionsProvider() : m_impl(new SuggestionsProviderImpl)
{
}

SuggestionsProvider::~SuggestionsProvider()
{
}

dispatch::future<SuggestionsList> SuggestionsProvider::SuggestTranslation(SuggestionsBackend& backend, const SuggestionQuery&& q)
{
    return m_impl->SuggestTranslation(backend, std::move(q));
}

void SuggestionsProvider::Delete(const Suggestion& s)
{
    if (s.id.empty())
        return;

    switch (s.source)
    {
        case Suggestion::Source::LocalTM:
            TranslationMemory::Get().Delete(s.id);
            break;
    }
}
