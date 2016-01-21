/*
 *  This file is part of Poedit (http://poedit.net)
 *
 *  Copyright (C) 2001-2016 Vaclav Slavik
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

#include <set>
#include <memory>
#include <string>
#include <vector>

#include <string.h>

#ifdef DB_HEADER
    #include DB_HEADER
#else
    #include <db_cxx.h>
#endif

/** \page db_desc Translation Memory Algorithms

    \section tm_def TM Definition
    
    For the purposes of algorithm description, let's say that TM is a 
    database that stores original string-translation pairs (where both 
    original string and translation are strings consisting of words 
    delimited by spaces and/or interpunction) and supports inexact 
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
    DbWords for all words in the sentence (adding new records as necessary).
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
        DbBase(DbEnv *env, const std::string& filename, DBTYPE type);

    protected:
        virtual ~DbBase();

        Db m_db;
};

/// Interface to the database of translations.
class DbTrans : public DbBase
{
    public:
        DbTrans(DbEnv *env, const std::string& path)
            : DbBase(env, path + "translations.db", DB_RECNO) {}

        /** Retrieves translations stored under given \index.
            \return array of translations or NULL if the key is absent.
            \remark The caller must delete returned object
         */
        std::vector<std::string> Read(DbKey index);
};

/// Interface to DB of original strings.
class DbOrig : public DbBase
{
    public:
        DbOrig(DbEnv *env, const std::string& path)
            : DbBase(env, path + "strings.db", DB_HASH) {}
        
        /** Returns index of string \a str or \c DBKEY_ILLEGAL if not found.
            Returned index can be used with DbTrans::Write and DbTrans::Read
         */
        DbKey Read(const char *c_str);

        template<typename Functor>
        void Enumerate(Functor f);
};



DbBase::DbBase(DbEnv *env, const std::string& filename, DBTYPE type)
    : m_db(env, 0)
{
    m_db.open(NULL,
              filename.c_str(),
              NULL,
              type,
              DB_RDONLY | DB_AUTO_COMMIT,
              0);
}


DbBase::~DbBase()
{
    m_db.close(0);
}


std::vector<std::string> DbTrans::Read(DbKey index)
{
    Dbt key(&index, sizeof(index));
    Dbt data;

    if ( m_db.get(NULL, &key, &data, 0) == DB_NOTFOUND )
        return std::vector<std::string>();

    std::vector<std::string> arr;
    char *ptr = (char*)data.get_data();
    char *endptr = ((char*)data.get_data()) + data.get_size();
    while (ptr < endptr)
    {
        arr.push_back(ptr);
        ptr += strlen(ptr) + 1;
    }

    return arr;
}


DbKey DbOrig::Read(const  char *c_str)
{
    Dbt key((void*)c_str, (u_int32_t)strlen(c_str));
    Dbt data;

    if ( m_db.get(NULL, &key, &data, 0) == DB_NOTFOUND )
        return DBKEY_ILLEGAL;

    return *((DbKey*)data.get_data());
}


template<typename Functor>
void DbOrig::Enumerate(Functor f)
{
    Dbc *cursor;
    m_db.cursor(NULL, &cursor, 0);

    Dbt key, value;

    int ret = cursor->get(&key, &value, DB_FIRST);
    while (ret == 0)
    {
        const std::string keys((const char*)key.get_data(), key.get_size());
        f(keys, *((DbKey*)value.get_data()));
        ret = cursor->get(&key, &value, DB_NEXT_NODUP);
    }

    cursor->close();
}



std::unique_ptr<DbEnv> CreateDbEnv(const char *path)
{
    const u_int32_t flags = DB_INIT_MPOOL |
                            DB_INIT_LOCK |
                            DB_INIT_LOG |
                            DB_INIT_TXN |
                            DB_RECOVER |
                            DB_CREATE;

    std::unique_ptr<DbEnv> env(new DbEnv(0));
    env->open(path, flags, 0600);

    // This prevents the log from growing indefinitely
    env->log_set_config(DB_LOG_AUTO_REMOVE, 1);

    return env;
}


void DestroyDbEnv(std::unique_ptr<DbEnv>& env)
{
    const char *dbpath_c;
    env->get_home(&dbpath_c);
    std::string dbpath(dbpath_c);

    env->close(0);

    // Clean up temporary environment files (unless they're still in use):
    DbEnv env2(0);
    env2.remove(dbpath.c_str(), 0);

    env.reset();
}


std::string escape(const std::string& data)
{
    std::string buffer;
    buffer.reserve(data.size());
    for (size_t pos = 0; pos != data.size(); ++pos)
    {
        switch(data[pos])
        {
            case '&':  buffer.append("&amp;");       break;
            case '\"': buffer.append("&quot;");      break;
            case '\'': buffer.append("&apos;");      break;
            case '<':  buffer.append("&lt;");        break;
            case '>':  buffer.append("&gt;");        break;
            default:   buffer.append(1, data[pos]);  break;
        }
    }
    return buffer;
}


void DumpLanguage(DbEnv *env, const char *envpath, const std::string& lang)
{
    try
    {
        std::string path(envpath);
        path += "/" + lang + "/";
        DbOrig orig(env, path);
        DbTrans trans(env, path);

        printf("<language lang=\"%s\">\n", escape(lang).c_str());

        orig.Enumerate([&trans](const std::string& str, DbKey key){
            std::vector<std::string> tr(trans.Read(key));
            for (auto t: tr)
            {
                printf("<i s=\"%s\"\n   t=\"%s\"/>\n", escape(str).c_str(), escape(t).c_str());
            }
        });
        
        printf("</language>\n");
    }
    catch (DbException& e)
    {
        // per-language DB may be missing, this is OK
        if (e.get_errno() == ENOENT)
            return;
        else
            throw;
    }
}


int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        fprintf(stderr, "Usage: %s <path to Poedit's legacy TM> languages (e.g. 'cs:fr:en')...\n", argv[0]);
        return 2;
    }

    try
    {
        std::unique_ptr<DbEnv> env = CreateDbEnv(argv[1]);

        printf("<?xml version=\"1.0\" encoding=\"utf-8\"?>\n");
        printf("<poedit-legacy-tm-export>\n");

        std::string lang;
        for (const char *ptr = argv[2];; ptr++)
        {
            if (*ptr == ':' || *ptr == '\0')
            {
                if (!lang.empty())
                    DumpLanguage(env.get(), argv[1], lang);
                lang.clear();
                if (*ptr == '\0')
                    break;
            }
            else
                lang += *ptr;
        }

        printf("</poedit-legacy-tm-export>\n");

        DestroyDbEnv(env);
    }
    catch (DbException& e)
    {
        fprintf(stderr, "TM Database Error: %s\n", e.what());
        return 1;
    }

    return 0;
}
