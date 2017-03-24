/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 2013-2017 Vaclav Slavik
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

#include "transmem.h"

#include "catalog.h"
#include "errors.h"
#include "utility.h"

#include <wx/stdpaths.h>
#include <wx/utils.h>
#include <wx/dir.h>
#include <wx/filename.h>

#include <time.h>
#include <mutex>

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/name_generator.hpp>
#include <boost/uuid/string_generator.hpp>

#include <Lucene.h>
#include <LuceneException.h>
#include <MMapDirectory.h>
#include <SerialMergeScheduler.h>
#include <SimpleFSDirectory.h>
#include <StandardAnalyzer.h>
#include <IndexWriter.h>
#include <IndexSearcher.h>
#include <IndexReader.h>
#include <Document.h>
#include <Field.h>
#include <DateField.h>
#include <PrefixQuery.h>
#include <StringUtils.h>
#include <TermQuery.h>
#include <BooleanQuery.h>
#include <PhraseQuery.h>
#include <Term.h>
#include <ScoreDoc.h>
#include <TopDocs.h>
#include <StringReader.h>
#include <TokenStream.h>
#include <TermAttribute.h>
#include <PositionIncrementAttribute.h>

using namespace Lucene;

namespace
{

#define CATCH_AND_RETHROW_EXCEPTION                                 \
    catch (LuceneException& e)                                      \
    {                                                               \
        throw Exception(wxString::Format("%s (%d)",                 \
                        e.getError(), (int)e.getType()));           \
    }                                                               \
    catch (std::exception& e)                                       \
    {                                                               \
        throw Exception(e.what());                                  \
    }


// Manages IndexReader and Searcher instances in multi-threaded environment.
// Curiously, Lucene uses shared_ptr-based refcounting *and* explicit one as
// well, with a crucial part not well protected.
//
// See https://issues.apache.org/jira/browse/LUCENE-3567 for the exact issue
// encountered by Poedit's use as well. For an explanation of the manager
// class, see
// http://blog.mikemccandless.com/2011/09/lucenes-searchermanager-simplifies.html
// http://blog.mikemccandless.com/2011/11/near-real-time-readers-with-lucenes.html
class SearcherManager
{
public:
    SearcherManager(IndexWriterPtr writer)
    {
        m_reader = writer->getReader();
        m_searcher = newLucene<IndexSearcher>(m_reader);
    }

    ~SearcherManager()
    {
        m_searcher.reset();
        m_reader->decRef();
    }

    // Safe, properly ref-counting (in Lucene way, not just shared_ptr) holder.
    template<typename T>
    class SafeRef
    {
    public:
        typedef boost::shared_ptr<T> TPtr;

        SafeRef(SafeRef&& other) : m_mng(other.m_mng) { std::swap(m_ptr, other.m_ptr); }
        ~SafeRef() { if (m_ptr) m_mng.DecRef(m_ptr); }

        TPtr ptr() { return m_ptr; }
        T* operator->() const { return m_ptr.get(); }

        SafeRef(const SafeRef&) = delete;
        SafeRef& operator=(const SafeRef&) = delete;

    private:
        friend class SearcherManager;
        explicit SafeRef(SearcherManager& m, TPtr ptr) : m_mng(m), m_ptr(ptr) {}

        SearcherManager& m_mng;
        boost::shared_ptr<T> m_ptr;
    };

    SafeRef<IndexReader> Reader()
    {
        std::lock_guard<std::mutex> guard(m_mutex);
        ReloadReaderIfNeeded();
        m_reader->incRef();
        return SafeRef<IndexReader>(*this, m_reader);
    }

    SafeRef<IndexSearcher> Searcher()
    {
        std::lock_guard<std::mutex> guard(m_mutex);
        ReloadReaderIfNeeded();
        m_searcher->getIndexReader()->incRef();
        return SafeRef<IndexSearcher>(*this, m_searcher);
    }

private:
    void ReloadReaderIfNeeded()
    {
        // contract: m_mutex is locked when this function is called
        if (m_reader->isCurrent())
            return; // nothing to do

        auto newReader = m_reader->reopen();
        auto newSearcher = newLucene<IndexSearcher>(newReader);

        m_reader->decRef();

        m_reader = newReader;
        m_searcher = newSearcher;
    }

