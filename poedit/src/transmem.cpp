
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

/** \page db_desc Translation Memory Algorithms

    \todo Describe how the algorithm works!!!
 */

#include <wx/wxprec.h>
#include <wx/string.h>
#include <wx/tokenzr.h>
#include <wx/log.h>
#include <wx/intl.h>
#include <wx/utils.h>

#include <db3/db.h>
// FIXME - use db.h, not db3/db.h

#include "transmem.h"

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
        DbTrans(const wxString& path) : DbBase(path + "translations.db", DB_RECNO) {}

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
        DbOrig(const wxString& path) : DbBase(path + "strings.db", DB_HASH) {}
        
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
        DbWords(const wxString& path) : DbBase(path + "words.db", DB_HASH) {}
        
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
    m_err = db_create(&m_db, NULL, 0);
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
    wxLogError(_("Database error: %s"), db_strerror(m_err));
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
        bufLen += strs->Item(i).Len() + 1;
    buf = new char[bufLen];
    for (ptr = buf, i = 0; i < strs->GetCount(); i++)
    {
        strcpy(ptr, strs->Item(i).c_str());
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

#ifdef DEBUG_TM
    fprintf(stderr,"DbTrans write: index=%i data=[", index);
    for (i = 0; i < strs->GetCount(); i++)
        fprintf(stderr,"'%s',", strs->Item(i).c_str());
    fprintf(stderr,"]\n");
#endif

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
        arr->Add(ptr);
        ptr += strlen(ptr) + 1;
    }

#ifdef DEBUG_TM
    fprintf(stderr,"DbTrans read(index=%i): data=[", index);
    for (size_t i = 0; i < arr->GetCount(); i++)
        fprintf(stderr,"'%s',", arr->Item(i).c_str());
    fprintf(stderr,"]\n");
#endif

    return arr;
}



DbKey DbOrig::Read(const wxString& str)
{
    const char *c_str = str.c_str();
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
    
#ifdef DEBUG_TM
    fprintf(stderr,"DbOrig read(str=%s): %i\n", c_str, *((DbKey*)data.data));
#endif
    
    return *((DbKey*)data.data);
}

bool DbOrig::Write(const wxString& str, DbKey value)
{
    const char *c_str = str.c_str();
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

#ifdef DEBUG_TM
    fprintf(stderr,"DbOrig write: str=%s, value=%i\n", c_str, value);
#endif

    return true;
}



DbKeys *DbWords::Read(const wxString& word, unsigned sentenceSize)
{
    char *keyBuf;
    size_t keyLen;
    DBT key, data;

    keyLen = word.Len() + sizeof(wxUint32);
    keyBuf = new char[keyLen+1];
    strcpy(keyBuf + sizeof(wxUint32), word.c_str());
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
#ifdef DEBUG_TM
    fprintf(stderr,"DbWords read(%s/%i): [", word.c_str(), sentenceSize);
    for (size_t i = 0; i < result->Count; i++)
        fprintf(stderr,"%i, ", result->List[i]);
    fprintf(stderr,"]\n");
#endif
    
    return result;
}

