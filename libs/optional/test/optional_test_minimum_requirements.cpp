// Copyright (C) 2014 Andrzej Krzemienski.
//
// Use, modification, and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/lib/optional for documentation.
//
// You are welcome to contact the author at:
//  akrzemi1@gmail.com
//
// Revisions:
//
#include<iostream>
#include<stdexcept>
#include<string>

#define BOOST_ENABLE_ASSERT_HANDLER

#include "boost/bind/apply.hpp" // Included just to test proper interaction with boost::apply<> as reported by Daniel Wallin
#include "boost/mpl/bool.hpp"
#include "boost/mpl/bool_fwd.hpp"  // For mpl::true_ and mpl::false_
#include "boost/static_assert.hpp"

#include "boost/optional/optional.hpp"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#include "boost/none.hpp"

#include "boost/test/minimal.hpp"

#include "optional_test_common.cpp"

class NonConstructible
{
private:
    NonConstructible();
    NonConstructible(NonConstructible const&);
#if (!defined BOOST_NO_CXX11_RVALUE_REFERENCES)
    NonConstructible(NonConstructible &&);
#endif   
};

void test_non_constructible()
{
    optional<NonConstructible> o;
    BOOST_CHECK(!o);
    BOOST_CHECK(o == boost::none);
    try { 
        o.value();
        BOOST_CHECK(false);
    }
    catch(...) {
        BOOST_CHECK(true);
    }
}

class Guard
{
public:
    explicit Guard(int) {}
private:
    Guard();
    Guard(Guard const&);
#if (!defined BOOST_NO_CXX11_RVALUE_REFERENCES)
    Guard(Guard &&);
#endif   
};

void test_guard()
{
    optional<Guard> o;
    o.emplace(1);
    BOOST_CHECK(o);
}

void test_non_assignable()
{
    optional<const std::string> o;
    o.emplace("cat");
    BOOST_CHECK(o);
    BOOST_CHECK(*o == "cat");
}

int test_main( int, char* [] )
{
  try
  {
    test_non_constructible();
    test_guard();
    test_non_assignable();
  }
  catch ( ... )
  {
    BOOST_ERROR("Unexpected Exception caught!");
  }

  return 0;
}