    void DecRef(IndexReaderPtr& r)
    {
        std::lock_guard<std::mutex> guard(m_mutex);
        r->decRef();
    }
    void DecRef(IndexSearcherPtr& s)
    {
        std::lock_guard<std::mutex> guard(m_mutex);
        s->getIndexReader()->decRef();
    }

    IndexReaderPtr   m_reader;
    IndexSearcherPtr m_searcher;
    std::mutex       m_mutex;
};


} // anonymous namespace

// ----------------------------------------------------------------
// TranslationMemoryImpl
// ----------------------------------------------------------------

class TranslationMemoryImpl
{
public:
#ifdef __WXMSW__
    typedef SimpleFSDirectory DirectoryType;
#else
    typedef MMapDirectory DirectoryType;
#endif

    TranslationMemoryImpl() { Init(); }

    ~TranslationMemoryImpl()
    {
        m_mng.reset();
        m_writer->close();
    }

    SuggestionsList Search(const Language& srclang, const Language& lang,
                           const std::wstring& source);

    std::shared_ptr<TranslationMemory::Writer> GetWriter() { return m_writerAPI; }

    void GetStats(long& numDocs, long& fileSize);

private:
    void Init();

    static std::wstring GetDatabaseDir();

private:
    AnalyzerPtr      m_analyzer;
    IndexWriterPtr   m_writer;
    std::shared_ptr<SearcherManager> m_mng;

    std::shared_ptr<TranslationMemory::Writer> m_writerAPI;
};


std::wstring TranslationMemoryImpl::GetDatabaseDir()
{
    wxString data;
#if defined(__UNIX__) && !defined(__WXOSX__)
    if ( !wxGetEnv("XDG_DATA_HOME", &data) )
        data = wxGetHomeDir() + "/.local/share";
    data += "/poedit";
#else
    data = wxStandardPaths::Get().GetUserDataDir();
#endif

    // ensure the parent directory exists:
    wxFileName::Mkdir(data, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);

    data += wxFILE_SEP_PATH;
    data += "TranslationMemory";

    return data.ToStdWstring();
}


namespace
{

static const int DEFAULT_MAXHITS = 10;

// Normalized score that must be met for a suggestion to be shown. This is
// an empirical guess of what constitues good matches.
static const double QUALITY_THRESHOLD = 0.6;

// Maximum allowed difference in phrase length, in #terms.
static const int MAX_ALLOWED_LENGTH_DIFFERENCE = 2;


bool ContainsResult(const SuggestionsList& all, const std::wstring& r)
{
    auto found = std::find_if(all.begin(), all.end(),
                              [&r](const Suggestion& x){ return x.text == r; });
    return found != all.end();
}

// Return translation (or source) text field.
//
// Older versions of Poedit used to store C-like escaped text (e.g. "\n" instead
// of newline), but starting with 1.8, the "true" form of the text is stored.
// To preserve compatibility with older data, a version field is stored with
// TM documents and this function decides whether to decode escapes or not.
//
// TODO: remove this a few years down the road.
std::wstring get_text_field(DocumentPtr doc, const std::wstring& field)
{
    auto version = doc->get(L"v");
    auto value = doc->get(field);
    if (version.empty()) // pre-1.8 data
        return UnescapeCString(value);
    else
        return value;
}


template<typename T>
void PerformSearchWithBlock(IndexSearcherPtr searcher,
                            QueryPtr srclang, QueryPtr lang,
                            const std::wstring& exactSourceText,
                            QueryPtr query,
                            double scoreThreshold,
                            double scoreScaling,
                            T callback)
{
    auto fullQuery = newLucene<BooleanQuery>();
    fullQuery->add(srclang, BooleanClause::MUST);
    fullQuery->add(lang, BooleanClause::MUST);
    fullQuery->add(query, BooleanClause::MUST);

    auto hits = searcher->search(fullQuery, DEFAULT_MAXHITS);

    for (int i = 0; i < hits->scoreDocs.size(); i++)
    {
        const auto& scoreDoc = hits->scoreDocs[i];
        auto score = scoreDoc->score / hits->maxScore;
        if (score < scoreThreshold)
            continue;

        auto doc = searcher->doc(scoreDoc->doc);
        auto src = get_text_field(doc, L"source");
        if (src == exactSourceText)
        {
            score = 1.0;
        }
        else
        {
            if (score == 1.0)
            {
                score = 0.95; // can't score non-exact thing as 100%:

                // Check against too small queries having perfect hit in a large stored text.
                // Do this by penalizing too large difference in lengths of the source strings.
                double len1 = exactSourceText.size();
                double len2 = src.size();
                score *= 1.0 - 0.4 * (std::abs(len1 - len2) / std::max(len1, len2));
            }

            score *= scoreScaling;
        }

        callback(doc, score);
    }
}

void PerformSearch(IndexSearcherPtr searcher,
                   QueryPtr srclang, QueryPtr lang,
                   const std::wstring& exactSourceText,
                   QueryPtr query,
                   SuggestionsList& results,
                   double scoreThreshold,
                   double scoreScaling)
{
    PerformSearchWithBlock
    (
        searcher, srclang, lang, exactSourceText, query,
        scoreThreshold, scoreScaling,
        [&results](DocumentPtr doc, double score)
        {
            auto t = get_text_field(doc, L"trans");
            if (!ContainsResult(results, t))
            {
                time_t ts = DateField::stringToTime(doc->get(L"created"));
                Suggestion r {t, score, ts};
                results.push_back(r);
            }
        }
    );

    std::stable_sort(results.begin(), results.end());
}

} // anonymous namespace

