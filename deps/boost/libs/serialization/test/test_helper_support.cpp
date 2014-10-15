/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// test_helper_support.cpp

// (C) Copyright 2008 Joaquin M Lopez Munoz. 
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// should pass compilation and execution

#include <algorithm>
#include <cstddef>
#include <fstream>

#include <cstdio> // remove
#include <boost/config.hpp>
#if defined(BOOST_NO_STDC_NAMESPACE)
namespace std{ 
    using ::remove;
}
#endif

#include "test_tools.hpp"
#include <boost/lexical_cast.hpp>
#include <boost/serialization/split_free.hpp>
#include <boost/serialization/vector.hpp>
#include <string>
#include <vector>

class my_string:public std::string
{
    typedef std::string super;

public:
    my_string(){}
    my_string(const super & str): super(str){}
    my_string & operator=(const super& rhs) {
      super::operator=(rhs);
      return *this;
    }
};

struct my_string_helper
{
  typedef std::vector<my_string> table;
  table t;
};

BOOST_SERIALIZATION_SPLIT_FREE(my_string)

namespace boost {
namespace serialization {

template<class Archive>
void save(Archive & ar, const my_string & str, unsigned int /* version */)
{
    typedef my_string_helper::table table;
    table& t = ar.get_helper(static_cast<my_string_helper *>(NULL)).t;

    table::iterator it = std::find(t.begin(), t.end(), str);
    if(it == t.end()){
        table::size_type s = t.size();
        ar << make_nvp("index", s);
        t.push_back(str);
        ar << make_nvp("string", static_cast<const std::string &>(str));
    }
    else{
        table::size_type s = (table::size_type)(it - t.begin());
        ar << make_nvp("index", s);
    }
}

template<class Archive>
void load(Archive & ar, my_string & str, unsigned int /* version */)
{
    typedef my_string_helper::table table;
    table& t = ar.get_helper(static_cast<my_string_helper *>(NULL)).t;

    table::size_type s = 0;
    ar >> make_nvp("index", s);
    t.reserve(s);
    if(s >= t.size()){
        std::string tmp;
        ar >> make_nvp("string", tmp);
        str = tmp;
        t.push_back(str);
    }
    else{
        str = t[s];
    }
}

} // namespace serialization
} // namespace boost

int test_main( int /* argc */, char* /* argv */[] ){
    const char * testfile = boost::archive::tmpnam(NULL);
    BOOST_REQUIRE(NULL != testfile);

    std::vector<my_string> v1;
    for(int i=0; i<1000; ++i){
        v1.push_back(my_string(boost::lexical_cast<std::string>(i % 100)));
    }
    {
        test_ostream os(testfile, TEST_STREAM_FLAGS);
        test_oarchive oa(os, TEST_ARCHIVE_FLAGS);
        oa << boost::serialization::make_nvp("vector", v1);
    }
    {
        std::vector<my_string> v2;
        test_istream is(testfile, TEST_STREAM_FLAGS);
        test_iarchive ia(is, TEST_ARCHIVE_FLAGS);
        ia >> boost::serialization::make_nvp("vector", v2);
        BOOST_CHECK(v1 == v2);
    }
    std::remove(testfile);
    return EXIT_SUCCESS;
}

// EOF
