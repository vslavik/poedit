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

#include <wx/string.h>
#include <wx/stdpaths.h>
#include <wx/filename.h>
#include <wx/config.h>
#include <wx/tokenzr.h>
#include <wx/msgdlg.h>

#include <set>
#include <memory>

/** \page db_desc Translation Memory Algorithms

    \section tm_def TM Definition
    
    For the purposes of algorithm description, let's say that TM is a 
    database that stores original string-translation pairs (where both 
    original string and translation are strings consisting of words 
    delimined by spaces and/or interpunction) and supports inexact 
    retrieval with original string as primary key. Inexact retrieval means
    that TM will return non-empty response even though there's no record
    with given key. In such case, TM will return \e similar records, that is
    records whose key differs in no more than N words from searched key
    and is at worst M words longer.
    
    \section tm_store Storage
    
    Data are stored in three tables implemented as Berkeley DB databases
    (they have a feature important for TM: all data are stored as 
    string_key:value pairs and B-tree or hash table is used for very fast
    access to records; records are variable-length).
    
    All strings are encoded in UTF-8.
    
    Table one, DbOrig, contains original strings. Its key is original string
    and stored value is 32bit ID of the string (which is identical to
    record's index in DbTranse table, see below). There's 1-1 correspondence 
    between original strings and indexes.
    
    Table two, DbTrans, holds translations of original strings. Unlike DbOrig,
    this one is indexed with IDs, which gives us fastest possible access to
    this table. Record's value in DbTrans is UTF-8 encoded string buffer that
    contains one or more NULL-terminated strings. (Number of translations in
    record is trivially equal to number of zeros in the buffer; this approach
    makes adding translations to existing record very simple.)
    
    These two tables fully describe TM's content, but they only allow exact
    retrieval. 
    
    The last table, DbWords, is the core of inexact lookup feature. It is 
    indexed with a tuple of word (converted to lowercase) and sentence length.
    The value is a list of IDs of original strings of given length that 
    contain given word. These lists are relatively small even in large 
    databases; this is thanks to fragmentation caused by sentence length part
    of the key. An important property of ID lists is that they are always 
    sorted - we'll need this later.
    
    
    \section tm_ops Operations
    
    TM supports two operations: 
    - Store(source_string, translation)
    - Lookup(string, max_words_diff, max_length_delta). This operation returns
      array of results and integer value indicating exactness of result
      (0=worst, 100=exact). All returned strings are of same exactness.


    \subsection tm_write Writing to TM

    First, TM tries to find \a source_string in DbOrig. This is a trivial
    case - if TM finds it, it reads the record with obtained ID from DbTrans,
    checks if the list already contains \a translation and if not, adds 
    \a translation to the list and writes it back to DbTrans. DBs are 
    consistent at this point and operation finished successfully.
    
    If DbOrig doesn't contain \a source_string, however, the situation is
    more complicated. TM writes \a translation to DbTrans and obtains ID
    (which equals new record's index in DbTrans).
    It then writes \a source_string and this ID to DbOrig. Last, TM converts
    \a source_string to an array of words (by splitting it with usual
    word separators, converting to lowercase and removing bad words that
    are too common, such as "a", "the" or "will"). Number of words is used
    as sentence length and the ID is added to (word,length) records in 
    DbWords for all words in the sentence (adding new records as neccessary).
    (IDs are added to the end of list; this ensures, together with ID=index
    property, that IDs in DbWords are always sorted.)
    
    
    \subsection tm_lookup TM Lookup
    
    As a first attempt, exact match is tried, that is, TM tries to retrieve
    \a string from DbOrig. If an ID is found, matching translations are
    retrieved from DbTrans and returned together with exactness value of 100
    (highest possible).
    
    This happens only rarely, though. In more common scenario, TM tries
    to find similar entries. TM loops over i=0..max_words_diff and 
    j=0..max_length_delta ranges (the 2nd one is in inner loop) and attempts
    to find records with \e exactly i words missing in \e exactly j words
    longer sentences.
    
    To accomplish this, TM must find all possible combinations of \a i 
    omitted words among the total of N words. The algorithm then gets lists of
    IDs for non-omitted words for each such combination and computes 
    union of all ID lists. ID lists are sorted, so we can do this by merging 
    lists in O(n) time. If the union is not empty, the algorithm returns 
    translations identified by IDs in the union, together with success value 
    computed from i,j values as percentage of i,j-space that was already 
    processed.
    
    If all unions for all combinations and for all possible i,j values are
    empty, the algorithm fails.
    
    \remark
    - Time complexity of this algorithm is hard to determine; if we assume
      DB accesses are constant-time (which is not true; Berkeley DB access is
      mostly O(log n) and we do lots of string processing that doesn't exceed
      O(size of query)), then the worst case scenario involves 
      O(max_words_diff*max_length_delta*words_in_string) unifications and
      lookups, where union operation depends on sum of lengths of ID lists. A
      sample DB created from full RedHat 6.1 installation CD had lists smaller
      than 300 IDs. 
    - Real-life execution speed is more than satisfying - lookup takes hardly
      any time on an average Celeron 400MHz system.
    
 */


