// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2013 Adam Wulkiewicz, Lodz, Poland.

// This file was modified by Oracle on 2013.
// Modifications copyright (c) 2013, Oracle and/or its affiliates.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "test_intersects.hpp"


#include <boost/geometry/geometries/geometries.hpp>
#include <boost/geometry/geometries/point_xy.hpp>

#include <boost/geometry/util/rational.hpp>

template <typename P>
void test_intersects_polygon_polygon()
{
    typedef bg::model::polygon<P, false, false> poly_ccw_o;
    test_geometry<poly_ccw_o, poly_ccw_o>("POLYGON((1 1, 3 3, 2 5))", "POLYGON((0 0, 9 0, 9 9, 0 9),(5 5,5 8,8 8,8 5))", true);
    test_geometry<poly_ccw_o, poly_ccw_o>("POLYGON((6 6, 7 6, 7 7, 6 7))", "POLYGON((0 0, 9 0, 9 9, 0 9),(5 5,5 8,8 8,8 5))", false);
    test_geometry<poly_ccw_o, poly_ccw_o>("POLYGON((7 7, 9 7, 9 9, 7 9))", "POLYGON((0 0, 9 0, 9 9, 0 9),(5 5,5 8,8 8,8 5))", true);
}

template <typename P>
void test_intersects_linestring_segment()
{
    typedef bg::model::linestring<P> ls;
    typedef bg::model::segment<P> seg;

    test_geometry<ls, seg>("LINESTRING(1 1, 3 3, 2 5)", "SEGMENT(2 0, 2 6)", true);
    test_geometry<ls, seg>("LINESTRING(1 1, 3 3)", "SEGMENT(1 0, 1 1)", true);
    test_geometry<ls, seg>("LINESTRING(1 1, 3 3)", "SEGMENT(2 0, 2 2)", true);
    test_geometry<ls, seg>("LINESTRING(1 1, 3 3)", "SEGMENT(3 0, 4 1)", false);
}

template <typename P>
void test_intersects_linestring_linestring()
{
    typedef bg::model::linestring<P> ls;

    test_geometry<ls, ls>("LINESTRING(0 0,2 0,3 0)", "LINESTRING(0 0,1 1,2 2)", true);
    test_geometry<ls, ls>("LINESTRING(0 0,2 0,3 0)", "LINESTRING(2 2,1 1,0 0)", true);
    test_geometry<ls, ls>("LINESTRING(3 0,2 0,0 0)", "LINESTRING(0 0,1 1,2 2)", true);
    test_geometry<ls, ls>("LINESTRING(3 0,2 0,0 0)", "LINESTRING(2 2,1 1,0 0)", true);

    test_geometry<ls, ls>("LINESTRING(0 0,2 0,3 0)", "LINESTRING(1 0,4 0,5 0)", true);
    test_geometry<ls, ls>("LINESTRING(1 0,2 0)", "LINESTRING(1 0,0 0)", true);
}

template <typename P>
void test_intersects_linestring_polygon()
{
    typedef bg::model::linestring<P> ls;
    typedef bg::model::multi_linestring<ls> mls;
    typedef bg::model::polygon<P> poly_cw_c;
    typedef bg::model::polygon<P, false> poly_ccw_c;
    typedef bg::model::polygon<P, false, false> poly_ccw_o;
    typedef bg::model::multi_polygon<poly_ccw_c> mpoly_ccw_c;

    test_geometry<ls, poly_ccw_c>("LINESTRING(1 1,2 2)", "POLYGON((0 0,10 0,10 10,0 10,0 0))", true);
    test_geometry<ls, poly_ccw_c>("LINESTRING(1 0,2 2)", "POLYGON((0 0,10 0,10 10,0 10,0 0))", true);
    test_geometry<ls, poly_ccw_c>("LINESTRING(11 0,12 12)", "POLYGON((0 0,10 0,10 10,0 10,0 0))", false);

    test_geometry<ls, poly_ccw_o>("LINESTRING(1 1, 3 3, 2 5)", "POLYGON((0 0, 9 0, 9 9, 0 9),(5 5,5 8,8 8,8 5))", true);
    test_geometry<ls, poly_ccw_o>("LINESTRING(6 6, 7 6, 7 7, 6 7)", "POLYGON((0 0, 9 0, 9 9, 0 9),(5 5,5 8,8 8,8 5))", false);
    test_geometry<ls, poly_ccw_o>("LINESTRING(7 7, 9 7, 9 9, 7 9)", "POLYGON((0 0, 9 0, 9 9, 0 9),(5 5,5 8,8 8,8 5))", true);

    test_geometry<poly_cw_c, ls>("POLYGON((0 0, 0 10, 10 10, 10 0, 0 0))", "LINESTRING(-2 -2, 12 7)", true);
    test_geometry<poly_cw_c, ls>("POLYGON((0 0, 0 10, 10 10, 10 0, 0 0))", "LINESTRING(5 5, 15 4)", true);
    test_geometry<poly_cw_c, ls>("POLYGON((0 0, 0 10, 10 10, 10 0, 0 0))", "LINESTRING(7 6, 15 4)", true);
    test_geometry<poly_cw_c, ls>("POLYGON((0 0, 0 10, 10 10, 10 0, 0 0))", "LINESTRING(6 2, 12 1)", true);

    // MULTI
    test_geometry<ls, mpoly_ccw_c>("LINESTRING(1 1,2 2)", "MULTIPOLYGON(((0 0,10 0,10 10,0 10,0 0)))", true);
    test_geometry<mls, mpoly_ccw_c>("MULTILINESTRING((1 1,2 2))", "MULTIPOLYGON(((0 0,10 0,10 10,0 10,0 0)))", true);
}

