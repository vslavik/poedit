// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.

// This file was modified by Oracle on 2013, 2014.
// Modifications copyright (c) 2013-2014 Oracle and/or its affiliates.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

#include <algorithms/test_relate.hpp>

//TEST
//#include <to_svg.hpp>

template <typename P>
void test_polygon_polygon()
{
    typedef bg::model::polygon<P> poly;
    typedef bg::model::ring<P> ring;

    // touching
    test_geometry<poly, poly>("POLYGON((0 0,0 10,10 10,10 0,0 0))",
                              "POLYGON((10 0,10 10,20 10,20 0,10 0))",
                              "FF2F11212");
    test_geometry<poly, poly>("POLYGON((0 0,0 10,10 10,10 0,0 0))",
                              "POLYGON((0 -10,0 0,10 0,10 -10,0 -10))",
                              "FF2F11212");
    test_geometry<poly, poly>("POLYGON((0 0,0 10,10 10,10 0,0 0))",
                              "POLYGON((10 0,15 10,20 10,20 0,10 0))",
                              "FF2F01212");

    // containing
    test_geometry<poly, poly>("POLYGON((0 0,0 10,10 10,10 0,0 0))",
                              "POLYGON((5 5,5 10,6 10,6 5,5 5))",
                              "212F11FF2");
    test_geometry<poly, poly>("POLYGON((0 0,0 10,10 10,10 0,0 0))",
                              "POLYGON((5 5,5 10,6 5,5 5))",
                              "212F01FF2");
    test_geometry<poly, poly>("POLYGON((0 0,0 10,10 10,10 0,0 0))",
                              "POLYGON((5 5,5 6,6 6,6 5,5 5))",
                              "212FF1FF2");

    // fully containing
    test_geometry<poly, poly>("POLYGON((0 0,0 10,10 10,10 0,0 0))",
                              "POLYGON((5 5,5 9,6 9,6 5,5 5))",
                              "212FF1FF2");
    // fully containing, with a hole
    test_geometry<poly, poly>("POLYGON((0 0,0 10,10 10,10 0,0 0),(3 3,7 3,7 7,3 7,3 3))",
                              "POLYGON((1 1,1 9,9 9,9 1,1 1))",
                              "2121F12F2");
    // fully containing, both with holes
    test_geometry<poly, poly>("POLYGON((0 0,0 10,10 10,10 0,0 0),(3 3,7 3,7 7,3 7,3 3))",
                              "POLYGON((1 1,1 9,9 9,9 1,1 1),(2 2,8 2,8 8,2 8,2 2))",
                              "212FF1FF2");
    // fully containing, both with holes
    test_geometry<poly, poly>("POLYGON((0 0,0 10,10 10,10 0,0 0),(3 3,7 3,7 7,3 7,3 3))",
                              "POLYGON((1 1,1 9,9 9,9 1,1 1),(4 4,6 4,6 6,4 6,4 4))",
                              "2121F1212");

    // overlapping
    test_geometry<poly, poly>("POLYGON((0 0,0 10,10 10,10 0,0 0))",
                              "POLYGON((5 0,5 10,20 10,20 0,5 0))",
                              "212111212");
    test_geometry<ring, poly>("POLYGON((0 0,0 10,10 10,10 0,0 0))",
                              "POLYGON((5 0,5 10,20 10,20 0,5 0))",
                              "212111212");
    test_geometry<ring, ring>("POLYGON((0 0,0 10,10 10,10 0,0 0))",
                              "POLYGON((5 0,5 10,20 10,20 0,5 0))",
                              "212111212");
    test_geometry<poly, poly>("POLYGON((0 0,0 10,10 10,15 5,10 0,0 0))",
                              "POLYGON((10 0,5 5,10 10,20 10,20 0,10 0))",
                              "212101212");

    // equal
    test_geometry<poly, poly>("POLYGON((0 0,0 10,10 10,10 0,0 0))",
                              "POLYGON((10 10,10 5,10 0,5 0,0 0,0 10,5 10,10 10))",
                              "2FFF1FFF2");
    // hole-sized
    test_geometry<poly, poly>("POLYGON((0 0,0 10,10 10,10 0,0 0),(5 5,6 5,6 6,5 6,5 5))",
                              "POLYGON((5 5,5 6,6 6,6 5,5 5))",
                              "FF2F112F2");

    // disjoint
    test_geometry<poly, poly>("POLYGON((0 0,0 10,10 10,10 0,0 0))",
                              "POLYGON((0 20,0 30,10 30,10 20,0 20))",
                              "FF2FF1212");
    // disjoint
    test_geometry<poly, poly>("POLYGON((0 0,0 10,10 10,10 0,0 0),(3 3,7 3,7 7,3 7,3 3))",
                              "POLYGON((0 20,0 30,10 30,10 20,0 20))",
                              "FF2FF1212");

    // equal non-simple / non-simple hole
    test_geometry<poly, poly>("POLYGON((0 0,0 10,10 10,10 0,0 0),(5 5,10 5,5 6,5 5))",
                              "POLYGON((0 0,0 10,10 10,10 5,5 6,5 5,10 5,10 0,0 0))",
                              "2FFF1FFF2");

    // within non-simple / simple
    test_geometry<poly, poly>("POLYGON((0 0,0 10,10 10,10 5,5 6,5 5,10 5,10 0,0 0))",
                              "POLYGON((0 0,5 5,10 5,10 0,0 0))",
                              "212F11FF2");
    // within non-simple hole / simple
    test_geometry<poly, poly>("POLYGON((0 0,0 10,10 10,10 0,0 0),(5 5,10 5,5 6,5 5))",
                              "POLYGON((0 0,5 5,10 5,10 0,0 0))",
                              "212F11FF2");


    // not within non-simple / simple
    test_geometry<poly, poly>("POLYGON((0 0,0 10,10 10,10 5,5 6,5 5,10 5,10 0,0 0))",
                              "POLYGON((0 0,0 10,10 10,10 0,0 0))",
                              "2FF11F2F2");
    // not within non-simple hole / simple
    test_geometry<poly, poly>("POLYGON((0 0,0 10,10 10,10 0,0 0),(5 5,10 5,5 6,5 5))",
                              "POLYGON((0 0,0 10,10 10,10 0,0 0))",
                              "2FF11F2F2");
    // not within simple hole / simple
    test_geometry<poly, poly>("POLYGON((0 0,0 10,10 10,10 0,0 0),(5 5,9 5,5 6,5 5))",
                              "POLYGON((0 0,0 10,10 10,9 5,10 0,0 0))",
                              "2121112F2");

    // within non-simple fake hole / simple
    test_geometry<poly, poly>("POLYGON((0 0,0 10,10 10,10 5,4 7,4 3,10 5,10 0,0 0))",
                              "POLYGON((0 0,0 10,10 10,10 0,0 0))",
                              "2FF11F2F2");
    // within non-simple fake hole / non-simple fake hole
    test_geometry<poly, poly>("POLYGON((0 0,0 10,10 10,10 5,4 7,4 3,10 5,10 0,0 0))",
                              "POLYGON((0 0,0 10,10 10,10 5,5 6,5 4,10 5,10 0,0 0))",
                              "2FF11F212");
    // within non-simple fake hole / non-simple hole
    test_geometry<poly, poly>("POLYGON((0 0,0 10,10 10,10 5,4 7,4 3,10 5,10 0,0 0))",
                              "POLYGON((0 0,0 10,10 10,10 0,0 0),(10 5,5 6,5 4,10 5))",
                              "2FF11F212");
    // containing non-simple fake hole / non-simple hole
    test_geometry<poly, poly>("POLYGON((0 0,0 10,10 10,10 5,4 7,4 3,10 5,10 0,0 0))",
                              "POLYGON((0 0,0 10,10 10,10 0,0 0),(10 5,3 8,3 2,10 5))",
                              "212F11FF2");

    // within non-simple hole / simple
    test_geometry<poly, poly>("POLYGON((0 0,0 10,10 10,10 0,0 0),(10 5,4 7,4 3,10 5))",
                              "POLYGON((0 0,0 10,10 10,10 0,0 0))",
                              "2FF11F2F2");
    // within non-simple hole / non-simple fake hole
    test_geometry<poly, poly>("POLYGON((0 0,0 10,10 10,10 0,0 0),(10 5,4 7,4 3,10 5))",
                              "POLYGON((0 0,0 10,10 10,10 5,5 6,5 4,10 5,10 0,0 0))",
                              "2FF11F212");
    // containing non-simple hole / non-simple fake hole
    test_geometry<poly, poly>("POLYGON((0 0,0 10,10 10,10 0,0 0),(10 5,4 7,4 3,10 5))",
                              "POLYGON((0 0,0 10,10 10,10 5,3 8,3 2,10 5,10 0,0 0))",
                              "212F11FF2");
    // equal non-simple hole / non-simple fake hole
    test_geometry<poly, poly>("POLYGON((0 0,0 10,10 10,10 0,0 0),(10 5,4 7,4 3,10 5))",
                              "POLYGON((0 0,0 10,10 10,10 5,4 7,4 3,10 5,10 0,0 0))",
                              "2FFF1FFF2");
    // within non-simple hole / non-simple hole
    test_geometry<poly, poly>("POLYGON((0 0,0 10,10 10,10 0,0 0),(10 5,4 7,4 3,10 5))",
                              "POLYGON((0 0,0 10,10 10,10 0,0 0),(10 5,5 6,5 4,10 5))",
                              "2FF11F212");
    // containing non-simple hole / non-simple hole
    test_geometry<poly, poly>("POLYGON((0 0,0 10,10 10,10 0,0 0),(10 5,4 7,4 3,10 5))",
                              "POLYGON((0 0,0 10,10 10,10 0,0 0),(10 5,3 8,3 2,10 5))",
                              "212F11FF2");
    // equal non-simple hole / non-simple hole
    test_geometry<poly, poly>("POLYGON((0 0,0 10,10 10,10 0,0 0),(10 5,4 7,4 3,10 5))",
                              "POLYGON((0 0,0 10,10 10,10 0,0 0),(10 5,4 7,4 3,10 5))",
                              "2FFF1FFF2");

    // intersecting non-simple hole / non-simple hole - touching holes
    test_geometry<poly, poly>("POLYGON((0 0,0 10,10 10,10 0,0 0),(5 5,10 5,5 6,5 5))",
                              "POLYGON((0 0,0 10,10 10,10 0,0 0),(0 5,5 4,5 5,0 5))",
                              "21211F2F2");
    // intersecting non-simple fake hole / non-simple hole - touching holes
    test_geometry<poly, poly>("POLYGON((0 0,0 10,10 10,10 5,5 6,5 5,10 5,10 0,0 0))",
                              "POLYGON((0 0,0 10,10 10,10 0,0 0),(0 5,5 4,5 5,0 5))",
                              "21211F2F2");
    // intersecting non-simple fake hole / non-simple fake hole - touching holes
    test_geometry<poly, poly>("POLYGON((0 0,0 10,10 10,10 5,5 6,5 5,10 5,10 0,0 0))",
                              "POLYGON((0 0,0 5,5 4,5 5,0 5,0 10,10 10,10 0,0 0))",
                              "21211F2F2");

    // intersecting simple - i/i
    test_geometry<poly, poly>("POLYGON((0 0,0 10,4 10,6 8,5 5,6 2,4 0,0 0))",
                              "POLYGON((5 5,4 8,6 10,10 10,10 0,6 0,4 2,5 5))",
                              "212101212");
    // intersecting non-simple hole / non-simple hole - i/i
    test_geometry<poly, poly>("POLYGON((0 0,0 10,4 10,6 8,5 5,6 2,4 0,0 0),(5 5,2 6,2 4,5 5))",
                              "POLYGON((5 5,4 8,6 10,10 10,10 0,6 0,4 2,5 5),(5 5,8 4,8 6,5 5))",
                              "212101212");
    // intersecting non-simple hole / simple - i/i
    test_geometry<poly, poly>("POLYGON((0 0,0 10,4 10,6 8,5 5,6 2,4 0,0 0),(5 5,2 6,2 4,5 5))",
                              "POLYGON((5 5,4 8,6 10,10 10,10 0,6 0,4 2,5 5))",
                              "212101212");

    // no turns - disjoint inside a hole
    test_geometry<poly, poly>("POLYGON((0 0,0 10,10 10,10 0,0 0),(1 1,9 1,9 9,1 9,1 1))",
                              "POLYGON((3 3,3 7,7 7,7 3,3 3))",
                              "FF2FF1212");
    // no turns - within
    test_geometry<poly, poly>("POLYGON((0 0,0 10,10 10,10 0,0 0),(1 1,9 1,9 9,1 9,1 1))",
                              "POLYGON((-1 -1,-1 11,11 11,11 -1,-1 -1))",
                              "2FF1FF212");
    // no-turns - intersects
    test_geometry<poly, poly>("POLYGON((0 0,0 10,10 10,10 0,0 0),(2 2,8 2,8 8,2 8,2 2))",
                              "POLYGON((1 1,1 9,9 9,9 1,1 1))",
                              "2121F12F2");
    // no-turns - intersects, hole in a hole
    test_geometry<poly, poly>("POLYGON((0 0,0 10,10 10,10 0,0 0),(2 2,8 2,8 8,2 8,2 2))",
                              "POLYGON((1 1,1 9,9 9,9 1,1 1),(3 3,7 3,7 7,3 7,3 3))",
                              "2121F1212");

    // no-turns ring - for exteriors
    test_geometry<poly, poly>("POLYGON((0 0,0 10,10 10,10 0,0 0),(2 2,8 2,8 8,2 8,2 2))",
                              "POLYGON((1 1,1 9,9 9,9 1,1 1),(2 2,8 2,8 8,2 8,2 2))",
                              "212F11FF2");
    // no-turns ring - for interiors
    test_geometry<poly, poly>("POLYGON((0 0,0 10,10 10,10 0,0 0),(3 3,7 3,7 7,3 7,3 3))",
                              "POLYGON((0 0,0 10,10 10,10 0,0 0),(2 2,8 2,8 8,2 8,2 2))",
                              "212F11FF2");

    {
        test_geometry<poly, poly>("POLYGON((0 0,0 10,10 10,10 0,0 0))",
                              "POLYGON((5 5,5 10,6 10,6 5,5 5))",
                              "212F11FF2");

        test_geometry<poly, poly>("POLYGON((0 0,0 10,10 10,10 0,0 0))",
                                  "POLYGON((10 0,10 10,20 10,20 0,10 0))",
                                  "FF2F11212");

        namespace bgdr = bg::detail::relate;
        poly p1, p2, p3;
        bg::read_wkt("POLYGON((0 0,0 10,10 10,10 0,0 0))", p1);
        bg::read_wkt("POLYGON((10 0,10 10,20 10,20 0,10 0))", p2);
        bg::read_wkt("POLYGON((5 5,5 10,6 10,6 5,5 5))", p3);
        BOOST_CHECK(bgdr::relate(p1, p2, bgdr::mask9("FT*******")
                                      || bgdr::mask9("F**T*****")
                                      || bgdr::mask9("F***T****"))); // touches()
        BOOST_CHECK(bgdr::relate(p1, p3, bgdr::mask9("T*****FF*"))); // contains()
        BOOST_CHECK(bgdr::relate(p2, p3, bgdr::mask9("FF*FF****"))); // disjoint()
    }

    // CCW
    {
        typedef bg::model::polygon<P, false> poly;
        // within non-simple hole / simple
        test_geometry<poly, poly>("POLYGON((0 0,10 0,10 10,0 10,0 0),(5 5,5 6,10 5,5 5))",
                                  "POLYGON((0 0,10 0,10 5,5 5,0 0))",
                                  "212F11FF2");
    }
    // OPEN
    {
        typedef bg::model::polygon<P, true, false> poly;
        // within non-simple hole / simple
        test_geometry<poly, poly>("POLYGON((0 0,0 10,10 10,10 0),(5 5,10 5,5 6))",
                                  "POLYGON((0 0,5 5,10 5,10 0))",
                                  "212F11FF2");
    }
    // CCW, OPEN
    {
        typedef bg::model::polygon<P, false, false> poly;
        // within non-simple hole / simple
        test_geometry<poly, poly>("POLYGON((0 0,10 0,10 10,0 10),(5 5,5 6,10 5))",
                                  "POLYGON((0 0,10 0,10 5,5 5))",
                                  "212F11FF2");
    }
}