#ifdef USE_TRANSMEM

#include <wx/string.h>
#include <wx/tokenzr.h>
#include <wx/log.h>
#include <wx/intl.h>
#include <wx/utils.h>
#include <wx/filename.h>

#ifdef DB_HEADER
    #include DB_HEADER
#else
    #include <db_cxx.h>
#endif

#include "transmem.h"

typedef db_recno_t DbKey;
const DbKey DBKEY_ILLEGAL = 0;

class DbKeys
{
    public:
        DbKeys(const Dbt& data)
        {
            Count = data.get_size() / sizeof(DbKey);
            List = new DbKey[Count];
            memcpy(List, data.get_data(), data.get_size());
        }

        DbKeys(size_t cnt) : Count(cnt)
        {
            List = new DbKey[Count];
        }

        ~DbKeys()
        {
            delete[] List;
        }

        size_t Count;
        DbKey *List;
};

/// Simple OO interface to Berkeley DB database.
class DbBase
{
    public:
        /// Ctor.
        DbBase(DbEnv *env, const wxString& filename, DBTYPE type);

        void Release();

    protected:
        virtual ~DbBase() {}

        Db m_db;
};

/// Interface to the database of translations.
class DbTrans : public DbBase
{
    public:
        DbTrans(DbEnv *env, const wxString& path)
            : DbBase(env, path + _T("translations.db"), DB_RECNO) {}

        /** Writes array of translations for entry \a index to DB.
            \param strs   array of UTF-8 encoded strings to save 
            \param index  index of entry being modified. 
                          \c DBKEY_ILLEGAL if adding new entry.
         */
        DbKey Write(wxArrayString *strs, DbKey index = DBKEY_ILLEGAL);

        /** Writes translation for entry \a index to DB.
            \param str    UTF-8 encoded string to save 
            \param index  index of entry being modified. 
                          \c DBKEY_ILLEGAL if adding new entry.
         */
        DbKey Write(const wxString& str, DbKey index = DBKEY_ILLEGAL);

        /** Retrieves translations stored under given \index.
            \return array of translations or NULL if the key is absent.
            \remark The caller must delete returned object
         */
        wxArrayString *Read(DbKey index);

    protected:
        ~DbTrans() {}
};

/// Interface to DB of original strings.
class DbOrig : public DbBase
{
    public:
        DbOrig(DbEnv *env, const wxString& path)
            : DbBase(env, path + _T("strings.db"), DB_HASH) {}
        
        /** Returns index of string \a str or \c DBKEY_ILLEGAL if not found.
            Returned index can be used with DbTrans::Write and DbTrans::Read
         */
        DbKey Read(const wxString& str);

        /** Saves index value under which \a str's translation is stored
            in DbTrans.
            It is caller's responsibility to ensure \a value is consistent
            with DbTrans instance.
         */
        void Write(const wxString& str, DbKey value);

    protected:
        ~DbOrig() {}
};


/// Interface to DB of words.
class DbWords : public DbBase
{
    public:
        DbWords(DbEnv *env, const wxString& path)
            : DbBase(env, path + _T("words.db"), DB_HASH) {}
        
        /** Reads list of DbTrans indexes of translations of which
            original strings contained \a word and were \a sentenceSize
            words long. 
            \remarks Returned list is always sorted.
         */
        DbKeys *Read(const wxString& word, unsigned sentenceSize);

        /** Adds \a value to the list of DbTrans indexes stored for
            \a word and \a sentenceSize. 
            \see Read
         */
        void Append(const wxString& word, unsigned sentenceSize, DbKey value);

