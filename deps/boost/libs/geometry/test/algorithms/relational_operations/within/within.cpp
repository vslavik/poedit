// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2013 Adam Wulkiewicz, Lodz, Poland.

// This file was modified by Oracle on 2014.
// Modifications copyright (c) 2014 Oracle and/or its affiliates.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

#include "test_within.hpp"


#include <boost/geometry/geometries/geometries.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/multi/multi.hpp>
#include <boost/geometry/multi/geometries/multi_point.hpp>
#include <boost/geometry/multi/geometries/multi_linestring.hpp>
#include <boost/geometry/multi/geometries/multi_polygon.hpp>

template <typename P>
void test_all()
{
    typedef bg::model::box<P> box_type;

    test_geometry<P, box_type>("POINT(1 1)", "BOX(0 0,2 2)", true);
    test_geometry<P, box_type>("POINT(0 0)", "BOX(0 0,2 2)", false);
    test_geometry<P, box_type>("POINT(2 2)", "BOX(0 0,2 2)", false);
    test_geometry<P, box_type>("POINT(0 1)", "BOX(0 0,2 2)", false);
    test_geometry<P, box_type>("POINT(1 0)", "BOX(0 0,2 2)", false);

    test_geometry<box_type, box_type>("BOX(1 1,2 2)", "BOX(0 0,3 3)", true);
    test_geometry<box_type, box_type>("BOX(0 0,3 3)", "BOX(1 1,2 2)", false);

    test_geometry<box_type, box_type>("BOX(1 1,3 3)", "BOX(0 0,3 3)", true);
    test_geometry<box_type, box_type>("BOX(3 1,3 3)", "BOX(0 0,3 3)", false);

    /*
    test_within_code<P, box_type>("POINT(1 1)", "BOX(0 0,2 2)", 1);
    test_within_code<P, box_type>("POINT(1 0)", "BOX(0 0,2 2)", 0);
    test_within_code<P, box_type>("POINT(0 1)", "BOX(0 0,2 2)", 0);
    test_within_code<P, box_type>("POINT(0 3)", "BOX(0 0,2 2)", -1);
    test_within_code<P, box_type>("POINT(3 3)", "BOX(0 0,2 2)", -1);

    test_within_code<box_type, box_type>("BOX(1 1,2 2)", "BOX(0 0,3 3)", 1);
    test_within_code<box_type, box_type>("BOX(0 1,2 2)", "BOX(0 0,3 3)", 0);
    test_within_code<box_type, box_type>("BOX(1 0,2 2)", "BOX(0 0,3 3)", 0);
    test_within_code<box_type, box_type>("BOX(1 1,2 3)", "BOX(0 0,3 3)", 0);
    test_within_code<box_type, box_type>("BOX(1 1,3 2)", "BOX(0 0,3 3)", 0);
    test_within_code<box_type, box_type>("BOX(1 1,3 4)", "BOX(0 0,3 3)", -1);
    */
}

template <typename Point>
void test_spherical()
{
    // Test spherical boxes
    // See also http://www.gcmap.com/mapui?P=1E45N-19E45N-19E55N-1E55N-1E45N,10E55.1N,10E45.1N
    bg::model::box<Point> box;
    bg::read_wkt("POLYGON((1 45,19 55))", box);
    BOOST_CHECK_EQUAL(bg::within(Point(10, 55.1), box), true);
    BOOST_CHECK_EQUAL(bg::within(Point(10, 55.2), box), true);
    BOOST_CHECK_EQUAL(bg::within(Point(10, 55.3), box), true);
    BOOST_CHECK_EQUAL(bg::within(Point(10, 55.4), box), false);

    BOOST_CHECK_EQUAL(bg::within(Point(10, 45.1), box), false);
    BOOST_CHECK_EQUAL(bg::within(Point(10, 45.2), box), false);
    BOOST_CHECK_EQUAL(bg::within(Point(10, 45.3), box), false);
    BOOST_CHECK_EQUAL(bg::within(Point(10, 45.4), box), true);

    // Crossing the dateline (Near Tuvalu)
    // http://www.gcmap.com/mapui?P=178E10S-178W10S-178W6S-178E6S-178E10S,180W5.999S,180E9.999S
    // http://en.wikipedia.org/wiki/Tuvalu

    bg::model::box<Point> tuvalu(Point(178, -10), Point(-178, -6));
    BOOST_CHECK_EQUAL(bg::within(Point(180, -8), tuvalu), true);
    BOOST_CHECK_EQUAL(bg::within(Point(-180, -8), tuvalu), true);
    BOOST_CHECK_EQUAL(bg::within(Point(180, -5.999), tuvalu), false);
    BOOST_CHECK_EQUAL(bg::within(Point(180, -10.001), tuvalu), true);
}

