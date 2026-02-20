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
#include "errors.h"
#include "progress.h"
#include "str_helpers.h"
#include "tm/transmem.h"

#include <wx/stopwatch.h>

#include <boost/thread/thread.hpp>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <deque>
#include <mutex>
#include <thread>

using namespace std::chrono_literals;


namespace pretranslate
{

namespace
{


struct JobMetadata
{
    Language srclang, lang;
    unsigned nplurals;
    PreTranslateOptions options;
};


/**
 Base class for a worked implementing pre-translation process.

 Typically runs the work in some background threads, modifying catalog items passed to it.
 */
class Worker
{
public:
    Worker(const JobMetadata& meta) : m_metadata(meta), m_completed(false) {}
    virtual ~Worker() {}

    /// Add another item for processing
    void upload(CatalogItemPtr item)
    {
        std::lock_guard lock(m_mutex);
        if (m_completed)
            return;
        m_queue.push_back(item);
    }

    /**
        Call to mark the job as done with adding items to the queue.
        Can only be called from the primary thread, i.e. one that created the worker.
     */
    void upload_completed()
    {
        std::lock_guard lock(m_mutex);
        m_completed = true;
    }

    /// Is processing of the entire queue finished?
    bool is_finished() const
    {
        std::lock_guard lock(m_mutex);
        return m_completed && m_queue.empty();
    }

    /**
     Perform some amount of work on primary thread. May take some time, but shouldn't be _long_ time.

     Returns true if there is more work to do, false otherwise.

     Should accomodate cancellation by checking the cancellation token and returning false if cancellation
     is requested, but may e.g. do it inside http request handling, not immediately.

     Can only be called from the primary thread, i.e. one that created the worker.
     */
    virtual bool pump(dispatch::cancellation_token_ptr) = 0;

    /// Assignable next worker to process items this worker couldn't handle
    Worker *next_worker = nullptr;

    /// Assignable stats collector for processed items
    std::shared_ptr<Stats> stats;

protected:
    ResType process_results(CatalogItemPtr dt, unsigned index, const SuggestionsList& results)
    {
        if (results.empty())
            return ResType::None;

        const auto flags = m_metadata.options.flags;
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
    }

    ResType process_result(CatalogItemPtr dt, unsigned index, const Suggestion& result)
    {
        return process_results(dt, index, SuggestionsList{result});
    }

    void clear_queue()
    {
        std::lock_guard lock(m_mutex);
        m_queue.clear();
        m_completed = true;
    }

protected:
    JobMetadata m_metadata;

    mutable std::mutex m_mutex;
    std::deque<CatalogItemPtr> m_queue;
    std::atomic<bool> m_completed;
};


class LocalDBWorker : public Worker
{
public:
    LocalDBWorker(const JobMetadata& meta)
        : Worker(meta), m_tm(TranslationMemory::Get())
    {
        const auto nthreads = std::clamp(std::thread::hardware_concurrency(), 4u, 16u);
        for (unsigned i = 0; i < nthreads; ++i)
        {
            m_threads.create_thread([this]{ thread_worker(); });
        }
    }

    bool pump(dispatch::cancellation_token_ptr cancellation_token) override
    {
        if (cancellation_token->is_cancelled())
        {
            clear_queue();
            // fall through to wait for threads to finish and exit
        }

        if (is_finished())
        {
            m_threads.join_all();
            return false;
        }

        return true;
    }

private:
    void thread_worker()
    {
        CatalogItemPtr dt;

        while (true)
        {
            // pop one item of work:
            {
                std::lock_guard lock(m_mutex);
                if (m_queue.empty())
                {
                    if (m_completed)
                    {
                        break;  // no more work to do
                    }
                    else
                    {
                        std::this_thread::yield();
                        continue; // wait for more work to be added
                    }
                }

                dt = std::move(m_queue.front());
                m_queue.pop_front();
            }

            auto results = m_tm.Search(m_metadata.srclang, m_metadata.lang, str::to_wstring(dt->GetString()));
            auto rt = process_results(dt, 0, results);

            if (translated(rt) && dt->HasPlural())
            {
                switch (m_metadata.nplurals)
                {
                    case 2:  // "simple" English-like plurals
                    {
                        auto results_plural = m_tm.Search(m_metadata.srclang, m_metadata.lang, str::to_wstring(dt->GetPluralString()));
                        process_results(dt, 1, results_plural);
                    }
                    case 1:  // nothing else to do
                    default: // not supported
                        break;
                }
            }

            if (next_worker)
            {
                if (!translated(rt))
                {
                    // no usable translation, request elsewhere
                    next_worker->upload(dt);
                    continue;
                }
                else
                {
                    // usable local translation, but try to find better quality elsewhere if possible
                    auto score = results.front().score;
                    if (score < 0.95)
                    {
                        next_worker->upload(dt);
                        continue;
                    }
                }
            }

            // if the item wasn't passed to next worker, count it
            if (stats)
            {
                stats->inc_processed();
                stats->add(rt);
            }
        }
    }

private:
    boost::thread_group m_threads;
    TranslationMemory& m_tm;
};


} // anonymous namespace


std::shared_ptr<Stats> PreTranslateCatalog(CatalogPtr catalog,
                                           const CatalogItemArray& range,
                                           PreTranslateOptions options,
                                           dispatch::cancellation_token_ptr cancellation_token)
{
    wxStopWatch sw;

    auto stats = std::make_shared<Stats>();

    if (range.empty())
        return stats;

    Progress top_progress(1);
    top_progress.message(_(L"Preparing strings…"));

    const bool use_local_tm = Config::UseTM();
    if (!use_local_tm)
        return stats;

    JobMetadata metadata;
    metadata.srclang = catalog->GetSourceLanguage();
    metadata.lang = catalog->GetLanguage();
    metadata.nplurals = metadata.lang.nplurals();
    metadata.options = options;

    auto worker_local = use_local_tm ? std::make_unique<LocalDBWorker>(metadata) : nullptr;

    if (worker_local)
        worker_local->stats = stats;

    Worker *worker_ingest = worker_local.get();

    // Feed in the work to the worker:
    for (auto dt: range)
    {
        if (dt->IsTranslated() && !dt->IsFuzzy())
            continue;

        stats->input_strings_count++;
        worker_ingest->upload(dt);
    }
    worker_ingest->upload_completed();

    // Wait for completion:
    Progress progress(stats->input_strings_count, top_progress, 1);
    progress.message(_(L"Pre-translating…"));

    int last_matched = 0;
    bool more_work = true;
    while (more_work)
    {
        try
        {
            std::this_thread::sleep_for(10ms);

            // pump the workers:
            more_work = false;
            if (worker_local)
            {
                if (worker_local->pump(cancellation_token))
                {
                    more_work = true;
                }
                else
                {
                    worker_local.reset();
                }
            }

            // update progress bar:
            if (last_matched != stats->matched)
            {
                last_matched = stats->matched;
                progress.message(wxString::Format(wxPLURAL("Pre-translated %u string", "Pre-translated %u strings", last_matched), last_matched));
            }
            progress.set(stats->input_strings_processed);
        }
        catch (...)
        {
            stats->errors++;
            wxLogError("%s", DescribeCurrentException());
            break;
        }
    }

    wxLogTrace("poedit", "Pre-translation completed in %ld ms", sw.Time());
    return stats;
}

} // namespace pretranslate
