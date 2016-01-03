// Boost.Geometry (aka GGL, Generic Geometry Library)
// Unit Test

// Copyright (c) 2011-2015 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#include <geometry_test_common.hpp>

#include <boost/geometry/strategies/cartesian/side_of_intersection.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/geometries/segment.hpp>


namespace bg = boost::geometry;

int test_main(int, char* [])
{
    typedef bg::model::d2::point_xy<int> point;
    typedef bg::model::segment<point> segment;

    segment a(point(20, 10), point(10, 20));

    segment b1(point(11, 16), point(20, 14));  // IP with a: (14.857, 15.143)
    segment b2(point(10, 16), point(20, 14));  // IP with a: (15, 15)

    segment c1(point(15, 16), point(13, 8));
    segment c2(point(15, 16), point(14, 8));
    segment c3(point(15, 16), point(15, 8));

    typedef bg::strategy::side::side_of_intersection side;

    BOOST_CHECK_EQUAL( 1, side::apply(a, b1, c1));
    BOOST_CHECK_EQUAL(-1, side::apply(a, b1, c2));
    BOOST_CHECK_EQUAL(-1, side::apply(a, b1, c3));

    BOOST_CHECK_EQUAL( 1, side::apply(a, b2, c1));
    BOOST_CHECK_EQUAL( 1, side::apply(a, b2, c2));
    BOOST_CHECK_EQUAL( 0, side::apply(a, b2, c3));

    // Check internal calculation-method:
    BOOST_CHECK_EQUAL(-1400, side::side_value<int>(a, b1, c2));
    BOOST_CHECK_EQUAL( 2800, side::side_value<int>(a, b1, c1));

    BOOST_CHECK_EQUAL (2800, side::side_value<int>(a, b1, c1));
    BOOST_CHECK_EQUAL(-1400, side::side_value<int>(a, b1, c2));
    BOOST_CHECK_EQUAL(-5600, side::side_value<int>(a, b1, c3));

    BOOST_CHECK_EQUAL(12800, side::side_value<int>(a, b2, c1));
    BOOST_CHECK_EQUAL( 6400, side::side_value<int>(a, b2, c2));
    BOOST_CHECK_EQUAL(    0, side::side_value<int>(a, b2, c3));

    // TODO: we might add a check calculating the IP, determining the side
    // with the normal side strategy, and verify the results are equal

    return 0;
}

