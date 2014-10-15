/*=============================================================================
    Copyright (c) 2001-2013 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#include <boost/detail/lightweight_test.hpp>
#include <boost/spirit/home/x3.hpp>

#include <string>

#include "test.hpp"

int
main()
{
    using spirit_test::test_attr;
    using boost::spirit::x3::lit;
    using boost::spirit::x3::char_;

    {
	std::string attr;
	auto p = char_ >> lit("\n");
	BOOST_TEST(test_attr("A\n", p, attr));
	BOOST_TEST(attr == "A");
    }
    return boost::report_errors();
}
