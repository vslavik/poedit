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

#ifndef _TRANSMEM_H_
#define _TRANSMEM_H_

#include <exception>
#include <string>
#include <vector>
#include <memory>

#include "catalog.h"
#include "suggestions.h"

class TranslationMemoryImpl;

/** 
    Lucene-based translation memory.
    
    All methods may throw Exception.
 */
class TranslationMemory : public SuggestionsBackend
{
public:
    /// Return singleton instance of the TM.
    static TranslationMemory& Get();

    /// Destroys the singleton, must be called (omly) on app shutdown.
    static void CleanUp();

    /**
        Search translation memory for similar strings.
        
        @param srclang Language of the source text.
        @param lang    Language of the desired translation.
        @param source  Source text.

        @return List of hits that were found, possibly empty.
     */
    SuggestionsList Search(const Language& srclang,
                           const Language& lang,
                           const std::wstring& source);

    /// SuggestionsBackend API implementation:
    dispatch::future<SuggestionsList> SuggestTranslation(const Language& srclang,
                                                         const Language& lang,
                                                         const std::wstring& source) override;

    /**
        Performs updates to the translation memory.
        
        Call Commit() to commit changes since the last commit to disk.
        Call Rollback() to undo all changes since the last commit.
        
        Committing shouldn't be done too often, as it is expensive.
        The writer is shared and can be used by multiple threads.
        
        Note that closing the writer on shutdown, if it has uncommitted
        changes, will result in them being committed. You must to explicitly
        Rollback() them if you don't want that behavior.

        All methods may throw Exception.
      */
    class Writer
    {
    public:
        virtual ~Writer() {}

        /**
            Insert translation into the TM.

            @param srclang Source text language.
            @param lang    Translation language.
            @param source  Source text.
            @param trans   Translation text.
         */
        virtual void Insert(const Language& srclang,
                            const Language& lang,
                            const std::wstring& source,
                            const std::wstring& trans) = 0;

        /**
            Inserts a single catalog item.

            @note
            Not everything is included: fuzzy or untranslated entries are skipped.
         */
        virtual void Insert(const Language& srclang,
                            const Language& lang,
                            const CatalogItemPtr& item) = 0;

        /**
            Inserts entire content of the catalog.
            
            @note
            Not everything is included: fuzzy or untranslated entries are omitted.
            If the catalog doesn't have language header, it is not included either.
         */
        virtual void Insert(const CatalogPtr& cat) = 0;

        /// Deletes everything from the TM.
        virtual void DeleteAll() = 0;

        /// Commits changes written so far.
        virtual void Commit() = 0;

        /// Rolls back changes written so far.
        virtual void Rollback() = 0;
    };

    /// Returns the shared writer instance
    std::shared_ptr<Writer> GetWriter();

    /// Returns statistics about the TM
    void GetStats(long& numDocs, long& fileSize);

private:
    TranslationMemory();
    ~TranslationMemory();

    TranslationMemoryImpl *m_impl;
    std::exception_ptr m_error;
    static TranslationMemory *ms_instance;
};


#endif // _TRANSMEM_H_