    protected:
        ~DbWords() {}
};



DbBase::DbBase(DbEnv *env, const wxString& filename, DBTYPE type)
    : m_db(env, 0)
{
    m_db.open(NULL,
#ifdef __WINDOWS__
              filename.utf8_str(),
#else
              filename.fn_str(),
#endif
              NULL,
              type,
              DB_CREATE | DB_AUTO_COMMIT,
              0);
}


void DbBase::Release()
{
    m_db.close(0);
    delete this;
}


DbKey DbTrans::Write(const wxString& str, DbKey index)
{
    wxArrayString a;
    a.Add(str);
    return Write(&a, index);
}

DbKey DbTrans::Write(wxArrayString *strs, DbKey index)
{
    size_t bufLen;
    size_t i;
    char *ptr;

    for (bufLen = 0, i = 0; i < strs->GetCount(); i++)
    {
        bufLen += strlen(strs->Item(i).mb_str(wxConvUTF8)) + 1;
    }
    wxCharBuffer buf(bufLen);
    for (ptr = buf.data(), i = 0; i < strs->GetCount(); i++)
    {
        strcpy(ptr, strs->Item(i).mb_str(wxConvUTF8));
        ptr += strlen(ptr) + 1;
    }

    Dbt data(buf.data(), bufLen);

    if (index == DBKEY_ILLEGAL)
    {
        Dbt key;
        m_db.put(NULL, &key, &data, DB_APPEND);
        return *((db_recno_t*)key.get_data());
    }
    else
    {
        Dbt key(&index, sizeof(index));
        m_db.put(NULL, &key, &data, 0);
        return index;
    }
}

wxArrayString *DbTrans::Read(DbKey index)
{
    Dbt key(&index, sizeof(index));
    Dbt data;

    if ( m_db.get(NULL, &key, &data, 0) == DB_NOTFOUND )
        return NULL;

    wxArrayString *arr = new wxArrayString;
    char *ptr = (char*)data.get_data();
    char *endptr = ((char*)data.get_data()) + data.get_size();
    while (ptr < endptr)
    {
        arr->Add(wxString(ptr, wxConvUTF8));
        ptr += strlen(ptr) + 1;
    }

    return arr;
}


DbKey DbOrig::Read(const wxString& str)
{
    const wxWX2MBbuf c_str_buf = str.mb_str(wxConvUTF8);
    const char *c_str = c_str_buf;

    Dbt key((void*)c_str, strlen(c_str));
    Dbt data;

    if ( m_db.get(NULL, &key, &data, 0) == DB_NOTFOUND )
        return DBKEY_ILLEGAL;

    return *((DbKey*)data.get_data());
}


void DbOrig::Write(const wxString& str, DbKey value)
{
    const wxWX2MBbuf c_str_buf = str.mb_str(wxConvUTF8);
    const char *c_str = c_str_buf;

    Dbt key((void*)c_str, strlen(c_str));
    Dbt data(&value, sizeof(value));

    m_db.put(NULL, &key, &data, 0);
}


DbKeys *DbWords::Read(const wxString& word, unsigned sentenceSize)
{
    const wxWX2MBbuf word_mb = word.mb_str(wxConvUTF8);
    size_t keyLen = strlen(word_mb) + sizeof(wxUint32);
    wxCharBuffer keyBuf(keyLen+1);
    strcpy(keyBuf.data() + sizeof(wxUint32), word_mb);
    *((wxUint32*)(keyBuf.data())) = sentenceSize;

    Dbt key(keyBuf.data(), keyLen);
    Dbt data;

    if ( m_db.get(NULL, &key, &data, 0) == DB_NOTFOUND )
        return NULL;

    return new DbKeys(data);
}


