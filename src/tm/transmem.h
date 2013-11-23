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

#ifndef _TRANSMEM_H_
#define _TRANSMEM_H_

#include <string>
#include <vector>
#include <memory>

class Catalog;
class TranslationMemoryImpl;

/** 
    Lucene-based translation memory.
    
    All methods may throw Exception.
 */
class TranslationMemory
{
public:
    /// Return singleton instance of the TM.
    static TranslationMemory& Get();

    /// Destroys the singleton, should be called on app shutdown.
    static void CleanUp();

    typedef std::vector<std::wstring> Results;

    /**
        Search translation memory for similar strings.
        
        @param lang    Language of the desired translation.
        @param source  Source text.
        @param results Array to store any results into.
        @param maxHits Maximum number of requested hits; -1 leaves the
                       decision to the function.
                       
        @return true if any hits were found, false otherwise.
     */
    bool Search(const std::wstring& lang,
                const std::wstring& source,
                Results& results,
                int maxHits = -1);

    /**
        Performs updates to the translation memory.
        
        You must call Commit() for them to be written.
    
        All methods may throw Exception.
      */
    class Writer
    {
    public:
        virtual ~Writer() {}

        /**
            Insert translation into the TM.
            
            @param lang    Language code (e.g. pt_BR or cs).
            @param source  Source text.
            @param trans   Translation text.
         */
        virtual void Insert(const std::wstring& lang,
                            const std::wstring& source,
                            const std::wstring& trans) = 0;

        /**
            Inserts entire content of the catalog.
            
            @note
            Not everything is included: fuzzy or untranslated entries are omitted.
            If the catalog doesn't have language header, it is not included either.
         */
        virtual void Insert(const Catalog& cat) = 0;

        /// Commits changes written so far.
        virtual void Commit() = 0;
    };

    /// Creates a writer for updating the TM
    std::shared_ptr<Writer> CreateWriter();

    /// Returns statistics about the TM
    void GetStats(long& numDocs, long& fileSize);

private:
    TranslationMemory();
    ~TranslationMemory();

    TranslationMemoryImpl *m_impl;
    static TranslationMemory *ms_instance;
};


#endif // _TRANSMEM_H_
