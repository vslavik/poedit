//
// Copyright Antony Polukhin, 2012-2013.
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <boost/test/minimal.hpp>

#include <boost/type_index.hpp>
#include "test_lib.hpp"

#define BOOST_CHECK_EQUAL(x, y) BOOST_CHECK(x == y)
#define BOOST_CHECK_NE(x, y) BOOST_CHECK(x != y)

namespace user_defined_namespace {
    class user_defined{};
}

void comparing_types_between_modules()
{
    boost::typeindex::type_index t_const_int = boost::typeindex::type_id_with_cvr<const int>();
    boost::typeindex::type_index t_int = boost::typeindex::type_id<int>();

    BOOST_CHECK_EQUAL(t_int, test_lib::get_integer());
    BOOST_CHECK_EQUAL(t_const_int, test_lib::get_const_integer());
    BOOST_CHECK_NE(t_const_int, test_lib::get_integer());
    BOOST_CHECK_NE(t_int, test_lib::get_const_integer());


    boost::typeindex::type_index t_const_userdef 
        = boost::typeindex::type_id_with_cvr<const user_defined_namespace::user_defined>();
    boost::typeindex::type_index t_userdef 
        = boost::typeindex::type_id<user_defined_namespace::user_defined>();

    BOOST_CHECK_EQUAL(t_userdef, test_lib::get_user_defined_class());
    BOOST_CHECK_EQUAL(t_const_userdef, test_lib::get_const_user_defined_class());
    BOOST_CHECK_NE(t_const_userdef, test_lib::get_user_defined_class());
    BOOST_CHECK_NE(t_userdef, test_lib::get_const_user_defined_class());


    BOOST_CHECK_NE(t_userdef, test_lib::get_integer());
    BOOST_CHECK_NE(t_const_userdef, test_lib::get_integer());
    BOOST_CHECK_NE(t_int, test_lib::get_user_defined_class());
    BOOST_CHECK_NE(t_const_int, test_lib::get_const_user_defined_class());

    #ifndef BOOST_HAS_PRAGMA_DETECT_MISMATCH
        test_lib::accept_typeindex(t_int);
    #endif
}


int test_main(int , char* []) {
    comparing_types_between_modules();

    return 0;
}

