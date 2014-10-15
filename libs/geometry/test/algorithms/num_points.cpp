// Boost.Geometry (aka GGL, Generic Geometry Library)
// Unit Test

// Copyright (c) 2007-2014 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2014 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2014 Mateusz Loskot, London, UK.
// Copyright (c) 2013-2014 Adam Wulkiewicz, Lodz, Poland.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>

#include <geometry_test_common.hpp>

#include <boost/geometry/algorithms/num_points.hpp>
#include <boost/geometry/multi/algorithms/num_points.hpp>
#include <boost/geometry/geometries/geometries.hpp>
#include <boost/geometry/multi/geometries/multi_geometries.hpp>

#include <boost/geometry/io/wkt/read.hpp>
#include <boost/geometry/multi/io/wkt/read.hpp>

template <typename Geometry>
inline void test_num_points(std::string const& wkt, std::size_t expected)
{
    namespace bg = boost::geometry;
    Geometry geometry;
    boost::geometry::read_wkt(wkt, geometry);
    std::size_t detected = bg::num_points(geometry);
    BOOST_CHECK_EQUAL(expected, detected);
    detected = bg::num_points(geometry, false);
    BOOST_CHECK_EQUAL(expected, detected);
    detected = bg::num_points(geometry, true);
}

int test_main(int, char* [])
{
    typedef bg::model::point<double,2,bg::cs::cartesian> point;
    typedef bg::model::linestring<point> linestring;
    typedef bg::model::segment<point> segment;
    typedef bg::model::box<point> box;
    typedef bg::model::ring<point> ring;
    typedef bg::model::polygon<point> polygon;
    typedef bg::model::multi_point<point> multi_point;
    typedef bg::model::multi_linestring<linestring> multi_linestring;
    typedef bg::model::multi_polygon<polygon> multi_polygon;

    test_num_points<point>("POINT(0 0)", 1u);
    test_num_points<linestring>("LINESTRING(0 0,1 1)", 2u);
    test_num_points<segment>("LINESTRING(0 0,1 1)", 2u);
    test_num_points<box>("POLYGON((0 0,10 10))", 4u);
    test_num_points<ring>("POLYGON((0 0,1 1,0 1,0 0))", 4u);
    test_num_points<polygon>("POLYGON((0 0,10 10,0 10,0 0))", 4u);
    test_num_points<polygon>("POLYGON((0 0,0 10,10 10,10 0,0 0),(4 4,6 4,6 6,4 6,4 4))", 10u);
    test_num_points<multi_point>("MULTIPOINT((0 0),(1 1))", 2u);
    test_num_points<multi_linestring>("MULTILINESTRING((0 0,1 1),(2 2,3 3,4 4))", 5u);
    test_num_points<multi_polygon>("MULTIPOLYGON(((0 0,0 10,10 10,10 0,0 0)),((0 10,1 10,1 9,0 10)))", 9u);

    return 0;
}