template <typename P>
void test_polygon_multi_polygon()
{
    typedef bg::model::polygon<P> poly;
    typedef bg::model::ring<P> ring;
    typedef bg::model::multi_polygon<poly> mpoly;

    test_geometry<poly, mpoly>("POLYGON((0 0,0 10,10 10,10 0,0 0))",
                               "MULTIPOLYGON(((5 5,5 10,6 10,6 5,5 5)),((0 20,0 30,10 30,10 20,0 20)))",
                               "212F11212");
    test_geometry<ring, mpoly>("POLYGON((0 0,0 10,10 10,10 0,0 0))",
                               "MULTIPOLYGON(((5 5,5 10,6 10,6 5,5 5)),((0 20,0 30,10 30,10 20,0 20)))",
                               "212F11212");
}

template <typename P>
void test_multi_polygon_multi_polygon()
{
    typedef bg::model::polygon<P> poly;
    typedef bg::model::multi_polygon<poly> mpoly;

    test_geometry<mpoly, mpoly>("MULTIPOLYGON(((0 0,0 10,10 10,10 0,0 0)))",
                                "MULTIPOLYGON(((5 5,5 10,6 10,6 5,5 5)),((0 20,0 30,10 30,10 20,0 20)))",
                                "212F11212");
    test_geometry<mpoly, mpoly>("MULTIPOLYGON(((0 0,0 10,10 10,10 0,0 0)),((0 20,0 30,10 30,10 20,0 20)))",
                                "MULTIPOLYGON(((5 5,5 10,6 10,6 5,5 5)))",
                                "212F11FF2");

    test_geometry<mpoly, mpoly>("MULTIPOLYGON(((0 0,0 10,10 10,10 0,0 0)))",
                                "MULTIPOLYGON(((5 5,5 6,6 6,6 5,5 5)),((0 20,0 30,10 30,10 20,0 20)))",
                                "212FF1212");
    test_geometry<mpoly, mpoly>("MULTIPOLYGON(((0 0,0 10,10 10,10 0,0 0)),((0 20,0 30,10 30,10 20,0 20)))",
                                "MULTIPOLYGON(((5 5,5 6,6 6,6 5,5 5)))",
                                "212FF1FF2");
}

template <typename P>
void test_all()
{
    test_polygon_polygon<P>();
    test_polygon_multi_polygon<P>();
    test_multi_polygon_multi_polygon<P>();
}

int test_main( int , char* [] )
{
    test_all<bg::model::d2::point_xy<int> >();
    test_all<bg::model::d2::point_xy<double> >();

#if defined(HAVE_TTMATH)
    test_all<bg::model::d2::point_xy<ttmath_big> >();
#endif

    return 0;
}
