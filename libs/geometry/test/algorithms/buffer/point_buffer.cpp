// Boost.Geometry (aka GGL, Generic Geometry Library)
// Unit Test

// Copyright (c) 2012-2014 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//#define BOOST_GEOMETRY_DEBUG_WITH_MAPPER
//#define BOOST_GEOMETRY_DEBUG_ASSEMBLE
//#define BOOST_GEOMETRY_DEBUG_IDENTIFIER

#include <geometry_test_common.hpp>

#include <boost/geometry/algorithms/buffer.hpp>
#include <boost/geometry/core/coordinate_type.hpp>

#include <boost/geometry/strategies/strategies.hpp>

#include <boost/geometry/geometries/point.hpp>
#include <boost/geometry/geometries/box.hpp>

#include <test_buffer.hpp>


static std::string const simplex = "POINT(5 5)";

template <typename P>
void test_all()
{
    typedef bg::model::polygon<P> polygon;

    bg::strategy::buffer::join_miter join_miter;
    bg::strategy::buffer::end_flat end_flat;

    double const pi = boost::geometry::math::pi<double>();

    test_one<P, polygon>("simplex1", simplex, join_miter, end_flat, pi, 1.0, 1.0);
    test_one<P, polygon>("simplex2", simplex, join_miter, end_flat, pi * 4.0, 2.0, 2.0, true, 0.1);
    test_one<P, polygon>("simplex3", simplex, join_miter, end_flat, pi * 9.0, 3.0, 3.0, true, 0.1);
}


int test_main(int, char* [])
{
    //std::cout << std::setprecision(6);
    //test_all<bg::model::point<float, 2, bg::cs::cartesian> >();
    test_all<bg::model::point<double, 2, bg::cs::cartesian> >();
    return 0;
}