void DbWords::Append(const wxString& word, unsigned sentenceSize, DbKey value)
{
    // VS: there is a dirty trick: it is always true that 'value' is 
    //     greater than all values already present in the db, so we may
    //     append it to the end of list while still keeping the list sorted.
    //     This is important because it allows us to efficiently merge
    //     these lists when looking up inexact translations...

    const wxWX2MBbuf word_mb = word.mb_str(wxConvUTF8);
    DbKey *valueBuf;

    size_t keyLen = strlen(word_mb) + sizeof(wxUint32);
    wxCharBuffer keyBuf(keyLen+1);
    strcpy(keyBuf.data() + sizeof(wxUint32), word_mb);
    *((wxUint32*)(keyBuf.data())) = sentenceSize;

    Dbt key(keyBuf.data(), keyLen);
    Dbt data;

    std::auto_ptr<DbKeys> keys(Read(word, sentenceSize));
    if (keys.get() == NULL)
    {
        valueBuf = NULL;
        data.set_data(&value);
        data.set_size(sizeof(DbKey));
    }
    else
    {
        valueBuf = new DbKey[keys->Count + 1];
        memcpy(valueBuf, keys->List, keys->Count * sizeof(DbKey));
        valueBuf[keys->Count] = value;
        data.set_data(valueBuf);
        data.set_size((keys->Count + 1) * sizeof(DbKey));
    }

    try
    {
        m_db.put(NULL, &key, &data, 0);
    }
    catch ( ... )
    {
        delete[] valueBuf;
        throw;
    }
}




// ---------------- helper functions ----------------

#define WORD_SEPARATORS _T(" \t\r\n\\~`!@#$%^&*()-_=+|[]{};:'\"<>,./?")

/// Extracts list of words from \a string.
static void StringToWordsArray(const wxString& str, wxArrayString& array)
{
    static wxSortedArrayString *BadWords = NULL;
    
    if (BadWords == NULL)
    {
        // some words are so common in English we would be crazy to put
        // them into index. (The list was taken from ht://Dig.)
        BadWords = new wxSortedArrayString;
        BadWords->Add(_T("a"));
        //BadWords->Add(_T("all"));
        BadWords->Add(_T("an"));
        //BadWords->Add(_T("are"));
        //BadWords->Add(_T("can"));
        //BadWords->Add(_T("for"));
        //BadWords->Add(_T("from"));
        BadWords->Add(_T("have"));
        //BadWords->Add(_T("it"));
        //BadWords->Add(_T("may"));
        //BadWords->Add(_T("not"));
        BadWords->Add(_T("of"));
        //BadWords->Add(_T("that"));
        BadWords->Add(_T("the"));
        //BadWords->Add(_T("this"));
        //BadWords->Add(_T("was"));
        BadWords->Add(_T("will"));
        //BadWords->Add(_T("with"));
        //BadWords->Add(_T("you"));
        //BadWords->Add(_T("your"));
    }

    wxString s;
    wxStringTokenizer tkn(str, WORD_SEPARATORS, wxTOKEN_STRTOK);
    array.Clear();
    while (tkn.HasMoreTokens())
    {
        s = tkn.GetNextToken().Lower();
        if (s.Len() == 1) continue;
        if (array.Index(s) != wxNOT_FOUND) continue;
        if (BadWords->Index(s) != wxNOT_FOUND) continue;
        array.Add(s);
    }
}



static DbKeys *UnionOfDbKeys(size_t cnt, DbKeys *keys[], bool mask[])
{
    size_t i, minSize;
    DbKey **heads = new DbKey*[cnt];
    size_t *counters = new size_t[cnt];

    // initialize heads and counters _and_ find size of smallest keys list
    // (union can't be larger than that)
    for (minSize = 0, i = 0; i < cnt; i++)
    {
        if (mask[i])
        {
            counters[i] = keys[i]->Count;
            heads[i] = keys[i]->List;
            if (minSize == 0 || minSize > keys[i]->Count)
                minSize = keys[i]->Count;
        }
        else
        {
            counters[i] = 0;
            heads[i] = NULL;
        }
    }
    if (minSize == 0)
    {
        delete[] counters;
        delete[] heads;
        return NULL;
    }

    DbKeys *result = new DbKeys(minSize);
    result->Count = 0;
    
    // Do union of 'cnt' sorted arrays. Algorithm: treat arrays as lists,
    // remember pointer to first unprocessed item. In each iteration, do:
    // if all heads have same value, add that value to output list, otherwise
    // move the head with smallest value one item forward. (This way we won't
    // miss any value that could possibly be in the output list, because 
    // if the smallest value of all heads is not at the beginning of other
    // lists, it cannot appear later in these lists due to their sorted nature.
    // We end processing when any head reaches end of (its) list
    DbKey smallestValue;
    size_t smallestIndex = 0;
    bool allSame;
    for (;;)
    {
        allSame = true;
        smallestValue = DBKEY_ILLEGAL;

        for (i = 0; i < cnt; i++)
        {
            if (counters[i] == 0) continue;
            if (smallestValue == DBKEY_ILLEGAL)
            {
                smallestValue = *heads[i];
                smallestIndex = i;
            }
            else if (smallestValue != *heads[i])
            {
                allSame = false;
                if (smallestValue > *heads[i])
                {
                    smallestValue = *heads[i];
                    smallestIndex = i;
                }
            }
        }

        if (smallestValue == DBKEY_ILLEGAL) break;
        
        if (allSame)
        {
            bool breakMe = false;
            result->List[result->Count++] = smallestValue;
            for (i = 0; i < cnt; i++)
            {
                if (counters[i] == 0) continue;
                if (--counters[i] == 0) { breakMe = true; break; }
                heads[i]++;
            }
            if (breakMe) break;
        }
        else
        {
            if (--counters[smallestIndex] == 0) break;
            heads[smallestIndex]++;
        }
    }
    
    delete[] counters;
    delete[] heads;   
    if (result->Count == 0)
    {
        delete result;
        result = NULL;
    }

    return result;
}


