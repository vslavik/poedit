//
// Test boost::polymorphic_cast, boost::polymorphic_downcast
//
// Copyright 1999 Beman Dawes
// Copyright 1999 Dave Abrahams
// Copyright 2014 Peter Dimov
//
// Distributed under the Boost Software License, Version 1.0.
//
// See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt
//

#define BOOST_ENABLE_ASSERT_HANDLER
#include <boost/polymorphic_cast.hpp>
#include <boost/core/lightweight_test.hpp>
#include <string>

static bool expect_assertion = false;
static int assertion_failed_count = 0;

// BOOST_ASSERT custom handler
void boost::assertion_failed( char const * expr, char const * function, char const * file, long line )
{
    if( expect_assertion )
    {
        ++assertion_failed_count;
    }
    else
    {
        BOOST_ERROR( "unexpected assertion" );

        BOOST_LIGHTWEIGHT_TEST_OSTREAM
          << file << "(" << line << "): assertion '" << expr << "' failed in function '"
          << function << "'" << std::endl;
    }
}

//

struct Base
{
    virtual ~Base() {}
    virtual std::string kind() { return "Base"; }
};

struct Base2
{
    virtual ~Base2() {}
    virtual std::string kind2() { return "Base2"; }
};

struct Derived : public Base, Base2
{
    virtual std::string kind() { return "Derived"; }
};

static void test_polymorphic_cast()
{
    Base * base = new Derived;

    Derived * derived;
    
    try
    {
        derived = boost::polymorphic_cast<Derived*>( base );

        BOOST_TEST( derived != 0 );

        if( derived != 0 )
        {
            BOOST_TEST_EQ( derived->kind(), "Derived" );
        }
    }
    catch( std::bad_cast const& )
    {
        BOOST_ERROR( "boost::polymorphic_cast<Derived*>( base ) threw std::bad_cast" );
    }

    Base2 * base2;

    try
    {
        base2 = boost::polymorphic_cast<Base2*>( base ); // crosscast

        BOOST_TEST( base2 != 0 );

        if( base2 != 0 )
        {
            BOOST_TEST_EQ( base2->kind2(), "Base2" );
        }
    }
    catch( std::bad_cast const& )
    {
        BOOST_ERROR( "boost::polymorphic_cast<Base2*>( base ) threw std::bad_cast" );
    }

    delete base;
}

static void test_polymorphic_downcast()
{
    Base * base = new Derived;

    Derived * derived = boost::polymorphic_downcast<Derived*>( base );

    BOOST_TEST( derived != 0 );

    if( derived != 0 )
    {
        BOOST_TEST_EQ( derived->kind(), "Derived" );
    }

    // polymorphic_downcast can't do crosscasts

    delete base;
}

static void test_polymorphic_cast_fail()
{
    Base * base = new Base;

    BOOST_TEST_THROWS( boost::polymorphic_cast<Derived*>( base ), std::bad_cast );

    delete base;
}

static void test_polymorphic_downcast_fail()
{
    Base * base = new Base;

    int old_count = assertion_failed_count;
    expect_assertion = true;

    boost::polymorphic_downcast<Derived*>( base ); // should assert

    BOOST_TEST_EQ( assertion_failed_count, old_count + 1 );
    expect_assertion = false;

    delete base;
}

int main()
{
    test_polymorphic_cast();
    test_polymorphic_downcast();
    test_polymorphic_cast_fail();
    test_polymorphic_downcast_fail();

    return boost::report_errors();
}
