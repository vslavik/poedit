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

//#ifndef BOOST_OPTIONAL_NO_CONVERTING_ASSIGNMENT
//#ifndef BOOST_OPTIONAL_NO_CONVERTING_COPY_CTOR

#if (!defined BOOST_NO_CXX11_RVALUE_REFERENCES) && (!defined BOOST_NO_CXX11_VARIADIC_TEMPLATES)

class Guard
{
public:
    int which_ctor;
    Guard () : which_ctor(0) { }
    Guard (int&, double&&) : which_ctor(1) { }
    Guard (int&&, double&) : which_ctor(2) { }
    Guard (int&&, double&&) : which_ctor(3) { }
    Guard (int&, double&) : which_ctor(4) { }
    Guard (std::string const&) : which_ctor(5) { }
    Guard (std::string &) : which_ctor(6) { }
    Guard (std::string &&) : which_ctor(7) { }
private:
    Guard(Guard&&);
    Guard(Guard const&);
    void operator=(Guard &&);
    void operator=(Guard const&);
};


void test_emplace()
{
    int i = 0;
    double d = 0.0;
    const std::string cs;
    std::string ms;
    optional<Guard> o;
    
    o.emplace();
    BOOST_CHECK(o);
    BOOST_CHECK(0 == o->which_ctor);
    
    o.emplace(i, 2.0);
    BOOST_CHECK(o);
    BOOST_CHECK(1 == o->which_ctor);
    
    o.emplace(1, d);
    BOOST_CHECK(o);
    BOOST_CHECK(2 == o->which_ctor);
    
    o.emplace(1, 2.0);
    BOOST_CHECK(o);
    BOOST_CHECK(3 == o->which_ctor);
    
    o.emplace(i, d);
    BOOST_CHECK(o);
    BOOST_CHECK(4 == o->which_ctor);
    
    o.emplace(cs);
    BOOST_CHECK(o);
    BOOST_CHECK(5 == o->which_ctor);
    
    o.emplace(ms);
    BOOST_CHECK(o);
    BOOST_CHECK(6 == o->which_ctor);
    
    o.emplace(std::string());
    BOOST_CHECK(o);
    BOOST_CHECK(7 == o->which_ctor);
}


#endif

#if (!defined BOOST_NO_CXX11_RVALUE_REFERENCES)


struct ThrowOnMove
{
    ThrowOnMove(ThrowOnMove&&) { throw int(); }
    ThrowOnMove(ThrowOnMove const&) { throw int(); }
    ThrowOnMove(int){}
};


void test_no_moves_on_emplacement()
{
    try {
        optional<ThrowOnMove> o;
        o.emplace(1);
        BOOST_CHECK(o);
    } 
    catch (...) {
        BOOST_CHECK(false);
    }
}
#endif

struct Thrower
{
    Thrower(bool throw_) { if (throw_) throw int(); }
    
private:
    Thrower(Thrower const&);
#if (!defined BOOST_NO_CXX11_RVALUE_REFERENCES)
    Thrower(Thrower&&);
#endif
};

void test_clear_on_throw()
{
    optional<Thrower> ot;
    try {
        ot.emplace(false);
        BOOST_CHECK(ot);
    } catch(...) {
        BOOST_CHECK(false);
    }
    
    try {
        ot.emplace(true);
        BOOST_CHECK(false);
    } catch(...) {
        BOOST_CHECK(!ot);
    }
}

void test_no_assignment_on_emplacement()
{
    optional<const std::string> os;
    BOOST_CHECK(!os);
    os.emplace("wow");
    BOOST_CHECK(os);
    BOOST_CHECK(*os == "wow");
}

int test_main( int, char* [] )
{
  try
  {
#if (!defined BOOST_NO_CXX11_RVALUE_REFERENCES) && (!defined BOOST_NO_CXX11_VARIADIC_TEMPLATES)
    test_emplace();
#endif
#if (!defined BOOST_NO_CXX11_RVALUE_REFERENCES)
    test_no_moves_on_emplacement();
#endif
    test_clear_on_throw();
    test_no_assignment_on_emplacement();
  }
  catch ( ... )
  {
    BOOST_ERROR("Unexpected Exception caught!");
  }

  return 0;
}


