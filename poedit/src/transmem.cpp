
/*

    poedit, a wxWindows i18n catalogs editor

    ---------------
      transmem.cpp
    
      Translation memory database
    
      (c) Vaclav Slavik, 2001

*/


#ifdef __GNUG__
#pragma implementation
#endif

#include <wx/wxprec.h>


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
    record's index in DbTranse table, see bellow). There's 1-1 correspondence 
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
    - Store(original_string, translation)
    - Lookup(string, max_words_diff, max_length_delta). This operation returns
      array of results and integer value indicating exactness of result
      (0=worst, 100=exact). All returned strings are of same exactness.


    \subsection tm_write Writing to TM

    First, TM tries to find \a original_string in DbOrig. This is a trivial
    case - if TM finds it, it reads the record with obtained ID from DbTrans,
    checks if the list already contains \a translation and if not, adds 
    \a translation to the list and writes it back to DbTrans. DBs are 
    consistent at this point and operation finished successfully.
    
    If DbOrig doesn't contain \a original_string, however, the situation is
    more complicated. TM writes \a translation to DbTrans and obtains ID
    (which equals new record's index in DbTrans).
    It then writes \a original_string and this ID to DbOrig. Last, TM converts
    \a original_string to an array of words (by splitting it with usual
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
      O(max_words_diff*max_length_delta*words_in_string) unifications and lookups,
      where union operation depends on sum of lengths of ID lists. A sample
      DB created from full RedHat 6.1 installation CD had lists smaller than
      300 IDs. 
    - Real-life execution speed is more than satisfying - lookup takes hardly any
      time on an average Celeron 400MHz system.
    
 */


#ifdef USE_TRANSMEM

#include <wx/string.h>
#include <wx/tokenzr.h>
#include <wx/log.h>
#include <wx/intl.h>
#include <wx/utils.h>

#ifdef USE_REDHAT_DB3
#include <db3/db.h>
#else
#include <db.h>
#endif

#include "transmem.h"

#ifdef __WINDOWS__

#include <wx/dynlib.h>
#include <wx/module.h>

static wxDynamicLibrary *g_libraryDB = NULL;
static bool g_triedLibraryDB = false;
typedef int (*db_create_t) __P((DB **, DB_ENV *, u_int32_t));
typedef char (*db_strerror_t) __P((int));
static db_create_t g_db_create = NULL;
static db_strerror_t g_db_strerror = NULL;

class wxDBModule: public wxModule
{
DECLARE_DYNAMIC_CLASS(wxDBModule)
public:
    wxDBModule() {}
    bool OnInit() { return true; }
    void OnExit() 
    {
        if (g_libraryDB)
            delete g_libraryDB;
    }
};
IMPLEMENT_DYNAMIC_CLASS(wxDBModule, wxModule)

#endif



typedef db_recno_t DbKey;
const DbKey DBKEY_ILLEGAL = 0;

class DbKeys
{
    public:
        DbKeys(DBT& data)
        {
            Count = data.size / sizeof(DbKey);
            List = new DbKey[Count];
            memcpy(List, data.data, data.size);
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
        DbBase(const wxString& filename, DBTYPE type);
        virtual ~DbBase();
        /// Returns false if opening DB failed
        bool IsOk() const { return m_ok; }
        /// Logs last DB error and sets error variable to "no error".
        void LogError();
        
    protected:
        DB *m_db;
        bool m_ok;
        int m_err;
};

/// Interface to the database of translations.
class DbTrans : public DbBase
{
    public:
        DbTrans(const wxString& path) : DbBase(path + _T("translations.db"), DB_RECNO) {}

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
};

/// Interface to DB of original strings.
class DbOrig : public DbBase
{
    public:
        DbOrig(const wxString& path) : DbBase(path + _T("strings.db"), DB_HASH) {}
        
        /** Returns index of string \a str or \c DBKEY_ILLEGAL if not found.
            Returned index can be used with DbTrans::Write and DbTrans::Read
         */
        DbKey Read(const wxString& str);

        /** Saves index value under which \a str's translation is stored
            in DbTrans.
            It is caller's responsibility to ensure \a value is consistent
            with DbTrans instance.
         */
        bool Write(const wxString& str, DbKey value);
};


/// Interface to DB of words.
class DbWords : public DbBase
{
    public:
        DbWords(const wxString& path) : DbBase(path + _T("words.db"), DB_HASH) {}
        
        /** Reads list of DbTrans indexes of translations of which
            original strings contained \a word and were \a sentenceSize
            words long. 
            \remarks Returned list is always sorted.
         */
        DbKeys *Read(const wxString& word, unsigned sentenceSize);

        /** Adds \a value to the list of DbTrans indexes stored for
            \a word and \a sentenceSize. 
            \return false on DB failure, true otherwise
            \see Read
         */
        bool Append(const wxString& word, unsigned sentenceSize, DbKey value);
};



DbBase::DbBase(const wxString& filename, DBTYPE type)
{
    m_ok = false;
#ifdef __WINDOWS__
    m_err = g_db_create(&m_db, NULL, 0);
#else
    m_err = db_create(&m_db, NULL, 0);
#endif
    if (m_err != 0)
    {
        LogError();
        return;
    }
    m_err = m_db->open(m_db, filename.mb_str(), NULL, type, DB_CREATE, 0);
    if (m_err != 0)
    {
        LogError();
        return;
    }
    m_ok = true;
}

DbBase::~DbBase()
{
    if ((m_err = m_db->close(m_db, 0)) != 0)
        LogError();
}

void DbBase::LogError()
{
#ifdef __WINDOWS__
    wxLogError(_("Database error: %s"), g_db_strerror(m_err));
#else
    wxLogError(_("Database error: %s"), db_strerror(m_err));
#endif
    m_err = 0;
}



DbKey DbTrans::Write(const wxString& str, DbKey index)
{
    wxArrayString a;
    a.Add(str);
    return Write(&a, index);
}

DbKey DbTrans::Write(wxArrayString *strs, DbKey index)
{
    DBT key, data;
    char *buf;
    size_t bufLen;
    size_t i;
    char *ptr;

    for (bufLen = 0, i = 0; i < strs->GetCount(); i++)
    {
        bufLen += strlen(strs->Item(i).mb_str(wxConvUTF8)) + 1;
    }
    buf = new char[bufLen];
    for (ptr = buf, i = 0; i < strs->GetCount(); i++)
    {
        strcpy(ptr, strs->Item(i).mb_str(wxConvUTF8));
        ptr += strlen(ptr) + 1;
    }

    memset(&key, 0, sizeof(key));
    memset(&data, 0, sizeof(data));
    data.data = buf;
    data.size = bufLen;

    if (index == DBKEY_ILLEGAL)
    {
        m_err = m_db->put(m_db, NULL, &key, &data, DB_APPEND);
    }
    else
    {
        key.data = &index;
        key.size = sizeof(index);
        m_err = m_db->put(m_db, NULL, &key, &data, 0);
    }
    delete[] buf;

    if (m_err != 0)
    {
        LogError();
        return DBKEY_ILLEGAL;
    }    
    return (index == DBKEY_ILLEGAL) ? *((db_recno_t*)key.data) : index;
}

wxArrayString *DbTrans::Read(DbKey index)
{
    DBT key, data;

    memset(&key, 0, sizeof(key));
    memset(&data, 0, sizeof(data));
    key.data = &index;
    key.size = sizeof(index);
    m_err = m_db->get(m_db, NULL, &key, &data, 0);

    if (m_err != 0)
    {
        LogError();
        return NULL;
    }
    
    wxArrayString *arr = new wxArrayString;
    char *ptr = (char*)data.data;
    while (ptr < ((char*)data.data) + data.size)
    {
        arr->Add(wxString(ptr, wxConvUTF8));
        ptr += strlen(ptr) + 1;
    }

    return arr;
}



DbKey DbOrig::Read(const wxString& str)
{
    const wxWX2MBbuf c_str_buf = str.mb_str();
    const char *c_str = c_str_buf;
    DBT key, data;

    memset(&key, 0, sizeof(key));
    memset(&data, 0, sizeof(data));
    key.data = (void*)c_str;
    key.size = strlen(c_str);
    m_err = m_db->get(m_db, NULL, &key, &data, 0);
    if (m_err != 0)
    {
        if (m_err == DB_NOTFOUND)
            m_err = 0;
        else
            LogError();
        return DBKEY_ILLEGAL;
    }
    
    return *((DbKey*)data.data);
}

bool DbOrig::Write(const wxString& str, DbKey value)
{
    const wxWX2MBbuf c_str_buf = str.mb_str();
    const char *c_str = c_str_buf;
    DBT key, data;

    memset(&key, 0, sizeof(key));
    memset(&data, 0, sizeof(data));
    key.data = (void*) c_str;
    key.size = strlen(c_str);
    data.data = &value;
    data.size = sizeof(value);

    m_err = m_db->put(m_db, NULL, &key, &data, 0);
    if (m_err != 0)
    {
        LogError();
        return false;
    }

    return true;
}



DbKeys *DbWords::Read(const wxString& word, unsigned sentenceSize)
{
    const wxWX2MBbuf word_mb = word.mb_str();
    char *keyBuf;
    size_t keyLen;
    DBT key, data;

    keyLen = strlen(word_mb) + sizeof(wxUint32);
    keyBuf = new char[keyLen+1];
    strcpy(keyBuf + sizeof(wxUint32), word_mb);
    *((wxUint32*)(keyBuf)) = sentenceSize;
    memset(&key, 0, sizeof(key));
    memset(&data, 0, sizeof(data));
    key.data = keyBuf;
    key.size = keyLen;
    m_err = m_db->get(m_db, NULL, &key, &data, 0);
    delete[] keyBuf;

    if (m_err != 0)
    {
        if (m_err == DB_NOTFOUND)
            m_err = 0;
        else
            LogError();
        return NULL;
    }    

    DbKeys *result = new DbKeys(data);
    
    return result;
}

bool DbWords::Append(const wxString& word, unsigned sentenceSize, DbKey value)
{
    // VS: there is a dirty trick: it is always true that 'value' is 
    //     greater than all values already present in the db, so we may
    //     append it to the end of list while still keeping the list sorted.
    //     This is important because it allows us to efficiently merge
    //     these lists when looking up inexact translations...

    const wxWX2MBbuf word_mb = word.mb_str();
    DbKey *valueBuf;
    char *keyBuf;
    size_t keyLen;
    DBT key, data;

    keyLen = strlen(word_mb) + sizeof(wxUint32);
    keyBuf = new char[keyLen+1];
    strcpy(keyBuf + sizeof(wxUint32), word_mb);
    *((wxUint32*)(keyBuf)) = sentenceSize;

    memset(&key, 0, sizeof(key));
    memset(&data, 0, sizeof(data));
    key.data = keyBuf;
    key.size = keyLen;

    DbKeys *keys = Read(word, sentenceSize);
    if (keys == NULL)
    {
        valueBuf = NULL;
        data.data = &value;
        data.size = sizeof(DbKey);
    }
    else
    {
        valueBuf = new DbKey[keys->Count + 1];
        memcpy(valueBuf, keys->List, keys->Count * sizeof(DbKey));
        valueBuf[keys->Count] = value;
        data.data = valueBuf;
        data.size = (keys->Count + 1) * sizeof(DbKey);
    }
    
    m_err = m_db->put(m_db, NULL, &key, &data, 0);

    delete[] keyBuf;
    delete[] valueBuf;

    if (m_err != 0)
    {
        LogError();
        return false;
    }

    return true;
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


static inline wxString GetDBPath(const wxString& p, const wxString& l)
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





#include <wx/listimpl.cpp>
WX_DEFINE_LIST(TranslationMemoriesList);
TranslationMemoriesList TranslationMemory::ms_instances;

/*static*/ TranslationMemory *TranslationMemory::Create(
                      const wxString& language, const wxString& path)
{
    wxString dbPath = GetDBPath(path, language);
    if (!dbPath)
        dbPath = path + _T("/") + language;

    if ((!wxDirExists(path) && !wxMkdir(path)) ||
        (!wxDirExists(dbPath) && !wxMkdir(dbPath)))
    {
        wxLogError(_("Cannot create database directory!"));
        return NULL;
    }
    dbPath += _T('/');
    
    for (TranslationMemoriesList::Node *node = ms_instances.GetFirst(); 
         node; node = node->GetNext())
    {
        TranslationMemory *mem = node->GetData();
        if (mem->m_lang == language && mem->m_dbPath == dbPath)
        {
            mem->m_refCnt++;
            return mem;
        }
    }

#ifdef __WINDOWS__
    if (!g_triedLibraryDB)
    {
        g_libraryDB = new wxDynamicLibrary(_T("libdb31.dll"));
        if (!g_libraryDB->IsLoaded()) 
        {
            delete g_libraryDB;
            g_libraryDB = NULL;
        }
        else
        {
            g_db_create = (db_create_t)
                    g_libraryDB->GetSymbol(_T("db_create"));
            g_db_strerror = (db_strerror_t)
                    g_libraryDB->GetSymbol(_T("db_strerror"));
            if (!g_db_create || !g_db_strerror)
            {
                delete g_libraryDB;
                g_libraryDB = NULL;
            }
        }
        g_triedLibraryDB = true;
    }
    if (!g_libraryDB) return NULL;
#endif

    TranslationMemory *tm = new TranslationMemory(language, dbPath);
    if (!tm->m_dbTrans->IsOk() || !tm->m_dbOrig->IsOk() || 
        !tm->m_dbWords->IsOk())
    {
        delete tm;
        return NULL;
    }
    else
        return tm;
}

TranslationMemory::TranslationMemory(const wxString& language, 
                                     const wxString& dbPath)
{
    m_dbPath = dbPath;
    m_refCnt = 1;
    m_lang = language;
    SetParams(2, 2);
    m_dbTrans = new DbTrans(dbPath);
    m_dbOrig = new DbOrig(dbPath);
    m_dbWords = new DbWords(dbPath);
}                                     

TranslationMemory::~TranslationMemory()
{
    delete m_dbTrans;
    delete m_dbOrig;
    delete m_dbWords;
}

/*static*/ bool TranslationMemory::IsSupported(const wxString& lang,
                                               const wxString& path)
{
    return !!GetDBPath(path, lang);
}

bool TranslationMemory::Store(const wxString& string, 
                              const wxString& translation)
{
    DbKey key;
    bool ok;
    
    key = m_dbOrig->Read(string);
    if (key == DBKEY_ILLEGAL)
    {
        key = m_dbTrans->Write(translation);
        ok = (key != DBKEY_ILLEGAL) && m_dbOrig->Write(string, key);
        if (ok)
        {
            wxArrayString words;
            StringToWordsArray(string, words);
            size_t sz = words.GetCount();
            for (size_t i = 0; i < sz; i++)
                m_dbWords->Append(words[i], sz, key);
        }
        return ok;
    }
    else
    {
        wxArrayString *t = m_dbTrans->Read(key);
        if (t->Index(translation) == wxNOT_FOUND)
        {
            t->Add(translation);
            DbKey key2 = m_dbTrans->Write(t, key);
            ok = (key2 != DBKEY_ILLEGAL);
        }
        else
            ok = true;
        delete t;
        return ok;
    }
}


int TranslationMemory::Lookup(const wxString& string, wxArrayString& results)
{
    results.Clear();

    // First of all, try exact match:
    DbKey key = m_dbOrig->Read(string);
    if (key != DBKEY_ILLEGAL)
    {
        wxArrayString *a = m_dbTrans->Read(key);
        WX_APPEND_ARRAY(results, (*a));
        delete a;
        return 100;
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
            wxArrayString *a;
            for (i = 0; i < result->Count; i++)
            {
                a = m_dbTrans->Read(result->List[i]);
                WX_APPEND_ARRAY(results, (*a));
                delete a;
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
                wxArrayString *a;
                for (i = 0; i < result->Count; i++)
                {
                    a = m_dbTrans->Read(result->List[i]);
                    WX_APPEND_ARRAY(results, (*a));
                    delete a;
                }
                delete result;
                RETURN_WITH_CLEANUP(true)
            }
            
            if (!AdvanceCycle(omitted, depth, cnt)) break;
        }
    }

    RETURN_WITH_CLEANUP(false)

    #undef RETURN_WITH_CLEANUP
}

#endif // USE_TRANSMEM