static inline wxString MakeLangDbPath(const wxString& p, const wxString& l)
{
    if (!wxDirExists(p))
        return wxEmptyString;

    wxString db = p + _T("/") + l;
    if (wxDirExists(db)) 
        return db;
    if (l.Len() == 5)
    {
        db = p + _T("/") + l.Mid(0,2);
        if (wxDirExists(db))
            return db;
    }
    if (l.Len() == 2)
    {
        wxString l2 = wxFindFirstFile(p + _T("/") + l + _T("_??"), wxDIR);
        if (!!l2)
            return l2;
    }
    return wxEmptyString;
}




namespace
{

typedef std::set<TranslationMemory*> TranslationMemories;
TranslationMemories ms_instances;
DbEnv *gs_dbEnv = NULL;


void ReportDbError(const DbException& e)
{
    wxString what(e.what(), wxConvLocal);
    wxLogError(_("Translation memory database error: %s"), what.c_str());
}


DbEnv *CreateDbEnv(const wxString& path)
{
    wxLogTrace(_T("poedit.tm"),
               _T("initializing BDB environment in %s"),
               path.c_str());

    const u_int32_t flags = DB_INIT_MPOOL |
                            DB_INIT_LOCK |
                            DB_INIT_LOG |
                            DB_INIT_TXN |
                            DB_RECOVER |
                            DB_CREATE;

    std::auto_ptr<DbEnv> env(new DbEnv(0));
#ifdef __WINDOWS__
    env->open(path.utf8_str(), flags, 0600);
#else
    env->open(path.fn_str(), flags, 0600);
#endif

    // This prevents the log from growing indefinitely
    env->log_set_config(DB_LOG_AUTO_REMOVE, 1);

    return env.release();
}


void DestroyDbEnv(DbEnv *env)
{
    try
    {
        const char *dbpath_c;
        gs_dbEnv->get_home(&dbpath_c);
        std::string dbpath(dbpath_c);

        wxLogTrace(_T("poedit.tm"),
                   _T("closing and removing BDB environment in %s"),
                   wxString(dbpath.c_str(), wxConvUTF8).c_str());

        gs_dbEnv->close(0);

        // Clean up temporary environment files (unless they're still in use):
        DbEnv env2(0);
        env2.remove(dbpath.c_str(), 0);

        wxLogTrace(_T("poedit.tm"),
                   _T("closed and removed BDB environment"));
    }
    catch ( const DbException& e)
    {
        ReportDbError(e);
    }

    delete env;
}

} // anonymous namespace

/*static*/
TranslationMemory *TranslationMemory::Create(const wxString& language)
{
    wxString dbPath = MakeLangDbPath(GetDatabaseDir(), language);
    if (!dbPath)
        dbPath = GetDatabaseDir() + wxFILE_SEP_PATH + language;

    if ( !wxFileName::Mkdir(dbPath, 0700, wxPATH_MKDIR_FULL) )
    {
        wxLogError(_("Cannot create TM database directory!"));
        return NULL;
    }
    dbPath += _T('/');

    for (TranslationMemories::const_iterator i = ms_instances.begin();
         i != ms_instances.end(); ++i)
    {
        TranslationMemory *mem = *i;
        if (mem->m_lang == language && mem->m_dbPath == dbPath)
        {
            mem->m_refCnt++;
            return mem;
        }
    }

    try
    {
        if ( !gs_dbEnv )
            gs_dbEnv = CreateDbEnv(GetDatabaseDir());

        TranslationMemory *tm = new TranslationMemory(language, dbPath);
        ms_instances.insert(tm);
        return tm;
    }
    catch ( const DbException& e )
    {
        ReportDbError(e);
        return NULL;
    }
}

