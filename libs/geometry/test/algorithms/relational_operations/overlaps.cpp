// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.

// This file was modified by Oracle on 2014, 2015.
// Modifications copyright (c) 2014-2015 Oracle and/or its affiliates.

// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "test_overlaps.hpp"

template <typename P>
void test_box_box_2d()
{
#if defined(BOOST_GEOMETRY_COMPILE_FAIL)
    test_geometry<P, P>("POINT(1 1)", "POINT(1 1)", true);
#endif

    test_geometry<bg::model::box<P>, bg::model::box<P> >("BOX(1 1, 3 3)", "BOX(0 0,2 2)", true);

    // touch -> false
    test_geometry<bg::model::box<P>, bg::model::box<P> >("BOX(1 1, 3 3)", "BOX(3 3,5 5)", false);

    // disjoint -> false
    test_geometry<bg::model::box<P>, bg::model::box<P> >("BOX(1 1, 3 3)", "BOX(4 4,6 6)", false);

    // within -> false
    test_geometry<bg::model::box<P>, bg::model::box<P> >("BOX(1 1, 5 5)", "BOX(2 2,3 3)", false);

    // within+touch -> false
    test_geometry<bg::model::box<P>, bg::model::box<P> >("BOX(1 1, 5 5)", "BOX(2 2,5 5)", false);
}

template <typename P>
void test_3d()
{
    test_geometry<bg::model::box<P>, bg::model::box<P> >("BOX(1 1 1, 3 3 3)", "BOX(0 0 0,2 2 2)", true);
    test_geometry<bg::model::box<P>, bg::model::box<P> >("BOX(1 1 1, 3 3 3)", "BOX(3 3 3,5 5 5)", false);
    test_geometry<bg::model::box<P>, bg::model::box<P> >("BOX(1 1 1, 3 3 3)", "BOX(4 4 4,6 6 6)", false);
}

template <typename P>
void test_pp()
{
    typedef bg::model::multi_point<P> mpt;

    test_geometry<mpt, mpt>("MULTIPOINT(0 0,1 1,2 2)", "MULTIPOINT(1 1,3 3,4 4)", true);
    test_geometry<mpt, mpt>("MULTIPOINT(0 0,1 1,2 2)", "MULTIPOINT(1 1,2 2)", false);
}

template <typename P>
void test_ll()
{
    typedef bg::model::linestring<P> ls;
    typedef bg::model::multi_linestring<ls> mls;

    test_geometry<ls, ls>("LINESTRING(0 0,2 2,3 1)", "LINESTRING(1 1,2 2,4 4)", true);
    test_geometry<ls, ls>("LINESTRING(0 0,2 2,4 0)", "LINESTRING(0 1,2 1,3 2)", false);

    test_geometry<ls, mls>("LINESTRING(0 0,2 2,3 1)", "MULTILINESTRING((1 1,2 2),(2 2,4 4))", true);
    test_geometry<ls, mls>("LINESTRING(0 0,2 2,3 1)", "MULTILINESTRING((1 1,2 2),(3 3,4 4))", true);
    test_geometry<ls, mls>("LINESTRING(0 0,3 3,3 1)", "MULTILINESTRING((3 3,2 2),(0 0,1 1))", false);
}

template <typename P>
void test_aa()
{
    typedef bg::model::polygon<P> poly;
    typedef bg::model::multi_polygon<poly> mpoly;

    test_geometry<poly, poly>("POLYGON((0 0,0 5,5 5,5 0,0 0))", "POLYGON((3 3,3 9,9 9,9 3,3 3))", true);
    test_geometry<poly, poly>("POLYGON((0 0,0 5,5 5,5 0,0 0))", "POLYGON((5 5,5 9,9 9,9 5,5 5))", false);
    test_geometry<poly, poly>("POLYGON((0 0,0 5,5 5,5 0,0 0))", "POLYGON((3 3,3 5,5 5,5 3,3 3))", false);

    test_geometry<poly, mpoly>("POLYGON((0 0,0 5,5 5,5 0,0 0))",
                               "MULTIPOLYGON(((3 3,3 5,5 5,5 3,3 3)),((5 5,5 6,6 6,6 5,5 5)))",
                               true);
    test_geometry<mpoly, mpoly>("MULTIPOLYGON(((3 3,3 5,5 5,5 3,3 3)),((0 0,0 3,3 3,3 0,0,0)))",
                                "MULTIPOLYGON(((3 3,3 5,5 5,5 3,3 3)),((5 5,5 6,6 6,6 5,5 5)))",
                                true);

    // related to https://svn.boost.org/trac/boost/ticket/10912
    test_geometry<poly, poly>("POLYGON((0 0,0 10,10 10,10 0,0 0),(2 2,4 2,4 4,2 4,2 2))",
                              "POLYGON((3 3,3 9,9 9,9 3,3 3))",
                              true);
    test_geometry<poly, poly>("POLYGON((0 0,0 10,10 10,10 0,0 0),(2 2,4 2,4 4,2 4,2 2),(6 6,8 6,8 8,6 8,6 6))",
                              "POLYGON((0 0,0 5,5 5,5 0,0 0))",
                              true);

    test_geometry<mpoly, poly>("MULTIPOLYGON(((0 0,0 10,10 10,10 0,0 0)),((0 0,0 -10,-10 -10,-10 0,0 0)))",
                               "POLYGON((0 0,0 5,5 5,5 0,0 0))",
                               false);
    test_geometry<mpoly, poly>("MULTIPOLYGON(((0 0,0 10,10 10,10 0,0 0)),((0 0,0 -10,-10 -10,-10 0,0 0)))",
                               "POLYGON((0 0,0 10,10 10,10 0,0 0))",
                               false);
}

template <typename P>
void test_2d()
{
    test_pp<P>();
    test_ll<P>();
    test_aa<P>();

    test_box_box_2d<P>();
}

int test_main( int , char* [] )
{
    test_2d<bg::model::d2::point_xy<int> >();
    test_2d<bg::model::d2::point_xy<double> >();

#if defined(HAVE_TTMATH)
    test_2d<bg::model::d2::point_xy<ttmath_big> >();
#endif

   //test_3d<bg::model::point<double, 3, bg::cs::cartesian> >();

    return 0;
}