template <typename P>
void test_intersects_linestring_ring()
{
    typedef bg::model::linestring<P> ls;
    typedef bg::model::multi_linestring<ls> mls;
    typedef bg::model::ring<P, false> ring_ccw_c;

    test_geometry<ls, ring_ccw_c>("LINESTRING(1 1,2 2)", "POLYGON((0 0,10 0,10 10,0 10,0 0))", true);
    test_geometry<ls, ring_ccw_c>("LINESTRING(1 0,2 2)", "POLYGON((0 0,10 0,10 10,0 10,0 0))", true);
    test_geometry<ls, ring_ccw_c>("LINESTRING(11 0,12 12)", "POLYGON((0 0,10 0,10 10,0 10,0 0))", false);

    // MULTI
    test_geometry<mls, ring_ccw_c>("MULTILINESTRING((1 1,2 2))", "POLYGON((0 0,10 0,10 10,0 10,0 0))", true);
}

template <typename P>
void test_intersects_ring_polygon()
{
    typedef bg::model::ring<P, false, false> ring_ccw_o;
    typedef bg::model::polygon<P, false, false> poly_ccw_o;

    test_geometry<ring_ccw_o, poly_ccw_o>("POLYGON((1 1, 3 3, 2 5))", "POLYGON((0 0, 9 0, 9 9, 0 9),(5 5,5 8,8 8,8 5))", true);
    test_geometry<ring_ccw_o, poly_ccw_o>("POLYGON((6 6, 7 6, 7 7, 6 7))", "POLYGON((0 0, 9 0, 9 9, 0 9),(5 5,5 8,8 8,8 5))", false);
    test_geometry<ring_ccw_o, poly_ccw_o>("POLYGON((7 7, 9 7, 9 9, 7 9))", "POLYGON((0 0, 9 0, 9 9, 0 9),(5 5,5 8,8 8,8 5))", true);

    test_geometry<ring_ccw_o, poly_ccw_o>("POLYGON((6 6,7 6,7 7,6 7))", "POLYGON((0 0, 9 0, 9 9, 0 9),(5 5,5 8,8 8,8 5))", false);
}

template <typename P>
void test_intersects_point_linestring()
{
    typedef bg::model::linestring<P> ls;
    typedef bg::model::multi_linestring<ls> mls;

    test_geometry<P, ls>("POINT(0 0)", "LINESTRING(0 0,2 2,4 0)", true);
    test_geometry<P, ls>("POINT(1 1)", "LINESTRING(0 0,2 2,4 0)", true);
    test_geometry<P, ls>("POINT(1 0)", "LINESTRING(0 0,2 2,4 0)", false);

    // MULTI
    test_geometry<P, mls>("POINT(0 0)", "MULTILINESTRING((0 0,2 2,4 0))", true);
}

template <typename P>
void test_intersects_point_segment()
{
    typedef bg::model::segment<P> seg;

    test_geometry<P, seg>("POINT(0 0)", "LINESTRING(0 0,2 2)", true);
    test_geometry<P, seg>("POINT(1 1)", "LINESTRING(0 0,2 2)", true);
    test_geometry<P, seg>("POINT(1 0)", "LINESTRING(0 0,2 2)", false);
}

template <typename P>
void test_multi_linestring_polygon()
{
    typedef bg::model::linestring<P> ls;
    typedef bg::model::multi_linestring<ls> mls;
    typedef bg::model::polygon<P> poly;

    test_geometry<mls, poly>("MULTILINESTRING((11 11, 20 20),(5 7, 4 1))",
                             "POLYGON((0 0,0 10,10 10,10 0,0 0),(2 2,4 2,4 4,2 4,2 2))",
                             true);
    test_geometry<mls, poly>("MULTILINESTRING((10 0, 18 12),(2 2,2 1))",
                             "POLYGON((5 0,0 -5,-5 0,0 5,5 0))",
                             true);
}

