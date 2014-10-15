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

#include "boost/optional/optional.hpp"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#include "boost/none.hpp"

#include "boost/test/minimal.hpp"

#include "optional_test_common.cpp"

struct SemiRegular // no operator==
{ 
private: void operator==(SemiRegular const&) const {}
private: void operator!=(SemiRegular const&) const {}
};

void test_equal_to_none_of_noncomparable_T()
{
    optional<SemiRegular> i = SemiRegular();
    optional<SemiRegular> o;
    
    BOOST_CHECK(i != boost::none);
    BOOST_CHECK(boost::none != i);
    BOOST_CHECK(o == boost::none);
    BOOST_CHECK(boost::none == o);
}

int test_main( int, char* [] )
{
  try
  {
    test_equal_to_none_of_noncomparable_T();

  }
  catch ( ... )
  {
    BOOST_ERROR("Unexpected Exception caught!");
  }

  return 0;
}


