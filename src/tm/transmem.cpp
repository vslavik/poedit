/*
 *  This file is part of Poedit (http://poedit.net)
 *
 *  Copyright (C) 2013-2015 Vaclav Slavik
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

using namespace Lucene;

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

    TranslationMemoryImpl()
        : m_dir(GetDatabaseDir()),
          m_analyzer(newLucene<StandardAnalyzer>(LuceneVersion::LUCENE_CURRENT))
    {}

    SuggestionsList Search(const Language& lang,
                           const std::wstring& source);

    std::shared_ptr<TranslationMemory::Writer> CreateWriter();

    void GetStats(long& numDocs, long& fileSize);

private:
    static std::wstring GetDatabaseDir();
    IndexReaderPtr Reader();
    SearcherPtr Searcher();

private:
    std::wstring     m_dir;
    AnalyzerPtr      m_analyzer;
    IndexReaderPtr   m_reader;
    std::mutex       m_readerMutex;
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


IndexReaderPtr TranslationMemoryImpl::Reader()
{
    std::lock_guard<std::mutex> guard(m_readerMutex);
    if ( m_reader )
        m_reader = m_reader->reopen();
    else
        m_reader = IndexReader::open(newLucene<DirectoryType>(m_dir), true);
    return m_reader;
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

template<typename T>
void PerformSearchWithBlock(IndexSearcherPtr searcher,
                            QueryPtr lang,
                            const std::wstring& exactSourceText,
                            QueryPtr query,
                            double scoreThreshold,
                            double scoreScaling,
                            T callback)
{
    auto fullQuery = newLucene<BooleanQuery>();
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
        auto src = doc->get(L"source");
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
                   QueryPtr lang,
                   const std::wstring& exactSourceText,
                   QueryPtr query,
                   SuggestionsList& results,
                   double scoreThreshold,
                   double scoreScaling)
{
    PerformSearchWithBlock
    (
        searcher, lang, exactSourceText, query,
        scoreThreshold, scoreScaling,
        [&results](DocumentPtr doc, double score)
        {
            auto t = doc->get(L"trans");
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

SuggestionsList TranslationMemoryImpl::Search(const Language& lang,
                                              const std::wstring& source)
{
    try
    {
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
        while (stream->incrementToken())
        {
            sourceTokensCount++;
            auto word = stream->getAttribute<TermAttribute>()->term();
            auto term = newLucene<Term>(sourceField, word);
            boolQ->add(newLucene<TermQuery>(term), BooleanClause::SHOULD);
            phraseQ->add(term);
        }

        auto searcher = newLucene<IndexSearcher>(Reader());

        // Try exact phrase first:
        PerformSearch(searcher, langQ, source, phraseQ, results,
                      QUALITY_THRESHOLD, /*scoreScaling=*/1.0);
        if (!results.empty())
            return results;

        // Then, if no matches were found, permit being a bit sloppy:
        phraseQ->setSlop(1);
        PerformSearch(searcher, langQ, source, phraseQ, results,
                      QUALITY_THRESHOLD, /*scoreScaling=*/0.9);

        if (!results.empty())
            return results;

        // As the last resort, try terms search. This will almost certainly
        // produce low-quality results, but hopefully better than nothing.
        boolQ->setMinimumNumberShouldMatch(std::max(1, boolQ->getClauses().size() - MAX_ALLOWED_LENGTH_DIFFERENCE));
        PerformSearchWithBlock
        (
            searcher, langQ, source, boolQ,
            QUALITY_THRESHOLD, /*scoreScaling=*/0.8,
            [=,&results](DocumentPtr doc, double score)
            {
                auto s = doc->get(sourceField);
                auto t = doc->get(L"trans");
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
        auto reader = Reader();
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
    TranslationMemoryWriterImpl(DirectoryPtr dir, AnalyzerPtr analyzer)
    {
        m_writer = newLucene<IndexWriter>(dir, analyzer, IndexWriter::MaxFieldLengthLIMITED);
        m_writer->setMergeScheduler(newLucene<SerialMergeScheduler>());
    }

    ~TranslationMemoryWriterImpl()
    {
        if (m_writer)
            m_writer->rollback();
    }

    virtual void Commit()
    {
        try
        {
            m_writer->close();
            m_writer.reset();
        }
        CATCH_AND_RETHROW_EXCEPTION
    }

    virtual void Insert(const Language& lang,
                        const std::wstring& source,
                        const std::wstring& trans)
    {
        if (!lang.IsValid())
            return;

        // Compute unique ID for the translation:

        static const boost::uuids::uuid s_namespace =
          boost::uuids::string_generator()("6e3f73c5-333f-4171-9d43-954c372a8a02");
        boost::uuids::name_generator gen(s_namespace);

        std::wstring itemId(L"en"); // TODO: srclang
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
            doc->add(newLucene<Field>(L"created", DateField::timeToString(time(NULL)),
                                      Field::STORE_YES, Field::INDEX_NO));
            doc->add(newLucene<Field>(L"srclang", L"en",
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

    virtual void Insert(const Catalog &cat)
    {
        auto lang = cat.GetLanguage();
        if (!lang.IsValid())
            return;

        int cnt = cat.GetCount();
        for (int i = 0; i < cnt; i++)
        {
            const CatalogItem& item = cat[i];

            // ignore translations with errors in them
            if (item.GetValidity() == CatalogItem::Val_Invalid)
                continue;

            // can't handle plurals yet (TODO?)
            if (item.HasPlural())
                continue;

            // ignore untranslated or unfinished translations
            if (item.IsFuzzy() || !item.IsTranslated())
                continue;

            // Note that dt.IsModified() is intentionally not checked - we
            // want to save old entries in the TM too, so that we harvest as
            // much useful translations as we can.

            Insert(lang, item.GetString().ToStdWstring(), item.GetTranslation().ToStdWstring());
        }
    }

    virtual void DeleteAll()
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

std::shared_ptr<TranslationMemory::Writer> TranslationMemoryImpl::CreateWriter()
{
    try
    {
        return std::make_shared<TranslationMemoryWriterImpl>(newLucene<DirectoryType>(m_dir), m_analyzer);
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
        try
        {
            ms_instance = new TranslationMemory;
        }
        CATCH_AND_RETHROW_EXCEPTION
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

TranslationMemory::TranslationMemory() : m_impl(new TranslationMemoryImpl) {}
TranslationMemory::~TranslationMemory() { delete m_impl; }


// ----------------------------------------------------------------
// public API
// ----------------------------------------------------------------

SuggestionsList TranslationMemory::Search(const Language& lang,
                                          const std::wstring& source)
{
    return m_impl->Search(lang, source);
}

void TranslationMemory::SuggestTranslation(const Language& lang,
                                           const std::wstring& source,
                                           success_func_type onSuccess,
                                           error_func_type onError)
{
    try
    {
        onSuccess(Search(lang, source));
    }
    catch (...)
    {
        onError(std::current_exception());
    }
}

std::shared_ptr<TranslationMemory::Writer> TranslationMemory::CreateWriter()
{
    return m_impl->CreateWriter();
}

void TranslationMemory::GetStats(long& numDocs, long& fileSize)
{
    m_impl->GetStats(numDocs, fileSize);
}
