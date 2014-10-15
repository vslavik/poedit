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

struct IntWrapper
{
  int _i;
  IntWrapper(int i) : _i(i) {}
  bool operator==(IntWrapper const& rhs) const { return _i == rhs._i; }
};

template <typename T>
void test_function_value_or_for()
{
  optional<T> oM0;
  const optional<T> oC0;
  optional<T> oM1(1);
  const optional<T> oC2(2);
  
  BOOST_CHECK(oM0.value_or(5) == 5);
  BOOST_CHECK(oC0.value_or(5) == 5);
  BOOST_CHECK(oM1.value_or(5) == 1);
  BOOST_CHECK(oC2.value_or(5) == 2);
}

template <typename T>
void test_function_value_for()
{
  optional<T> o0;
  optional<T> o1(1);
  const optional<T> oC(2);
  
  try
  {
    T& v = o1.value();
    BOOST_CHECK(v == 1);
  }
  catch(...)
  {
    BOOST_CHECK(false);
  }
  
  try
  {
    T const& v = oC.value();
    BOOST_CHECK(v == 2);
  }
  catch(...)
  {
    BOOST_CHECK(false);
  }
  
  try
  {
    T& v = o0.value();
    BOOST_CHECK(false);
    boost::ignore_unused(v);
  }
  catch(boost::bad_optional_access const&)
  {
  }
  catch(...)
  {
    BOOST_CHECK(false);
  }
}

void test_function_value()
{
    test_function_value_for<int>();
    test_function_value_for<double>();
    test_function_value_for<IntWrapper>();
}

struct FatToIntConverter
{
  static int conversions;
  int _val;
  FatToIntConverter(int val) : _val(val) {}
  operator int() const { conversions += 1; return _val; }
};

int FatToIntConverter::conversions = 0;

void test_function_value_or()
{
    test_function_value_or_for<int>();
    test_function_value_or_for<double>();
    test_function_value_or_for<IntWrapper>();
    
    optional<int> oi(1);
    BOOST_CHECK(oi.value_or(FatToIntConverter(2)) == 1);
    BOOST_CHECK(FatToIntConverter::conversions == 0);
    
    oi = boost::none;
    BOOST_CHECK(oi.value_or(FatToIntConverter(2)) == 2);
    BOOST_CHECK(FatToIntConverter::conversions == 1);
}


struct FunM
{
    int operator()() { return 5; }
};

struct FunC
{
    int operator()() const { return 6; }
};

int funP ()
{
    return 7;
}

int throw_()
{
    throw int();
}

void test_function_value_or_eval()
{
    optional<int> o1 = 1;
    optional<int> oN;
    FunM funM;
    FunC funC;
    
    BOOST_CHECK(o1.value_or_eval(funM) == 1);
    BOOST_CHECK(oN.value_or_eval(funM) == 5);
    BOOST_CHECK(o1.value_or_eval(FunM()) == 1);
    BOOST_CHECK(oN.value_or_eval(FunM()) == 5);
    
    BOOST_CHECK(o1.value_or_eval(funC) == 1);
    BOOST_CHECK(oN.value_or_eval(funC) == 6);
    BOOST_CHECK(o1.value_or_eval(FunC()) == 1);
    BOOST_CHECK(oN.value_or_eval(FunC()) == 6);
    
    BOOST_CHECK(o1.value_or_eval(funP) == 1);
    BOOST_CHECK(oN.value_or_eval(funP) == 7);
    
#ifndef BOOST_NO_CXX11_LAMBDAS
    BOOST_CHECK(o1.value_or_eval([](){return 8;}) == 1);
    BOOST_CHECK(oN.value_or_eval([](){return 8;}) == 8);
#endif

    try
    {
      BOOST_CHECK(o1.value_or_eval(throw_) == 1);
    }
    catch(...)
    {
      BOOST_CHECK(false);
    }
    
    try
    {
      BOOST_CHECK(oN.value_or_eval(throw_) == 1);
      BOOST_CHECK(false);
    }
    catch(...)
    {
      BOOST_CHECK(true);
    }
}

const optional<std::string> makeConstOptVal()
{
    return std::string("something");
}

void test_const_move()
{
    std::string s5 = *makeConstOptVal();
    std::string s6 = makeConstOptVal().value();
    boost::ignore_unused(s5);
    boost::ignore_unused(s6);
}


#ifndef BOOST_NO_CXX11_REF_QUALIFIERS
struct MoveOnly
{
    explicit MoveOnly(int){}
    MoveOnly(MoveOnly &&){}
    void operator=(MoveOnly &&);
private:
    MoveOnly(MoveOnly const&);
    void operator=(MoveOnly const&);
};

optional<MoveOnly> makeMoveOnly()
{
    return MoveOnly(1);
}

MoveOnly moveOnlyDefault()
{
    return MoveOnly(1);
}

// compile-time test
void test_move_only_getters()
{
    MoveOnly m1 = *makeMoveOnly();
    MoveOnly m2 = makeMoveOnly().value();
    MoveOnly m3 = makeMoveOnly().value_or(MoveOnly(1));
    MoveOnly m4 = makeMoveOnly().value_or_eval(moveOnlyDefault);
    boost::ignore_unused(m1);
    boost::ignore_unused(m2);
    boost::ignore_unused(m3);
    boost::ignore_unused(m4);
}

#endif // !defined BOOST_NO_CXX11_REF_QUALIFIERS

int test_main( int, char* [] )
{
  try
  {
    test_function_value();
    test_function_value_or();
    test_function_value_or_eval();
    test_const_move();
  }
  catch ( ... )
  {
    BOOST_ERROR("Unexpected Exception caught!");
  }

  return 0;
}