SuggestionsList TranslationMemoryImpl::Search(const Language& srclang,
                                              const Language& lang,
                                              const std::wstring& source)
{
    try
    {
        // TODO: query by srclang too!
        auto srclangQ = newLucene<TermQuery>(newLucene<Term>(L"srclang", srclang.WCode()));

        const Lucene::String fullLang = lang.WCode();
        const Lucene::String shortLang = StringUtils::toUnicode(lang.Lang());

        QueryPtr langPrimary = newLucene<TermQuery>(newLucene<Term>(L"lang", fullLang));
        QueryPtr langSecondary;
        if (fullLang == shortLang)
        {
            // for e.g. 'cs', search also 'cs_*' (e.g. 'cs_CZ')
            langSecondary = newLucene<PrefixQuery>(newLucene<Term>(L"lang", shortLang + L"_"));
        }
        else
        {
            // search short variants of the language too
            langSecondary = newLucene<TermQuery>(newLucene<Term>(L"lang", shortLang));
        }
        langSecondary->setBoost(0.85);
        auto langQ = newLucene<BooleanQuery>();
        langQ->add(langPrimary, BooleanClause::SHOULD);
        langQ->add(langSecondary, BooleanClause::SHOULD);

        SuggestionsList results;

        const Lucene::String sourceField(L"source");
        auto boolQ = newLucene<BooleanQuery>();
        auto phraseQ = newLucene<PhraseQuery>();

        auto stream = m_analyzer->tokenStream(sourceField, newLucene<StringReader>(source));
        int sourceTokensCount = 0;
        int sourceTokenPosition = -1;
        while (stream->incrementToken())
        {
            sourceTokensCount++;
            auto word = stream->getAttribute<TermAttribute>()->term();
            sourceTokenPosition += stream->getAttribute<PositionIncrementAttribute>()->getPositionIncrement();
            auto term = newLucene<Term>(sourceField, word);
            boolQ->add(newLucene<TermQuery>(term), BooleanClause::SHOULD);
            phraseQ->add(term, sourceTokenPosition);
        }

        auto searcher = m_mng->Searcher();

        // Try exact phrase first:
        PerformSearch(searcher.ptr(), srclangQ, langQ, source, phraseQ, results,
                      QUALITY_THRESHOLD, /*scoreScaling=*/1.0);
        if (!results.empty())
            return results;

        // Then, if no matches were found, permit being a bit sloppy:
        phraseQ->setSlop(1);
        PerformSearch(searcher.ptr(), srclangQ, langQ, source, phraseQ, results,
                      QUALITY_THRESHOLD, /*scoreScaling=*/0.9);

        if (!results.empty())
            return results;

        // As the last resort, try terms search. This will almost certainly
        // produce low-quality results, but hopefully better than nothing.
        boolQ->setMinimumNumberShouldMatch(std::max(1, boolQ->getClauses().size() - MAX_ALLOWED_LENGTH_DIFFERENCE));
        PerformSearchWithBlock
        (
            searcher.ptr(), srclangQ, langQ, source, boolQ,
            QUALITY_THRESHOLD, /*scoreScaling=*/0.8,
            [=,&results](DocumentPtr doc, double score)
            {
                auto s = get_text_field(doc, sourceField);
                auto t = get_text_field(doc, L"trans");
                auto stream2 = m_analyzer->tokenStream(sourceField, newLucene<StringReader>(s));
                int tokensCount2 = 0;
                while (stream2->incrementToken())
                    tokensCount2++;

                if (std::abs(tokensCount2 - sourceTokensCount) <= MAX_ALLOWED_LENGTH_DIFFERENCE &&
                    !ContainsResult(results, t))
                {
                    time_t ts = DateField::stringToTime(doc->get(L"created"));
                    Suggestion r {t, score, ts};
                    results.push_back(r);
                }
            }
        );

        std::stable_sort(results.begin(), results.end());
        return results;
    }
    catch (LuceneException&)
    {
        return SuggestionsList();
    }
}