void test_3d()
{
    typedef boost::geometry::model::point<double, 3, boost::geometry::cs::cartesian> point_type;
    typedef boost::geometry::model::box<point_type> box_type;
    box_type box(point_type(0, 0, 0), point_type(4, 4, 4));
    BOOST_CHECK_EQUAL(bg::within(point_type(2, 2, 2), box), true);
    BOOST_CHECK_EQUAL(bg::within(point_type(2, 4, 2), box), false);
    BOOST_CHECK_EQUAL(bg::within(point_type(2, 2, 4), box), false);
    BOOST_CHECK_EQUAL(bg::within(point_type(2, 2, 5), box), false);

    box_type box2(point_type(2, 2, 2), point_type(3, 3, 3));
    BOOST_CHECK_EQUAL(bg::within(box2, box), true);

}

template <typename P1, typename P2>
void test_mixed_of()
{
    typedef boost::geometry::model::polygon<P1> polygon_type1;
    typedef boost::geometry::model::polygon<P2> polygon_type2;
    typedef boost::geometry::model::box<P1> box_type1;
    typedef boost::geometry::model::box<P2> box_type2;

    polygon_type1 poly1;
    polygon_type2 poly2;
    boost::geometry::read_wkt("POLYGON((0 0,0 5,5 5,5 0,0 0))", poly1);
    boost::geometry::read_wkt("POLYGON((0 0,0 5,5 5,5 0,0 0))", poly2);

    box_type1 box1(P1(1, 1), P1(4, 4));
    box_type2 box2(P2(0, 0), P2(5, 5));
    P1 p1(3, 3);
    P2 p2(3, 3);

    BOOST_CHECK_EQUAL(bg::within(p1, poly2), true);
    BOOST_CHECK_EQUAL(bg::within(p2, poly1), true);
    BOOST_CHECK_EQUAL(bg::within(p2, box1), true);
    BOOST_CHECK_EQUAL(bg::within(p1, box2), true);
    BOOST_CHECK_EQUAL(bg::within(box1, box2), true);
    BOOST_CHECK_EQUAL(bg::within(box2, box1), false);
}


void test_mixed()
{
    // Mixing point types and coordinate types
    test_mixed_of
        <
            boost::geometry::model::d2::point_xy<double>,
            boost::geometry::model::point<double, 2, boost::geometry::cs::cartesian>
        >();
    test_mixed_of
        <
            boost::geometry::model::d2::point_xy<float>,
            boost::geometry::model::point<double, 2, boost::geometry::cs::cartesian>
        >();
    test_mixed_of
        <
            boost::geometry::model::d2::point_xy<int>,
            boost::geometry::model::d2::point_xy<double>
        >();
}

void test_strategy()
{
    // Test by explicitly specifying a strategy
    typedef bg::model::d2::point_xy<double> point_type;
    typedef bg::model::box<point_type> box_type;
    point_type p(3, 3);
    box_type b(point_type(0, 0), point_type(5, 5));
    box_type b0(point_type(0, 0), point_type(5, 0));

    bool r = bg::within(p, b,
        bg::strategy::within::point_in_box<point_type, box_type>());
    BOOST_CHECK_EQUAL(r, true);

    r = bg::within(b, b,
        bg::strategy::within::box_in_box<box_type, box_type>());
    BOOST_CHECK_EQUAL(r, true);

    r = bg::within(b0, b0,
        bg::strategy::within::box_in_box<box_type, box_type>());
    BOOST_CHECK_EQUAL(r, false);

    r = bg::within(p, b,
        bg::strategy::within::point_in_box_by_side<point_type, box_type>());
    BOOST_CHECK_EQUAL(r, true);
}


int test_main( int , char* [] )
{
    test_all<bg::model::d2::point_xy<int> >();
    test_all<bg::model::d2::point_xy<double> >();

    test_spherical<bg::model::point<double, 2, bg::cs::spherical_equatorial<bg::degree> > >();

    test_mixed();
    test_3d();
    test_strategy();


#if defined(HAVE_TTMATH)
    test_all<bg::model::d2::point_xy<ttmath_big> >();
    test_spherical<bg::model::point<ttmath_big, 2, bg::cs::spherical_equatorial<bg::degree> > >();
#endif

    return 0;
}
