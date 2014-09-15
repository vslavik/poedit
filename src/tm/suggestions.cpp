/*
 *  This file is part of Poedit (http://poedit.net)
 *
 *  Copyright (C) 2014 Vaclav Slavik
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

#include "ThreadPool.h"


class SuggestionsProviderImpl
{
public:
    SuggestionsProviderImpl() : m_pool(2) {}

    void SuggestTranslation(SuggestionsBackend& backend,
                            const std::string& lang,
                            const std::wstring& source,
                            int maxHits,
                            SuggestionsProvider::success_func_type onSuccess,
                            SuggestionsProvider::error_func_type onError)
    {
        auto bck = &backend;
        m_pool.enqueue([=](){
            bck->SuggestTranslation(lang, source, maxHits, onSuccess, onError);
        });
    }

private:
    ThreadPool m_pool;
};



SuggestionsProvider::SuggestionsProvider() : m_impl(new SuggestionsProviderImpl)
{
}

SuggestionsProvider::~SuggestionsProvider()
{
}

void SuggestionsProvider::SuggestTranslation(SuggestionsBackend& backend,
                                             const std::string& lang,
                                             const std::wstring& source,
                                             int maxHits,
                                             success_func_type onSuccess,
                                             error_func_type onError)
{
    m_impl->SuggestTranslation(backend, lang, source, maxHits, onSuccess, onError);
}