void TranslationMemory::Release()
{
    if (--m_refCnt == 0)
    {
        try
        {
            m_dbTrans->Release();
        }
        catch ( const DbException& e) { ReportDbError(e); }

        try
        {
            m_dbOrig->Release();
        }
        catch ( const DbException& e) { ReportDbError(e); }

        try
        {
            m_dbWords->Release();
        }
        catch ( const DbException& e) { ReportDbError(e); }

        ms_instances.erase(this);
        delete this;

        if ( gs_dbEnv && ms_instances.empty() && gs_dbEnv )
        {
            DestroyDbEnv(gs_dbEnv);
            gs_dbEnv = NULL;
        }
    }
}

TranslationMemory::TranslationMemory(const wxString& language,
                                     const wxString& dbPath)
{
    m_dbPath = dbPath;
    m_refCnt = 1;
    m_lang = language;
    SetParams(2, 2);

    try
    {
        m_dbTrans = NULL;
        m_dbOrig = NULL;
        m_dbWords = NULL;

        m_dbTrans = new DbTrans(gs_dbEnv, dbPath);
        m_dbOrig = new DbOrig(gs_dbEnv, dbPath);
        m_dbWords = new DbWords(gs_dbEnv, dbPath);
    }
    catch ( ... )
    {
        if ( m_dbTrans )
            m_dbTrans->Release();
        if ( m_dbOrig )
            m_dbOrig->Release();
        if ( m_dbWords )
            m_dbWords->Release();

        throw;
    }
}

TranslationMemory::~TranslationMemory()
{
}

/*static*/ bool TranslationMemory::IsSupported(const wxString& lang)
{
    return !!MakeLangDbPath(GetDatabaseDir(), lang);
}

bool TranslationMemory::Store(const wxString& string,
                              const wxString& translation)
{
    try
    {
        DbKey key = m_dbOrig->Read(string);

        if (key == DBKEY_ILLEGAL)
        {
            key = m_dbTrans->Write(translation);
            m_dbOrig->Write(string, key);

            wxArrayString words;
            StringToWordsArray(string, words);
            const size_t sz = words.GetCount();
            for (size_t i = 0; i < sz; i++)
                m_dbWords->Append(words[i], sz, key);

            return true;
        }
        else
        {
            std::auto_ptr<wxArrayString> t(m_dbTrans->Read(key));
            if (!t.get())
                return false;

            if (t->Index(translation) == wxNOT_FOUND)
            {
                t->Add(translation);
                m_dbTrans->Write(t.get(), key);
            }

            return true;
        }
    }
    catch ( const DbException& e )
    {
        ReportDbError(e);
        return false;
    }
}


int TranslationMemory::Lookup(const wxString& string, wxArrayString& results)
{
    results.Clear();

    try
    {
        // First of all, try exact match:
        DbKey key = m_dbOrig->Read(string);
        if (key != DBKEY_ILLEGAL)
        {
            std::auto_ptr<wxArrayString> a(m_dbTrans->Read(key));
            if (a.get())
            {
                WX_APPEND_ARRAY(results, (*a));
                return 100;
            }
        }

        // Then, try to find inexact one within defined limits
        // (MAX_OMITS is max permitted number of unmatched words,
        // MAX_DELTA is max difference in sentences lengths).
        // Start with best matches first, continue to worse ones.
        wxArrayString words;
        StringToWordsArray(string, words);
        for (unsigned omits = 0; omits <= m_maxOmits; omits++)
        {
            for (size_t delta = 0; delta <= m_maxDelta; delta++)
            {
                if (LookupFuzzy(words, results, omits, delta))
                {
                    int score =
                        (m_maxOmits - omits) * 100 / (m_maxOmits + 1) +
                        (m_maxDelta - delta) * 100 / 
                                ((m_maxDelta + 1) * (m_maxDelta + 1));
                    return (score == 0) ? 1 : score;
                }
            }
        }

        return 0;
    }
    catch ( const DbException& e )
    {
        ReportDbError(e);
        return 0;
    }
}



