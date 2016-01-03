/*=============================================================================
    Copyright (c) 2001-2012 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

// this file deliberately contains non-ascii characters
// boostinspect:noascii

#include <boost/detail/lightweight_test.hpp>
#include <boost/spirit/home/x3.hpp>
//~ #include <boost/phoenix/core.hpp>
//~ #include <boost/phoenix/operator.hpp>
//~ #include <boost/phoenix/object.hpp>
//~ #include <boost/phoenix/bind.hpp>
//~ #include <boost/fusion/include/std_pair.hpp>

#include <string>
#include <cstring>
#include <iostream>
#include "test.hpp"

using boost::spirit::x3::_val;

struct f
{
    template <typename Context>
    void operator()(Context const& ctx) const
    {
        _val(ctx) += _attr(ctx);
    }
};

int
main()
{
    using spirit_test::test_attr;
    using spirit_test::test;

    using namespace boost::spirit::x3::ascii;
    //~ using boost::spirit::x3::locals;
    using boost::spirit::x3::rule;
    using boost::spirit::x3::int_;
    //~ using boost::spirit::x3::uint_;
    //~ using boost::spirit::x3::fail;
    //~ using boost::spirit::x3::on_error;
    //~ using boost::spirit::x3::debug;
    using boost::spirit::x3::lit;
    //~ using boost::spirit::x3::_val;
    //~ using boost::spirit::x3::_1;
    //~ using boost::spirit::x3::_r1;
    //~ using boost::spirit::x3::_r2;
    //~ using boost::spirit::x3::_a;

    //~ namespace phx = boost::phoenix;

    { // synth attribute value-init

        std::string s;
        typedef rule<class r, std::string> rule_type;

        auto rdef = rule_type()
            = alpha                 [f()]
            ;

        BOOST_TEST(test_attr("abcdef", +rdef, s));
        BOOST_TEST(s == "abcdef");
    }

    { // synth attribute value-init

        std::string s;
        typedef rule<class r, std::string> rule_type;
       
        auto rdef = rule_type() =
            alpha /
               [](auto& ctx)
               {
                  _val(ctx) += _attr(ctx);
               }
            ;

        BOOST_TEST(test_attr("abcdef", +rdef, s));
        BOOST_TEST(s == "abcdef");
    }

    // $$$ Not yet implemented $$$
    //~ { // context (w/arg) tests

        //~ char ch;
        //~ rule<char const*, char(int)> a; // 1 arg
        //~ a = alpha[_val = _1 + _r1];

        //~ BOOST_TEST(test("x", a(phx::val(1))[phx::ref(ch) = _1]));
        //~ BOOST_TEST(ch == 'x' + 1);

        //~ BOOST_TEST(test_attr("a", a(1), ch)); // allow scalars as rule args too.
        //~ BOOST_TEST(ch == 'a' + 1);

        //~ rule<char const*, char(int, int)> b; // 2 args
        //~ b = alpha[_val = _1 + _r1 + _r2];
        //~ BOOST_TEST(test_attr("a", b(1, 2), ch));
        //~ BOOST_TEST(ch == 'a' + 1 + 2);
    //~ }

    // $$$ Not yet implemented $$$
    //~ { // context (w/ reference arg) tests

        //~ char ch;
        //~ rule<char const*, void(char&)> a; // 1 arg (reference)
        //~ a = alpha[_r1 = _1];

        //~ BOOST_TEST(test("x", a(phx::ref(ch))));
        //~ BOOST_TEST(ch == 'x');
    //~ }

    // $$$ Not yet implemented $$$
    //~ { // context (w/locals) tests

        //~ rule<char const*, locals<char> > a; // 1 local
        //~ a = alpha[_a = _1] >> char_(_a);
        //~ BOOST_TEST(test("aa", a));
        //~ BOOST_TEST(!test("ax", a));
    //~ }

    // $$$ Not yet implemented $$$
    //~ { // context (w/args and locals) tests

        //~ rule<char const*, void(int), locals<char> > a; // 1 arg + 1 local
        //~ a = alpha[_a = _1 + _r1] >> char_(_a);
        //~ BOOST_TEST(test("ab", a(phx::val(1))));
        //~ BOOST_TEST(test("xy", a(phx::val(1))));
        //~ BOOST_TEST(!test("ax", a(phx::val(1))));
    //~ }

    // $$$ No longer relevant $$$
    //~ { // void() has unused type (void == unused_type)

        //~ std::pair<int, char> attr;
        //~ rule<char const*, void()> r;
        //~ r = char_;
        //~ BOOST_TEST(test_attr("123ax", int_ >> char_ >> r, attr));
        //~ BOOST_TEST(attr.first == 123);
        //~ BOOST_TEST(attr.second == 'a');
    //~ }

    // $$$ Not yet implemented $$$
    //~ { // bug: test that injected attributes are ok

        //~ rule<char const*, char(int) > r;

        //~ // problem code:
        //~ r = char_(_r1)[_val = _1];
    //~ }

    return boost::report_errors();
}

