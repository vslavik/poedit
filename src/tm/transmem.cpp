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

#include "transmem.h"

#include "catalog.h"

#include <wx/stdpaths.h>
#include <wx/utils.h>
#include <wx/dir.h>

#include <mutex>

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/name_generator.hpp>
#include <boost/uuid/string_generator.hpp>

#include <Lucene.h>
#include <LuceneException.h>
#include <MMapDirectory.h>
#include <StandardAnalyzer.h>
#include <IndexWriter.h>
#include <IndexSearcher.h>
#include <IndexReader.h>
#include <Document.h>
#include <Field.h>
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

// ----------------------------------------------------------------
// TranslationMemoryImpl
// ----------------------------------------------------------------

class TranslationMemoryImpl
{
public:
    static const int DEFAULT_MAXHITS = 10;

    typedef TranslationMemory::Results Results;

    TranslationMemoryImpl()
        : m_dir(newLucene<MMapDirectory>(GetDatabaseDir())),
          m_analyzer(newLucene<StandardAnalyzer>(LuceneVersion::LUCENE_CURRENT))
    {}

    bool Search(const std::wstring& lang,
                const std::wstring& source,
                Results& results,
                int maxHits = -1);

    std::shared_ptr<TranslationMemory::Writer> CreateWriter();

    void GetStats(long& numDocs, long& fileSize);

private:
    static std::wstring GetDatabaseDir();
    IndexReaderPtr Reader();
    SearcherPtr Searcher();

private:
    DirectoryPtr     m_dir;
    AnalyzerPtr      m_analyzer;
    IndexReaderPtr   m_reader;
    std::mutex       m_readerMutex;
};


std::wstring TranslationMemoryImpl::GetDatabaseDir()
{
    wxString data;
#if defined(__UNIX__) && !defined(__WXMAC__)
    if ( !wxGetEnv("XDG_DATA_HOME", &data) )
        data = wxGetHomeDir() + "/.local/share";
    data += "/poedit";
#else
    data = wxStandardPaths::Get().GetUserDataDir();
#endif
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
        m_reader = IndexReader::open(m_dir, true);
    return m_reader;
}


namespace
{

void PerformSearch(IndexSearcherPtr searcher,
                   const std::wstring& lang,
                   QueryPtr query,
                   TranslationMemory::Results& results, int maxHits)
{
    // TODO: use short form of the language too (cs vs cs_CZ), boost the
    //       full form in the query
    auto langQ = newLucene<TermQuery>(newLucene<Term>(L"lang", lang));
    auto fullQuery = newLucene<BooleanQuery>();
    fullQuery->add(langQ, BooleanClause::MUST);
    fullQuery->add(query, BooleanClause::MUST);

    auto hits = searcher->search(fullQuery, maxHits);
    for (int i = 0; i < hits->scoreDocs.size(); i++)
    {
        DocumentPtr doc = searcher->doc(hits->scoreDocs[i]->doc);
        auto t = doc->get(L"trans");
        if (std::find(results.begin(), results.end(), t) == results.end())
            results.push_back(t);
    }
}

} // anonymous namespace

bool TranslationMemoryImpl::Search(const std::wstring& lang,
                                   const std::wstring& source,
                                   Results& results,
                                   int maxHits)
{
    try
    {
        if (maxHits <= 0)
            maxHits = DEFAULT_MAXHITS;

        results.clear();

        Lucene::String fieldName(L"source");
        auto boolQ = newLucene<BooleanQuery>();
        auto phraseQ = newLucene<PhraseQuery>();

        auto stream = m_analyzer->tokenStream(fieldName, newLucene<StringReader>(source));
        while (stream->incrementToken())
        {
            auto word = stream->getAttribute<TermAttribute>()->term();
            auto term = newLucene<Term>(fieldName, word);
            boolQ->add(newLucene<TermQuery>(term), BooleanClause::SHOULD);
            phraseQ->add(term);
        }

        auto searcher = newLucene<IndexSearcher>(Reader());

        PerformSearch(searcher, lang, phraseQ, results, maxHits);
        maxHits -= results.size();
        if (maxHits > 0)
            PerformSearch(searcher, lang, boolQ, results, maxHits);

        return !results.empty();
    }
    catch (LuceneException&)
    {
        return false;
    }
}


void TranslationMemoryImpl::GetStats(long& numDocs, long& fileSize)
{
    auto reader = Reader();
    numDocs = reader->numDocs();
    fileSize = wxDir::GetTotalSize(GetDatabaseDir()).GetValue();
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
    }

    ~TranslationMemoryWriterImpl()
    {
        if (m_writer)
            m_writer->rollback();
    }

    virtual void Commit()
    {
        m_writer->close();
        m_writer.reset();
    }

    virtual void Insert(const std::wstring& lang,
                        const std::wstring& source,
                        const std::wstring& trans)
    {
        if (lang.empty())
            return;

        // Compute unique ID for the translation:

        static const boost::uuids::uuid s_namespace =
          boost::uuids::string_generator()("6e3f73c5-333f-4171-9d43-954c372a8a02");
        boost::uuids::name_generator gen(s_namespace);

        std::wstring itemId(L"en"); // TODO: srclang
        itemId += lang;
        itemId += source;
        itemId += trans;

        const std::wstring itemUUID = boost::uuids::to_wstring(gen(itemId));

        // Then add a new document:
        auto doc = newLucene<Document>();

        doc->add(newLucene<Field>(L"uuid", itemUUID,
                                  Field::STORE_YES, Field::INDEX_NOT_ANALYZED));
        doc->add(newLucene<Field>(L"srclang", L"en",
                                  Field::STORE_YES, Field::INDEX_NOT_ANALYZED));
        doc->add(newLucene<Field>(L"lang", lang,
                                  Field::STORE_YES, Field::INDEX_NOT_ANALYZED));
        doc->add(newLucene<Field>(L"source", source,
                                  Field::STORE_YES, Field::INDEX_ANALYZED));
        doc->add(newLucene<Field>(L"trans", trans,
                                  Field::STORE_YES, Field::INDEX_NOT_ANALYZED));

        m_writer->updateDocument(newLucene<Term>(L"uuid", itemUUID), doc);
    }

    virtual void Insert(const Catalog &cat)
    {
        const std::wstring lang = cat.GetLocaleCode().ToStdWstring();
        if (lang.empty())
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

            Insert(lang, item.GetString(), item.GetTranslation());
        }
    }

private:
    IndexWriterPtr m_writer;
};

std::shared_ptr<TranslationMemory::Writer> TranslationMemoryImpl::CreateWriter()
{
    return std::make_shared<TranslationMemoryWriterImpl>(m_dir, m_analyzer);
}



// ----------------------------------------------------------------
// Singleton management
// ----------------------------------------------------------------

TranslationMemory *TranslationMemory::ms_instance = nullptr;

TranslationMemory& TranslationMemory::Get()
{
    if (!ms_instance)
    {
        ms_instance = new TranslationMemory;
    }
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

bool TranslationMemory::Search(const std::wstring& lang,
                               const std::wstring& source,
                               Results& results,
                               int maxHits)
{
    return m_impl->Search(lang, source, results, maxHits);
}

std::shared_ptr<TranslationMemory::Writer> TranslationMemory::CreateWriter()
{
    return m_impl->CreateWriter();
}

void TranslationMemory::GetStats(long& numDocs, long& fileSize)
{
    m_impl->GetStats(numDocs, fileSize);
}