static bool AdvanceCycle(size_t omitted[], size_t depth, size_t cnt)
{
    if (++omitted[depth] == cnt)
    {
        if (depth == 0) 
            return false;
        depth--;
        if (!AdvanceCycle(omitted, depth, cnt)) return false;
        depth++;
        omitted[depth] = omitted[depth - 1] + 1;
        if (omitted[depth] >= cnt) return false;
        return true;
    }
    else
        return true;
}

bool TranslationMemory::LookupFuzzy(const wxArrayString& words, 
                                    wxArrayString& results, 
                                    unsigned omits, int delta)
{
    #define RETURN_WITH_CLEANUP(val) \
        { \
        for (i = 0; i < cntOrig; i++) delete keys[i]; \
        delete[] keys; \
        delete[] mask; \
        delete[] omitted; \
        return (val); \
        }
        
    size_t cnt = words.GetCount();
    size_t cntOrig = cnt;
    
    if (omits >= cnt) 
        // such search would yield all entries of given length
        return false;
    if (cnt + delta <= 0)
        return false;
        
    size_t i, slot;
    size_t missing;
    DbKeys **keys = new DbKeys*[cnt];
    bool *mask = new bool[cnt];
    size_t *omitted = new size_t[omits];

    try
    {
        for (missing = 0, slot = 0, i = 0; i < cnt; i++)
        {
            keys[i] = NULL; // so that unused entries are NULL
            keys[slot] = m_dbWords->Read(words[i], cnt + delta);
            if (keys[slot])
                slot++;
            else
                missing++;
        }

        if (missing >= cnt || missing > omits)
            RETURN_WITH_CLEANUP(false)
        cnt -= missing;
        omits -= missing;

        if (omits == 0)
        {
            for (i = 0; i < cnt; i++) mask[i] = true;
            DbKeys *result = UnionOfDbKeys(cnt, keys, mask);
            if (result != NULL)
            {
                for (i = 0; i < result->Count; i++)
                {
                    wxArrayString *a = m_dbTrans->Read(result->List[i]);
                    if (a)
                    {
                        WX_APPEND_ARRAY(results, (*a));
                        delete a;
                    }
                }
                delete result;
                RETURN_WITH_CLEANUP(true)
            }
        }
        else
        {
            DbKeys *result;
            size_t depth = omits - 1;
            for (i = 0; i < omits; i++) omitted[i] = i;
            for (;;)
            {
                for (i = 0; i < cnt; i++) mask[i] = true;
                for (i = 0; i < omits; i++) mask[omitted[i]] = false;

                result = UnionOfDbKeys(cnt, keys, mask);
                if (result != NULL)
                {
                    for (i = 0; i < result->Count; i++)
                    {
                        wxArrayString *a = m_dbTrans->Read(result->List[i]);
                        if (a)
                        {
                            WX_APPEND_ARRAY(results, (*a));
                            delete a;
                        }
                    }
                    delete result;
                    RETURN_WITH_CLEANUP(true)
                }

                if (!AdvanceCycle(omitted, depth, cnt)) break;
            }
        }

        RETURN_WITH_CLEANUP(false)
    }
    catch ( const DbException& e )
    {
        ReportDbError(e);
        RETURN_WITH_CLEANUP(false);
    }

    #undef RETURN_WITH_CLEANUP
}


/*static*/
wxString TranslationMemory::GetDatabaseDir()
{
    wxString data;
#if defined(__UNIX__) && !defined(__WXMAC__)
    if ( !wxGetEnv(_T("XDG_DATA_HOME"), &data) )
        data = wxGetHomeDir() + _T("/.local/share");
    data += _T("/poedit");
#else
    data = wxStandardPaths::Get().GetUserDataDir();
#endif

    data += wxFILE_SEP_PATH;
    data += _T("TM");
    return data;
}

