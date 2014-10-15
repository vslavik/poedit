// Boost.Geometry (aka GGL, Generic Geometry Library)
// Unit Test

// Copyright (c) 2012-2014 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <test_buffer.hpp>

#include <boost/geometry/multi/geometries/multi_geometries.hpp>

static std::string const simplex = "MULTILINESTRING((0 0,4 5),(5 4,10 0))";
static std::string const two_bends = "MULTILINESTRING((0 0,4 5,7 4,10 6),(1 5,5 9,8 6))";
static std::string const turn_inside = "MULTILINESTRING((0 0,4 5,7 4,10 6),(1 5,5 9,8 6),(0 4,-2 6))";


template <typename P>
void test_all()
{
    typedef bg::model::linestring<P> linestring;
    typedef bg::model::multi_linestring<linestring> multi_linestring_type;
    typedef bg::model::polygon<P> polygon;

    bg::strategy::buffer::join_miter join_miter;
    bg::strategy::buffer::join_round join_round(100);
    bg::strategy::buffer::join_round_by_divide join_round_by_divide(4);
    bg::strategy::buffer::end_flat end_flat;
    bg::strategy::buffer::end_round end_round(100);

    // Round joins / round ends
    test_one<multi_linestring_type, polygon>("simplex", simplex, join_round, end_round, 49.0217, 1.5, 1.5);
    test_one<multi_linestring_type, polygon>("two_bends", two_bends, join_round, end_round, 74.73, 1.5, 1.5);
    test_one<multi_linestring_type, polygon>("turn_inside", turn_inside, join_round, end_round, 86.3313, 1.5, 1.5);
    test_one<multi_linestring_type, polygon>("two_bends_asym", two_bends, join_round, end_round, 58.3395, 1.5, 0.75);

    // Round joins / flat ends:
    test_one<multi_linestring_type, polygon>("simplex", simplex, join_round, end_flat, 38.2623, 1.5, 1.5);
    test_one<multi_linestring_type, polygon>("two_bends", two_bends, join_round, end_flat, 64.6217, 1.5, 1.5);

    // TODO this should be fixed test_one<multi_linestring_type, polygon>("turn_inside", turn_inside, join_round, end_flat, 99, 1.5, 1.5);
    test_one<multi_linestring_type, polygon>("two_bends_asym", two_bends, join_round, end_flat, 52.3793, 1.5, 0.75);

    // This one is far from done:
    // test_one<multi_linestring_type, polygon>("turn_inside_asym_neg", turn_inside, join_round, end_flat, 99, +1.5, -1.0);

    // Miter / divide joins, various ends
    test_one<multi_linestring_type, polygon>("two_bends", two_bends, join_round_by_divide, end_flat, 64.6217, 1.5, 1.5);
    test_one<multi_linestring_type, polygon>("two_bends", two_bends, join_miter, end_flat, 65.1834, 1.5, 1.5);
    test_one<multi_linestring_type, polygon>("two_bends", two_bends, join_miter, end_round, 75.2917, 1.5, 1.5);
}



int test_main(int, char* [])
{
    test_all<bg::model::point<double, 2, bg::cs::cartesian> >();

    return 0;
}
