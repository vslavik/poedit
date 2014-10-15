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
//#include<string>

#define BOOST_ENABLE_ASSERT_HANDLER

//#include "boost/bind/apply.hpp" // Included just to test proper interaction with boost::apply<> as reported by Daniel Wallin
#include "boost/mpl/bool.hpp"
#include "boost/mpl/bool_fwd.hpp"  // For mpl::true_ and mpl::false_
#include "boost/static_assert.hpp"

#include "boost/optional/optional.hpp"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#include "boost/test/minimal.hpp"
#include "optional_test_common.cpp"


const int global_i = 0;

class TestingReferenceBinding
{
public:
    TestingReferenceBinding(const int& ii)
    {
        BOOST_CHECK(&ii == &global_i);
    }
    
    void operator=(const int& ii) 
    {
        BOOST_CHECK(&ii == &global_i);
    }
    #ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
    void operator=(int&&) 
    { 
        BOOST_CHECK(false);
    }
    #endif
};

class TestingReferenceBinding2 // same definition as above, I need a different type
{
public:
    TestingReferenceBinding2(const int& ii)
    {
        BOOST_CHECK(&ii == &global_i);
    }
    
    void operator=(const int& ii) 
    {
        BOOST_CHECK(&ii == &global_i);
    }
    #ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
    void operator=(int&&) 
    { 
        BOOST_CHECK(false);
    }
    #endif
};


void test_broken_compiler()
{
// we are not testing boost::optional here, but the c++ compiler
// if this test fails, optional references will obviously fail too

  const int& iref = global_i;
  BOOST_CHECK(&iref == &global_i);
  
  TestingReferenceBinding ttt = global_i;
  ttt = global_i;
  
  TestingReferenceBinding2 ttt2 = iref;
  ttt2 = iref;
}


int test_main( int, char* [] )
{
  try
  {
    test_broken_compiler();
  }
  catch ( ... )
  {
    BOOST_ERROR("Unexpected Exception caught!");
  }

  return 0;
}