namespace
{

bool MoveDBFile(const wxString& from, const wxString& to, const wxString& name)
{
    wxString f = from + wxFILE_SEP_PATH + name;
    wxString t = to + wxFILE_SEP_PATH + name;

    if ( wxRenameFile(f, t) )
        return true;

    if ( !wxCopyFile(f, t, false) )
        return false;

    return wxRemoveFile(f);
}

bool MoveDBLang(const wxString& from, const wxString& to)
{
    // maybe the files are on different volumes, try copying them
    if ( !wxFileName::DirExists(to) )
    {
        if ( !wxFileName::Mkdir(to, 0700, wxPATH_MKDIR_FULL) )
            return false;
    }

    bool ok1 = MoveDBFile(from, to, _T("strings.db"));
    bool ok2 = MoveDBFile(from, to, _T("translations.db"));
    bool ok3 = MoveDBFile(from, to, _T("words.db"));

    bool ok = ok1 && ok2 && ok3;

    if ( ok )
        return wxFileName::Rmdir(from);

    return ok;
}

} // anonymous namespace

/*static*/
void TranslationMemory::MoveLegacyDbIfNeeded()
{
    wxASSERT_MSG( ms_instances.empty(),
                  _T("TM cannot be migrated if already in use") );

    wxConfigBase *cfg = wxConfig::Get();

    const wxString oldPath = cfg->Read(_T("/TM/database_path"), _T(""));

    if ( oldPath.empty() )
        return; // nothing to do, legacy setting not present

    const wxString newPath = GetDatabaseDir();

    if ( oldPath == newPath )
        return; // already in the right location, nothing to do

#if defined(__WXMSW__) || defined(__WXMAC__)
    if ( oldPath.IsSameAs(newPath, /*caseSensitive=*/false) )
    {
        // This is no real difference and migration would fail, so just update
        // the path:
        // For now, keep the config setting, even though it's obsolete, just in
        // case some users downgrade. (FIXME)
        cfg->Write(_T("/TM/database_path"), newPath);
        return;
    }
#endif

    if ( !wxFileName::DirExists(oldPath) )
        return; // no TM data at all, don't bother

    wxLogTrace(_T("poedit.tm"),
               _T("moving TM database from old location \"%s\" to \"%s\""),
               oldPath.c_str(), newPath.c_str());

    const wxString tmLangsStr = cfg->Read(_T("/TM/languages"), _T(""));
    wxStringTokenizer tmLangs(tmLangsStr, _T(":"));
    wxLogTrace(_T("poedit.tm"),
               _T("languages to move: %s"), tmLangsStr.c_str());

    bool ok = true;
    {
        wxLogNull null;
        while ( tmLangs.HasMoreTokens() )
        {
            const wxString lang = tmLangs.GetNextToken();

            if ( !wxFileName::DirExists(oldPath + wxFILE_SEP_PATH + lang) )
            {
                ok = false;
                continue; // TM for this lang doesn't exist
            }

            if ( !wxFileName::DirExists(newPath) )
            {
                if ( !wxFileName::Mkdir(newPath, 0700, wxPATH_MKDIR_FULL) )
                {
                    ok = false;
                    continue; // error
                }
            }
            if ( !MoveDBLang(oldPath + wxFILE_SEP_PATH + lang,
                             newPath + wxFILE_SEP_PATH + lang) )
            {
                ok = false;
                continue; // error
            }
        }

        // intentionally don't delete the old path recursively, the user may
        // have pointed it to a location with their own files:
        if ( ok )
            ok = wxFileName::Rmdir(oldPath);
    }

    // For now, keep the config setting, even though it's obsolete, just in
    // case some users downgrade. (FIXME)
    cfg->Write(_T("/TM/database_path"), newPath);

    if ( !ok )
    {
        const wxString title =
            _("Poedit translation memory error");
        const wxString main =
            _("There was a problem moving your translation memory.");
        const wxString details =
            wxString::Format(
            _("Please verify that all files were moved to the new location or do it manually if they weren't.\n\nOld location: %s\nNew location: %s"),
            oldPath.c_str(), newPath.c_str());

#if wxCHECK_VERSION(2,9,0)
        wxMessageDialog dlg(NULL, main, title, wxOK | wxICON_ERROR);
        dlg.SetExtendedMessage(details);
        dlg.SetYesNoLabels(_("Purge"), _("Keep"));
#else // wx < 2.9.0
        const wxString all = main + _T("\n\n") + details;
        wxMessageDialog dlg(NULL, all, title, wxOK | wxICON_ERROR);
#endif
        dlg.ShowModal();
    }
}

#endif // USE_TRANSMEM