template <typename P>
void test_multi_polygon_polygon()
{
    typedef bg::model::polygon<P> poly;
    typedef bg::model::multi_polygon<poly> mpoly;

    test_geometry<mpoly, poly>("MULTIPOLYGON(((11 11,11 20,20 20,20 11,11 11)),((5 5,5 6,6 6,6 5,5 5)))",
                               "POLYGON((0 0,0 10,10 10,10 0,0 0),(2 2,4 2,4 4,2 4,2 2))",
                               true);
}

template <typename P>
void test_all()
{
    typedef bg::model::polygon<P> polygon;
    typedef bg::model::ring<P> ring;

    // intersect <=> ! disjoint (in most cases)
    // so most tests are done in disjoint test.
    // We only test compilation of a few cases.
    test_geometry<P, bg::model::box<P> >("POINT(1 1)", "BOX(0 0,2 2)", true);

    test_geometry<polygon, bg::model::box<P> >(
        "POLYGON((1992 3240,1992 1440,3792 1800,3792 3240,1992 3240))",
        "BOX(1941 2066, 2055 2166)", true);

    test_geometry<ring, bg::model::box<P> >(
        "POLYGON((1992 3240,1992 1440,3792 1800,3792 3240,1992 3240))",
        "BOX(1941 2066, 2055 2166)", true);

    test_geometry<polygon, bg::model::box<P> >(
        "POLYGON((1941 2066,2055 2066,2055 2166,1941 2166))",
        "BOX(1941 2066, 2055 2166)", true);

    test_intersects_point_segment<P>();
    test_intersects_point_linestring<P>();
    test_intersects_polygon_polygon<P>();
    test_intersects_linestring_polygon<P>();
    test_intersects_linestring_ring<P>();
    test_intersects_linestring_segment<P>();
    test_intersects_linestring_linestring<P>();
    test_intersects_ring_polygon<P>();
    test_multi_linestring_polygon<P>();
    test_multi_polygon_polygon<P>();

    // self-intersecting is not tested in disjoint, so that is done here.

    // Just a normal polygon
    test_self_intersects<polygon>("POLYGON((0 0,0 4,1.5 2.5,2.5 1.5,4 0,0 0))", false);

    // Self intersecting
    test_self_intersects<polygon>("POLYGON((1 2,1 1,2 1,2 2.25,3 2.25,3 0,0 0,0 3,3 3,2.75 2,1 2))", true);

    // Self intersecting in last segment
    test_self_intersects<polygon>("POLYGON((0 2,2 4,2 0,4 2,0 2))", true);

    // Self tangent
    test_self_intersects<polygon>("POLYGON((0 0,0 4,4 4,4 0,2 4,0 0))", true);

    // Self tangent in corner
    test_self_intersects<polygon>("POLYGON((0 0,0 4,4 4,4 0,0 4,2 0,0 0))", true);

    // With spike
    test_self_intersects<polygon>("POLYGON((0 0,0 4,4 4,4 2,6 2,4 2,4 0,0 0))", true);

    // Non intersection, but with duplicate
    test_self_intersects<polygon>("POLYGON((0 0,0 4,4 0,4 0,0 0))", false);

    // With many duplicates
    test_self_intersects<polygon>(
        "POLYGON((0 0,0 1,0 1,0 1,0 2,0 2,0 3,0 3,0 3,0 3,0 4,2 4,2 4,4 4,4 0,4 0,3 0,3 0,3 0,3 0,3 0,0 0))",
        false);

    // Hole: interior tangent to exterior
    test_self_intersects<polygon>("POLYGON((0 0,0 4,4 4,4 0,0 0),(1 2,2 4,3 2,1 2))", true);

    // Hole: interior intersecting exterior
    test_self_intersects<polygon>("POLYGON((0 0,0 4,4 4,4 0,0 0),(1 1,1 3,5 4,1 1))", true);

    // Hole: two intersecting holes
    test_self_intersects<polygon>(
        "POLYGON((0 0,0 4,4 4,4 0,0 0),(1 1,1 3,3 3,3 1,1 1),(2 2,2 3.5,3.5 3.5,3.5 2,2 2))", true);

    // Mail Akira T on [Boost-users] at 27-7-2011 3:17
    test_self_intersects<bg::model::linestring<P> >(
        "LINESTRING(0 0,0 4,4 4,2 2,2 5)", true);

    test_self_intersects<bg::model::linestring<P> >(
        "LINESTRING(0 4,4 4,2 2,2 5)", true);

    // Test self-intersections at last segment in close/open rings:
    test_self_intersects<bg::model::ring<P> >(
        "POLYGON((0 0,3 3,4 1,0 0))", false);

    test_self_intersects<bg::model::ring<P, true, false> >(
        "POLYGON((0 0,3 3,4 1))", false);

    test_self_intersects<bg::model::ring<P> >(
        "POLYGON((0 0,3 3,4 1,0 1,0 0))", true);

    test_self_intersects<bg::model::ring<P, true, false> >(
        "POLYGON((0 0,3 3,4 1,0 1))", true);

    // Duplicates in first or last
    test_self_intersects<bg::model::ring<P> >(
        "POLYGON((0 0,3 3,4 1,0 1,0 1,0 0))", true);
    test_self_intersects<bg::model::ring<P> >(
        "POLYGON((0 0,3 3,4 1,0 1,0 0,0 0))", true);
    test_self_intersects<bg::model::ring<P, true, false> >(
        "POLYGON((0 0,3 3,4 1,0 1,0 1))", true);
    test_self_intersects<bg::model::ring<P> >(
        "POLYGON((0 0,0 0,3 3,4 1,0 1,0 1,0 0))", true);
    test_self_intersects<bg::model::ring<P, true, false> >(
        "POLYGON((0 0,0 0,3 3,4 1,0 1,0 1))", true);
    test_self_intersects<bg::model::ring<P> >(
        "POLYGON((0 0,3 3,3 3,4 1,0 1,0 1,0 0))", true);
    test_self_intersects<bg::model::ring<P, true, false> >(
        "POLYGON((0 0,3 3,3 3,4 1,0 1,0 1))", true);

    test_self_intersects<bg::model::ring<P> >(
        "POLYGON((0 0,3 3,4 1,0 0,0 0))", false);
    test_self_intersects<bg::model::ring<P> >(
        "POLYGON((0 0,3 3,4 1,4 1,0 0))", false);
    test_self_intersects<bg::model::ring<P, true, false> >(
        "POLYGON((0 0,3 3,4 1,4 1))", false);
    test_self_intersects<bg::model::ring<P> >(
        "POLYGON((0 0,0 0,3 3,4 1,0 0))", false);
    test_self_intersects<bg::model::ring<P, true, false> >(
        "POLYGON((0 0,0 0,3 3,4 1))", false);
    test_self_intersects<bg::model::ring<P> >(
        "POLYGON((0 0,3 3,3 3,4 1,0 0))", false);
    test_self_intersects<bg::model::ring<P, true, false> >(
        "POLYGON((0 0,3 3,3 3,4 1))", false);

    test_geometry<P, bg::model::box<P> >(
        "POINT(0 0)",
        "BOX(0 0,4 4)",
        true);
    test_geometry<P, bg::model::ring<P> >(
        "POINT(0 0)",
        "POLYGON((0 0,3 3,3 3,4 1))",
        true);
    test_geometry<P, bg::model::polygon<P> >(
        "POINT(0 0)",
        "POLYGON((0 0,3 3,3 3,4 1))",
        true);

    test_geometry<bg::model::ring<P>, P>(
        "POLYGON((0 0,3 3,3 3,4 1))",
        "POINT(0 0)",
        true);
    test_geometry<bg::model::polygon<P>, P>(
        "POLYGON((0 0,3 3,3 3,4 1))",
        "POINT(0 0)",
        true);
}

