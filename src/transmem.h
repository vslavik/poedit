/*
 *  This file is part of Poedit (http://www.poedit.net)
 *
 *  Copyright (C) 2001-2013 Vaclav Slavik
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

#ifdef USE_TRANSMEM

class WXDLLIMPEXP_FWD_BASE wxString;

class DbTrans;
class DbOrig;
class DbWords;

/** TranslationMemory is a class that represents so-called translation memory
    (surprise ;), a mechanism used to speed up translator's work by 
    automatically finding translations based on the knowledge of all 
    sentence-translation pair previously entered to the system. It fails
    into the category of machine-aided human translation. 
    
    Typically, TM successfully finds translations for strings very similar
    to these in its database. For example, knowing the translation of 
    "What a nice day!", TranslationMemory will identify "What a beatiful
    day!" as similar to entry in the DB and will return fuzzy-marked 
    translation of "What a nice day!" as suggested translation of the second
    sentence. Human translator then changes one word, which is faster
    than typing the whole sentence and, more importantly, coming with it. 
    The main drawback of this method is nicely illustrated by the sentence
    "What a terrible day!", for which it, of course, returns the very same
    translation. 
    
    \see \ref db_desc
 */
class TranslationMemory
{
    public:
        /** Ctor. Constructs TM object that will use database stored in
            given location.

            \return NULL if failed (e.g. cannot load DLL under Windows),
                    constructed object otherwise.

            \see See IsSupported for rules on language name matching.
            \remark Don't delete TranslationMemory objects, use Release 
                    instead! The reason for this is that if you call
                    Create several times with same arguments, it returns
                    pointer to the same instance
                    
         */
        static TranslationMemory *Create(const wxString& language);

        void Release();

        /// Returns language of the catalog.
        wxString GetLanguage() const { return m_lang; }

        /** Returns whether there is TM database for given language.
            The database needs not have exact name, following rules
            apply (\i la and \i la_NG are 2- and 5-letter lang. codes):
              - if \a lang directory exists, return true
              - if \a lang is 2-letter, try any \i la_?? language
              - if \a lang is 5-letter, try \i la instead of \i la_NG
         */
        static bool IsSupported(const wxString& lang);
        
        /** Saves string and its translation into DB. 
         */
        bool Store(const wxString& string, const wxString& translation);

        /** Retrieves translation of given string from DB. Gets exact
            translation if possible and tries to find closest match
            otherwise. 
                    
            \return Returns score of found translations 
                    (100 - exact match, 0 - nothing found). Translations
                    are stored in \a results. 

            \see See \ref db_desc for details on the algorithm.
         */
        int Lookup(const wxString& string, wxArrayString& results);

        /** Sets parameters of inexact lookup.
            \param maxOmits number of words on input that can be ignored
            \param maxDelta look in sentences that are longer than the
                            sentense by at worst this number of words
         */
        void SetParams(size_t maxDelta, size_t maxOmits)
            { m_maxDelta = maxDelta, m_maxOmits = maxOmits; }

        // Return location of TM database files
        static wxString GetDatabaseDir();

        /**
            Moves the TM from old location (user-configurable,
            /TM/database_path in config) to a standard OS-specific location.
         */
        static void MoveLegacyDbIfNeeded();


    protected:
        /// Real ctor.
        TranslationMemory(const wxString& language,
                          const wxString& dbPath);

        /** Tries to find entries matching given criteria. Used by Lookup.
            It takes arguments that specify level of "fuzziness" used during
            the lookup. Specifically, two inexact lookup methods are 
            implemented: 
              - Searching in sentences that are longer than the query by 
                specified number of words.
              - Ignoring given number of words in input, i.e. trying to
                match the sentences partially. For instance, LookupFuzzy
                may report sucessfull match if 4 of 5 words match.
            
            \param words   list of words that make the string 
            \param results where to store output if found
            \param omits   number of words that will be ignored (not maximum,
                           but exactly this number)
            \param delta   look only in sentences that are longer than 
                           \a words.GetCount() by exactly this number
            
            \return true if LookupFuzzy found something, false otherwise
    
            \remarks
            Lookup calls this function several times with different \a omits
            and \a delta arguments.
         */
        bool LookupFuzzy(const wxArrayString& words, wxArrayString& results, 
                         unsigned omits, int delta);

    private:
        ~TranslationMemory();
        
    private:
        DbTrans *m_dbTrans;
        DbOrig  *m_dbOrig;
        DbWords *m_dbWords;
        wxString m_lang;
        wxString m_dbPath;
        size_t   m_maxDelta, m_maxOmits;
        
        int      m_refCnt;
};


#endif // USE_TRANSMEM

#endif // _TRANSMEM_H_