void TranslationMemoryImpl::GetStats(long& numDocs, long& fileSize)
{
    try
    {
        auto reader = m_mng->Reader();
        numDocs = reader->numDocs();
        fileSize = wxDir::GetTotalSize(GetDatabaseDir()).GetValue();
    }
    CATCH_AND_RETHROW_EXCEPTION
}

// ----------------------------------------------------------------
// TranslationMemoryWriterImpl
// ----------------------------------------------------------------

class TranslationMemoryWriterImpl : public TranslationMemory::Writer
{
public:
    TranslationMemoryWriterImpl(IndexWriterPtr writer) : m_writer(writer) {}

    ~TranslationMemoryWriterImpl() {}

    void Commit() override
    {
        try
        {
            m_writer->commit();
        }
        CATCH_AND_RETHROW_EXCEPTION
    }

    void Rollback() override
    {
        try
        {
            m_writer->rollback();
        }
        CATCH_AND_RETHROW_EXCEPTION
    }

    void Insert(const Language& srclang, const Language& lang,
                const std::wstring& source, const std::wstring& trans) override
    {
        if (!lang.IsValid() || !srclang.IsValid() || lang == srclang)
            return;

        // Compute unique ID for the translation:

        static const boost::uuids::uuid s_namespace =
          boost::uuids::string_generator()("6e3f73c5-333f-4171-9d43-954c372a8a02");
        boost::uuids::name_generator gen(s_namespace);

        std::wstring itemId(srclang.WCode());
        itemId += lang.WCode();
        itemId += source;
        itemId += trans;

        const std::wstring itemUUID = boost::uuids::to_wstring(gen(itemId));

        try
        {
            // Then add a new document:
            auto doc = newLucene<Document>();

            doc->add(newLucene<Field>(L"uuid", itemUUID,
                                      Field::STORE_YES, Field::INDEX_NOT_ANALYZED));
            doc->add(newLucene<Field>(L"v", L"1",
                                      Field::STORE_YES, Field::INDEX_NO));
            doc->add(newLucene<Field>(L"created", DateField::timeToString(time(NULL)),
                                      Field::STORE_YES, Field::INDEX_NO));
            doc->add(newLucene<Field>(L"srclang", srclang.WCode(),
                                      Field::STORE_YES, Field::INDEX_NOT_ANALYZED));
            doc->add(newLucene<Field>(L"lang", lang.WCode(),
                                      Field::STORE_YES, Field::INDEX_NOT_ANALYZED));
            doc->add(newLucene<Field>(L"source", source,
                                      Field::STORE_YES, Field::INDEX_ANALYZED));
            doc->add(newLucene<Field>(L"trans", trans,
                                      Field::STORE_YES, Field::INDEX_NOT_ANALYZED));

            m_writer->updateDocument(newLucene<Term>(L"uuid", itemUUID), doc);
        }
        CATCH_AND_RETHROW_EXCEPTION
    }