// Those tests won't pass for rational<> because numeric_limits<> isn't specialized for this type
template <typename P>
void test_additional()
{
    test_geometry<bg::model::segment<P>, bg::model::box<P> >(
        "SEGMENT(0 0,3 3)",
        "BOX(1 2,3 5)",
        true);
    test_geometry<bg::model::segment<P>, bg::model::box<P> >(
        "SEGMENT(1 1,2 3)",
        "BOX(0 0,4 4)",
        true);
    test_geometry<bg::model::segment<P>, bg::model::box<P> >(
        "SEGMENT(1 1,1 1)",
        "BOX(1 0,3 5)",
        true);
    test_geometry<bg::model::segment<P>, bg::model::box<P> >(
        "SEGMENT(0 1,0 1)",
        "BOX(1 0,3 5)",
        false);
    test_geometry<bg::model::segment<P>, bg::model::box<P> >(
        "SEGMENT(2 1,2 1)",
        "BOX(1 0,3 5)",
        true);
    test_geometry<bg::model::linestring<P>, bg::model::box<P> >(
        "LINESTRING(0 0,1 0,10 10)",
        "BOX(1 2,3 5)",
        true);
    test_geometry<bg::model::linestring<P>, bg::model::box<P> >(
        "LINESTRING(1 2)",
        "BOX(0 0,3 5)",
        true);
}


int test_main( int , char* [] )
{
    test_all<bg::model::d2::point_xy<double> >();
    test_additional<bg::model::d2::point_xy<double> >();

#if ! defined(BOOST_GEOMETRY_RESCALE_TO_ROBUST)
    test_all<bg::model::d2::point_xy<boost::rational<int> > >();
#endif

#if defined(HAVE_TTMATH)
    test_all<bg::model::d2::point_xy<ttmath_big> >();
#endif

    return 0;
}