bool DbWords::Append(const wxString& word, unsigned sentenceSize, DbKey value)
{
    // VS: there is a dirty trick: it is always true that 'value' is 
    //     greater than all values already present in the db, so we may
    //     append it to the end of list while still keeping the list sorted.
    //     This is important because it allows us to efficiently merge
    //     these lists when looking up inexact translations...
    DbKey *valueBuf;
    char *keyBuf;
    size_t keyLen;
    DBT key, data;

    keyLen = word.Len() + sizeof(wxUint32);
    keyBuf = new char[keyLen+1];
    strcpy(keyBuf + sizeof(wxUint32), word.c_str());
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

#ifdef DEBUG_TM
    if (m_err == 0)
    {
    fprintf(stderr,"DbWords write(%s/%i): [", word.c_str(), sentenceSize);
    for (size_t i = 0; i < data.size / sizeof(DbKey); i++)
        fprintf(stderr,"%i, ", ((DbKey*)(data.data))[i]);
    fprintf(stderr,"]\n");
    }
#endif

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

#define WORD_SEPARATORS " \t\r\n\\~`!@#$%^&*()-_=+|[]{};:'\"<>,./?"

/// Extracts list of words from \a string.
static void StringToWordsArray(const wxString& str, wxArrayString& array)
{
    static wxSortedArrayString *BadWords = NULL;
    
    if (BadWords == NULL)
    {
        // some words are so common in English we would be crazy to put
        // them into index. (The list was taken from ht://Dig.)
        BadWords = new wxSortedArrayString;
        BadWords->Add("a");
        //BadWords->Add("all");
        BadWords->Add("an");
        //BadWords->Add("are");
        //BadWords->Add("can");
        //BadWords->Add("for");
        //BadWords->Add("from");
        BadWords->Add("have");
        //BadWords->Add("it");
        //BadWords->Add("may");
        //BadWords->Add("not");
        BadWords->Add("of");
        //BadWords->Add("that");
        BadWords->Add("the");
        //BadWords->Add("this");
        //BadWords->Add("was");
        BadWords->Add("will");
        //BadWords->Add("with");
        //BadWords->Add("you");
        //BadWords->Add("your");
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

#ifdef DEBUG_TM
    fprintf(stderr,"StringToWords(%s): data=[", str.c_str());
    for (size_t i = 0; i < array.GetCount(); i++)
        fprintf(stderr,"'%s',", array[i].c_str());
    fprintf(stderr,"]\n");
#endif
}



static DbKeys *UnionOfDbKeys(size_t cnt, DbKeys *keys[], bool mask[])
{
    size_t i, minSize;
    DbKey **heads = new DbKey*[cnt];
    size_t *counters = new size_t[cnt];

#ifdef DEBUG_TM
    fprintf(stderr,"merging keylists:\n");
    for (i = 0; i  <cnt;i++)
    {
        if (!mask[i]) continue;
        fprintf(stderr,"    [");
        for (size_t j = 0; j < keys[i]->Count; j++)
            fprintf(stderr,"%i,", keys[i]->List[j]);
        fprintf(stderr,"]\n");
    }
#endif

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
#ifdef DEBUG_TM
        fprintf(stderr,"  = []\n");
#endif
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
#ifdef DEBUG_TM
        fprintf(stderr," = []\n");
#endif
    }
#ifdef DEBUG_TM
    else
    {
        fprintf(stderr," = [");
        for (i = 0; i < result->Count; i++)
            fprintf(stderr,"%i,", result->List[i]);
        fprintf(stderr,"]\n");
    }
#endif

    return result;
}


static inline wxString GetDBPath(const wxString& p, const wxString& l)
{
    wxString db = p + "/" + l;
    if (wxDirExists(db)) 
        return db;
    if (l.Len() == 5)
    {
        db = p + "/" + l.Mid(0,2);
        if (wxDirExists(db))
            return db;
    }
    if (l.Len() == 2)
    {
        wxString l2 = wxFindFirstFile(p + "/" + l + "_??", wxDIR);
        if (!!l2)
            return l2;
    }
    return wxEmptyString;
}

TranslationMemory::TranslationMemory(const wxString& language, 
                                     const wxString& path)
{
    wxString dbPath = GetDBPath(path, language);
    if (!dbPath)
        dbPath = path + "/" + language;

    if ((!wxDirExists(path) && !wxMkdir(path)) ||
        (!wxDirExists(dbPath) && !wxMkdir(dbPath)))
    {
        wxLogError(_("Cannot create database directory!"));
        return;
    }
    dbPath += '/';
    
    m_lang = language;
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
#ifdef DEBUG_TM
    fprintf(stderr,"###### Lookup querry for '%s'\n", string.c_str());
#endif
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
    const unsigned MAX_OMITS = 2;
    const int      MAX_DELTA = 2;

    wxArrayString words;
    StringToWordsArray(string, words);
    for (unsigned omits = 0; omits <= MAX_OMITS; omits++)
    {
        for (int delta = 0; delta <= MAX_DELTA; delta++)
        {
            if (LookupFuzzy(words, results, omits, delta))
            {
                int score = 
                    (MAX_OMITS - omits) * 100 / (MAX_OMITS + 1) +
                    (MAX_DELTA - delta) * 100 / 
                            ((MAX_DELTA + 1) * (MAX_OMITS + 1));
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
        
#ifdef DEBUG_TM
    fprintf(stderr,"LookupFuzzy omits=%i delta=%i\n", omits, delta);
#endif

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

#ifdef DEBUG_TM
            fprintf(stderr, "trying with mask: [");
            for (i = 0; i < cnt; i++) fprintf(stderr, "%c ", mask[i] ? ' ' : '+');
            fprintf(stderr, "]\n");
#endif
            
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
