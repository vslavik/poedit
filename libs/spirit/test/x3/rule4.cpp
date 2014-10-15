/*=============================================================================
    Copyright (c) 2001-2014 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

// this file deliberately contains non-ascii characters
// boostinspect:noascii

#include <boost/detail/lightweight_test.hpp>
#include <boost/spirit/home/x3.hpp>
#include <boost/fusion/include/vector.hpp>
#include <boost/fusion/include/at.hpp>
//~ #include <boost/fusion/include/std_pair.hpp>

#include <string>
#include <cstring>
#include <iostream>
#include "test.hpp"

namespace x3 = boost::spirit::x3;

int got_it = 0;

struct my_rule_class
{
    template <typename Iterator, typename Exception, typename Context>
    x3::error_handler_result
    on_error(Iterator&, Iterator const& last, Exception const& x, Context const& context)
    {
        std::cout
            << "Error! Expecting: "
            << x.which()
            << ", got: \""
            << std::string(x.where(), last)
            << "\""
            << std::endl
            ;
        return x3::error_handler_result::fail;
    }

    template <typename Iterator, typename Attribute, typename Context>
    inline void
    on_success(Iterator const&, Iterator const&, Attribute&, Context const&)
    {
        ++got_it;
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
    using boost::spirit::x3::lit;

    //~ namespace phx = boost::phoenix;

    { // show that ra = rb and ra %= rb works as expected
        rule<class a, int> ra;
        rule<class b, int> rb;
        int attr;

        auto ra_def = (ra %= int_);
        BOOST_TEST(test_attr("123", ra_def, attr));
        BOOST_TEST(attr == 123);

        auto rb_def = (rb %= ra_def);
        BOOST_TEST(test_attr("123", rb_def, attr));
        BOOST_TEST(attr == 123);

        auto rb_def2 = (rb = ra_def);
        BOOST_TEST(test_attr("123", rb_def2, attr));
        BOOST_TEST(attr == 123);
    }


    { // std::string as container attribute with auto rules

        std::string attr;

        // $$$ Maybe no longer relevant $$$
        //~ rule<char const*, std::string()> text;
        //~ text %= +(!char_(')') >> !char_('>') >> char_);
        //~ BOOST_TEST(test_attr("x", text, attr));
        //~ BOOST_TEST(attr == "x");

        // test deduced auto rule behavior

        auto text = rule<class text, std::string>()
            = +(!char_(')') >> !char_('>') >> char_);

        attr.clear();
        BOOST_TEST(test_attr("x", text, attr));
        BOOST_TEST(attr == "x");
    }

    { // error handling

        auto r = rule<my_rule_class, char const*>()
            = '(' > int_ > ',' > int_ > ')';

        BOOST_TEST(test("(123,456)", r));
        BOOST_TEST(!test("(abc,def)", r));
        BOOST_TEST(!test("(123,456]", r));
        BOOST_TEST(!test("(123;456)", r));
        BOOST_TEST(!test("[123,456]", r));

        BOOST_TEST(got_it == 1);
    }

    // $$$ No longer relevant $$$
// $$$ Do we support rule encoding? $$$
//~ #if BOOST_WORKAROUND(BOOST_MSVC, BOOST_TESTED_AT(1310))
//~ #pragma setlocale("french")
//~ #endif
    //~ { // specifying the encoding

        //~ typedef boost::spirit::char_encoding::iso8859_1 iso8859_1;
        //~ rule<char const*, iso8859_1> r;

        //~ r = no_case['·'];
        //~ BOOST_TEST(test("¡", r));
        //~ r = no_case[char_('·')];
        //~ BOOST_TEST(test("¡", r));

        //~ r = no_case[char_("Â-Ô")];
        //~ BOOST_TEST(test("…", r));
        //~ BOOST_TEST(!test("ˇ", r));

        //~ r = no_case["·¡"];
        //~ BOOST_TEST(test("¡·", r));
        //~ r = no_case[lit("·¡")];
        //~ BOOST_TEST(test("¡·", r));
    //~ }

//~ #if BOOST_WORKAROUND(BOOST_MSVC, BOOST_TESTED_AT(1310))
//~ #pragma setlocale("")
//~ #endif

    {
        typedef boost::variant<double, int> v_type;
        auto r1 = rule<class r1, v_type>()
            = int_;
        v_type v;
        BOOST_TEST(test_attr("1", r1, v) && v.which() == 1 &&
            boost::get<int>(v) == 1);

        typedef boost::optional<int> ov_type;
        auto r2 = rule<class r2, ov_type>()
            = int_;
        ov_type ov;
        BOOST_TEST(test_attr("1", r2, ov) && ov && boost::get<int>(ov) == 1);
    }

    // test handling of single element fusion sequences
    {
        using boost::fusion::vector;
        using boost::fusion::at_c;
        auto r = rule<class r, vector<int>>()
            = int_;

        vector<int> v(0);
        BOOST_TEST(test_attr("1", r, v) && at_c<0>(v) == 1);
    }

    // $$$ This test does not seem to add anything from the previous test above $$$
    //~ {
        //~ using boost::fusion::vector;
        //~ using boost::fusion::at_c;
        //~ rule<const char*, vector<unsigned int>()> r = uint_;

        //~ vector<unsigned int> v(0);
        //~ BOOST_TEST(test_attr("1", r, v) && at_c<0>(v) == 1);
    //~ }

    // $$$ No longer relevant $$$
    //~ {
        //~ using boost::spirit::x3::int_;
        //~ using boost::spirit::x3::_1;
        //~ using boost::spirit::x3::_val;
        //~ using boost::spirit::x3::space;
        //~ using boost::spirit::x3::space_type;

        //~ rule<const char*, int()> r1 = int_;
        //~ rule<const char*, int(), space_type> r2 = int_;

        //~ int i = 0;
        //~ int j = 0;
        //~ BOOST_TEST(test_attr("456", r1[_val = _1], i) && i == 456);
        //~ BOOST_TEST(test_attr("   456", r2[_val = _1], j, space) && j == 456);
    //~ }


    /// $$$ disabling test (can't fix): '_' has unused attribute $$$
    //~ {
        //~ using boost::spirit::x3::lexeme;
        //~ using boost::spirit::x3::alnum;

        //~ auto literal_ = rule<class literal_, std::string>()
            //~ = lexeme[ +(alnum | '_') ];

        //~ std::string attr;
        //~ BOOST_TEST(test_attr("foo_bar", literal_, attr) && attr == "foo_bar");
        //~ std::cout << attr << std::endl;
    //~ }

    { // attribute compatibility test
        using boost::spirit::x3::rule;
        using boost::spirit::x3::int_;

        auto const expr = int_;

        short i;
        BOOST_TEST(test_attr("1", expr, i) && i == 1); // ok

        const rule< class int_rule, int > int_rule( "int_rule" );
        auto const int_rule_def = int_;
        auto const start  = int_rule = int_rule_def;

        short j;
        BOOST_TEST(test_attr("1", start, j) && j == 1); // error
    }

    return boost::report_errors();
}