    void Insert(const Language& srclang, const Language& lang, const CatalogItemPtr& item) override
    {
        if (!lang.IsValid() || !srclang.IsValid())
            return;

        // ignore translations with errors in them
        if (item->HasError())
            return;

        // can't handle plurals yet (TODO?)
        if (item->HasPlural())
            return;

        // ignore untranslated or unfinished translations
        if (item->IsFuzzy() || !item->IsTranslated())
            return;

        Insert(srclang, lang, item->GetString().ToStdWstring(), item->GetTranslation().ToStdWstring());
    }

    void Insert(const CatalogPtr& cat) override
    {
        auto srclang = cat->GetSourceLanguage();
        auto lang = cat->GetLanguage();
        if (!lang.IsValid() || !srclang.IsValid())
            return;

        for (auto& item: cat->items())
        {
            // Note that dt.IsModified() is intentionally not checked - we
            // want to save old entries in the TM too, so that we harvest as
            // much useful translations as we can.
            Insert(srclang, lang, item);
        }
    }

    void DeleteAll() override
    {
        try
        {
            m_writer->deleteAll();
        }
        CATCH_AND_RETHROW_EXCEPTION
    }

private:
    IndexWriterPtr m_writer;
};


void TranslationMemoryImpl::Init()
{
    try
    {
        auto dir = newLucene<DirectoryType>(GetDatabaseDir());
        m_analyzer = newLucene<StandardAnalyzer>(LuceneVersion::LUCENE_CURRENT);

        m_writer = newLucene<IndexWriter>(dir, m_analyzer, IndexWriter::MaxFieldLengthLIMITED);
        m_writer->setMergeScheduler(newLucene<SerialMergeScheduler>());

        // get the associated realtime reader & searcher:
        m_mng.reset(new SearcherManager(m_writer));

        m_writerAPI = std::make_shared<TranslationMemoryWriterImpl>(m_writer);
    }
    CATCH_AND_RETHROW_EXCEPTION
}



// ----------------------------------------------------------------
// Singleton management
// ----------------------------------------------------------------

static std::once_flag initializationFlag;
TranslationMemory *TranslationMemory::ms_instance = nullptr;

TranslationMemory& TranslationMemory::Get()
{
    std::call_once(initializationFlag, []() {
        ms_instance = new TranslationMemory;
    });
    return *ms_instance;
}

void TranslationMemory::CleanUp()
{
    if (ms_instance)
    {
        delete ms_instance;
        ms_instance = nullptr;
    }
}

TranslationMemory::TranslationMemory() : m_impl(nullptr)
{
    try
    {
        m_impl = new TranslationMemoryImpl;
    }
    catch (...)
    {
        m_error = std::current_exception();
    }
}

TranslationMemory::~TranslationMemory() { delete m_impl; }


// ----------------------------------------------------------------
// public API
// ----------------------------------------------------------------

SuggestionsList TranslationMemory::Search(const Language& srclang,
                                          const Language& lang,
                                          const std::wstring& source)
{
    if (!m_impl)
        std::rethrow_exception(m_error);
    return m_impl->Search(srclang, lang, source);
}

dispatch::future<SuggestionsList> TranslationMemory::SuggestTranslation(const Language& srclang,
                                                                        const Language& lang,
                                                                        const std::wstring& source)
{
    try
    {
        return dispatch::make_ready_future(Search(srclang, lang, source));
    }
    catch (...)
    {
        return dispatch::make_exceptional_future_from_current<SuggestionsList>();
    }
}

std::shared_ptr<TranslationMemory::Writer> TranslationMemory::GetWriter()
{
    if (!m_impl)
        std::rethrow_exception(m_error);
    return m_impl->GetWriter();
}

void TranslationMemory::GetStats(long& numDocs, long& fileSize)
{
    if (!m_impl)
        std::rethrow_exception(m_error);
    m_impl->GetStats(numDocs, fileSize);
}
