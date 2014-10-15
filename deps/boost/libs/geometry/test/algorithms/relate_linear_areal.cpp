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
void test_linestring_polygon()
{
    typedef bg::model::linestring<P> ls;
    typedef bg::model::polygon<P> poly;
    typedef bg::model::ring<P> ring;

    // LS disjoint
    test_geometry<ls, poly>("LINESTRING(11 0,11 10)", "POLYGON((0 0,0 10,10 10,10 0,0 0))", "FF1FF0212");
    test_geometry<ls, ring>("LINESTRING(11 0,11 10)", "POLYGON((0 0,0 10,10 10,10 0,0 0))", "FF1FF0212");

    // II BB
    test_geometry<ls, poly>("LINESTRING(0 0,10 10)", "POLYGON((0 0,0 10,10 10,10 0,0 0))",    "1FFF0F212");
    test_geometry<ls, poly>("LINESTRING(5 0,5 5,10 5)", "POLYGON((0 0,0 10,10 10,10 0,0 0))", "1FFF0F212");
    test_geometry<ls, poly>("LINESTRING(5 1,5 5,9 5)", "POLYGON((0 0,0 10,10 10,10 0,0 0))",  "1FF0FF212");
    
    // IE 
    test_geometry<ls, poly>("LINESTRING(11 1,11 5)", "POLYGON((0 0,0 10,10 10,10 0,0 0))", "FF1FF0212");
    // IE IB0
    test_geometry<ls, poly>("LINESTRING(11 1,10 5)", "POLYGON((0 0,0 10,10 10,10 0,0 0))", "FF1F00212");
    // IE IB1
    test_geometry<ls, poly>("LINESTRING(11 1,10 5,10 10)", "POLYGON((0 0,0 10,10 10,10 0,0 0))", "F11F00212");
    test_geometry<ls, poly>("LINESTRING(11 1,10 10,0 10)", "POLYGON((0 0,0 10,10 10,10 0,0 0))", "F11F00212");
    test_geometry<ls, poly>("LINESTRING(11 1,10 0,0 0)", "POLYGON((0 0,0 10,10 10,10 0,0 0))", "F11F00212");
    test_geometry<ls, poly>("LINESTRING(0 -1,1 0,2 0)", "POLYGON((0 0,0 10,10 10,10 0,0 0))", "F11F00212");
    // IE IB0 II
    test_geometry<ls, poly>("LINESTRING(11 1,10 5,5 5)", "POLYGON((0 0,0 10,10 10,10 0,0 0))", "1010F0212");
    // IE IB0 lring
    test_geometry<ls, poly>("LINESTRING(11 1,10 5,11 5,11 1)", "POLYGON((0 0,0 10,10 10,10 0,0 0))", "F01FFF212");
    // IE IB1 lring
    test_geometry<ls, poly>("LINESTRING(11 1,10 5,10 10,11 5,11 1)", "POLYGON((0 0,0 10,10 10,10 0,0 0))", "F11FFF212");
    
    // IB1 II
    test_geometry<ls, poly>("LINESTRING(0 0,5 0,5 5)", "POLYGON((0 0,0 10,10 10,10 0,0 0))", "11F00F212");
    // BI0 II IB1
    test_geometry<ls, poly>("LINESTRING(5 0,5 5,10 5,10 10)", "POLYGON((0 0,0 10,10 10,10 0,0 0))", "11FF0F212");

    // IB1 II IB1
    test_geometry<ls, poly>("LINESTRING(1 0,2 0,3 1,4 0,5 0)", "POLYGON((0 0,0 10,10 10,10 0,0 0))", "11FF0F212");
    // IB1 IE IB1
    test_geometry<ls, poly>("LINESTRING(1 0,2 0,3 -1,4 0,5 0)", "POLYGON((0 0,0 10,10 10,10 0,0 0))", "F11F0F212");

    // II IB1
    test_geometry<ls, poly>("LINESTRING(5 5,10 5,10 10)", "POLYGON((0 0,0 10,10 10,10 0,0 0))", "11F00F212");
    // IB1 II
    test_geometry<ls, poly>("LINESTRING(10 10,10 5,5 5)", "POLYGON((0 0,0 10,10 10,10 0,0 0))", "11F00F212");
    // IE IB1
    test_geometry<ls, poly>("LINESTRING(15 5,10 5,10 10)", "POLYGON((0 0,0 10,10 10,10 0,0 0))", "F11F00212");
    // IB1 IE
    test_geometry<ls, poly>("LINESTRING(10 10,10 5,15 5)", "POLYGON((0 0,0 10,10 10,10 0,0 0))", "F11F00212");

    // duplicated points
    {
        // II IB0 IE
        test_geometry<ls, poly>("LINESTRING(5 5,10 5,15 10)", "POLYGON((0 0,0 10,10 10,10 0,0 0))", "1010F0212");
        test_geometry<ls, poly>("LINESTRING(5 5,5 5,5 5,10 5,10 5,10 5,15 10,15 10,15 10)",
            "POLYGON((0 0,0 0,0 0,0 10,0 10,0 10,10 10,10 10,10 10,10 0,10 0,10 0,0 0,0 0,0 0))",
            "1010F0212");
        test_geometry<ls, poly>("LINESTRING(5 5,5 5,5 5,10 0,10 0,10 0,15 10,15 10,15 10)",
            "POLYGON((0 0,0 0,0 0,0 10,0 10,0 10,10 10,10 10,10 10,10 0,10 0,10 0,0 0,0 0,0 0))",
            "1010F0212");
        // IE IB0 II
        test_geometry<ls, poly>("LINESTRING(15 10,15 10,15 10,10 5,10 5,10 5,5 5,5 5,5 5)",
            "POLYGON((0 0,0 0,0 0,0 10,0 10,0 10,10 10,10 10,10 10,10 0,10 0,10 0,0 0,0 0,0 0))",
            "1010F0212");
        test_geometry<ls, poly>("LINESTRING(15 10,15 10,15 10,10 0,10 0,10 0,5 5,5 5,5 5)",
            "POLYGON((0 0,0 0,0 0,0 10,0 10,0 10,10 10,10 10,10 10,10 0,10 0,10 0,0 0,0 0,0 0))",
            "1010F0212");

        // TEST
        //test_geometry<ls, poly>("LINESTRING(5 5,5 5,5 5)", "POLYGON((0 0,0 10,10 10,10 0,0 0))", "1010F0212");
        test_geometry<ls, poly>("LINESTRING(5 5,5 5,5 5,15 5,15 5,15 5)", "POLYGON((0 0,0 10,10 10,10 0,0 0))", "1010F0212");
    }

    // non-simple polygon with hole
    test_geometry<ls, poly>("LINESTRING(9 1,10 5,9 9)",
                            "POLYGON((0 0,0 10,10 10,10 0,0 0),(10 5,2 8,2 2,10 5))",
                            "10F0FF212");
    test_geometry<ls, poly>("LINESTRING(10 1,10 5,10 9)",
                            "POLYGON((0 0,0 10,10 10,10 0,0 0),(10 5,2 8,2 2,10 5))",
                            "F1FF0F212");
    test_geometry<ls, poly>("LINESTRING(2 8,10 5,2 2)",
                            "POLYGON((0 0,0 10,10 10,10 0,0 0),(10 5,2 8,2 2,10 5))",
                            "F1FF0F212");
    test_geometry<ls, poly>("LINESTRING(10 1,10 5,2 2)",
                            "POLYGON((0 0,0 10,10 10,10 0,0 0),(10 5,2 8,2 2,10 5))",
                            "F1FF0F212");
    test_geometry<ls, poly>("LINESTRING(10 1,10 5,2 8)",
                            "POLYGON((0 0,0 10,10 10,10 0,0 0),(10 5,2 8,2 2,10 5))",
                            "F1FF0F212");

    // non-simple polygon with hole, linear ring
    test_geometry<ls, poly>("LINESTRING(9 1,10 5,9 9,1 9,1 1,9 1)",
                            "POLYGON((0 0,0 10,10 10,10 0,0 0),(10 5,2 8,2 2,10 5))",
                            "10FFFF212");
    test_geometry<ls, poly>("LINESTRING(10 5,10 9,11 5,10 1,10 5)",
                            "POLYGON((0 0,0 10,10 10,10 0,0 0),(10 5,2 8,2 2,10 5))",
                            "F11FFF212");
    test_geometry<ls, poly>("LINESTRING(11 5,10 1,10 5,10 9,11 5)",
                            "POLYGON((0 0,0 10,10 10,10 0,0 0),(10 5,2 8,2 2,10 5))",
                            "F11FFF212");

    // non-simple polygon with self-touching holes
    test_geometry<ls, poly>("LINESTRING(7 1,8 5,7 9)",
                            "POLYGON((0 0,0 10,10 10,10 0,0 0),(8 1,9 1,9 9,8 9,8 1),(2 2,8 5,2 8,2 2))",
                            "10F0FF212");
    test_geometry<ls, poly>("LINESTRING(8 2,8 5,8 8)",
                            "POLYGON((0 0,0 10,10 10,10 0,0 0),(8 1,9 1,9 9,8 9,8 1),(2 2,8 5,2 8,2 2))",
                            "F1FF0F212");
    test_geometry<ls, poly>("LINESTRING(2 8,8 5,2 2)",
                            "POLYGON((0 0,0 10,10 10,10 0,0 0),(8 1,9 1,9 9,8 9,8 1),(2 2,8 5,2 8,2 2))",
                            "F1FF0F212");

    // non-simple polygon self-touching
    test_geometry<ls, poly>("LINESTRING(9 1,10 5,9 9)",
                            "POLYGON((0 0,0 10,10 10,10 5,2 8,2 2,10 5,10 0,0 0))",
                            "10F0FF212");
    test_geometry<ls, poly>("LINESTRING(10 1,10 5,10 9)",
                            "POLYGON((0 0,0 10,10 10,10 5,2 8,2 2,10 5,10 0,0 0))",
                            "F1FF0F212");
    test_geometry<ls, poly>("LINESTRING(2 8,10 5,2 2)",
                            "POLYGON((0 0,0 10,10 10,10 5,2 8,2 2,10 5,10 0,0 0))",
                            "F1FF0F212");

    // non-simple polygon self-touching, linear ring
    test_geometry<ls, poly>("LINESTRING(9 1,10 5,9 9,1 9,1 1,9 1)",
                            "POLYGON((0 0,0 10,10 10,10 5,2 8,2 2,10 5,10 0,0 0))",
                            "10FFFF212");
    test_geometry<ls, poly>("LINESTRING(10 5,10 9,11 5,10 1,10 5)",
                            "POLYGON((0 0,0 10,10 10,10 5,2 8,2 2,10 5,10 0,0 0))",
                            "F11FFF212");
    test_geometry<ls, poly>("LINESTRING(11 5,10 1,10 5,10 9,11 5)",
                            "POLYGON((0 0,0 10,10 10,10 5,2 8,2 2,10 5,10 0,0 0))",
                            "F11FFF212");

    // polygons with some ring equal to the linestring
    test_geometry<ls, poly>("LINESTRING(0 0,10 0,10 10,0 10,0 0)",
                            "POLYGON((0 0,0 10,10 10,10 0,0 0))",
                            "F1FFFF2F2");
    test_geometry<ls, poly>("LINESTRING(0 0,10 0,10 10,0 10,0 0)",
                            "POLYGON((0 0,0 10,10 10,10 0,0 0),(2 2,5 5,2 8,2 2))",
                            "F1FFFF212");
    test_geometry<ls, poly>("LINESTRING(2 2,5 5,2 8,2 2)",
                            "POLYGON((0 0,0 10,10 10,10 0,0 0),(2 2,5 5,2 8,2 2))",
                            "F1FFFF212");

    // self-IP going on the boundary then into the exterior and to the boundary again
    test_geometry<ls, poly>("LINESTRING(2 10,5 10,5 15,6 15,5 10,8 10)", "POLYGON((0 0,0 10,10 10,10 0,0 0))",
                            "F11F0F212");
    // self-IP going on the boundary then into the interior and to the boundary again
    test_geometry<ls, poly>("LINESTRING(2 10,5 10,5 5,6 5,5 10,8 10)", "POLYGON((0 0,0 10,10 10,10 0,0 0))",
                            "11FF0F212");

    // self-IP with a hole -> B to I to B to E
    test_geometry<ls, poly>("LINESTRING(0 0,3 3)", "POLYGON((0 0,0 10,10 10,10 0,0 0),(0 0,9 1,9 9,1 9,0 0))",
                            "FF1F00212");

    // ccw
    {
        typedef bg::model::polygon<P, false> ccwpoly;

        // IE IB0 II
        test_geometry<ls, ccwpoly>("LINESTRING(11 1,10 5,5 5)", "POLYGON((0 0,10 0,10 10,0 10,0 0))", "1010F0212");
        // IE IB1 II
        test_geometry<ls, ccwpoly>("LINESTRING(11 1,10 1,10 5,5 5)", "POLYGON((0 0,10 0,10 10,0 10,0 0))", "1110F0212");
        test_geometry<ls, ccwpoly>("LINESTRING(11 1,10 5,10 1,5 5)", "POLYGON((0 0,10 0,10 10,0 10,0 0))", "1110F0212");
        // II IB0 IE
        test_geometry<ls, ccwpoly>("LINESTRING(5 1,10 5,11 1)", "POLYGON((0 0,10 0,10 10,0 10,0 0))", "1010F0212");
        // IE IB1 II
        test_geometry<ls, ccwpoly>("LINESTRING(5 5,10 1,10 5,11 5)", "POLYGON((0 0,10 0,10 10,0 10,0 0))", "1110F0212");
        test_geometry<ls, ccwpoly>("LINESTRING(5 5,10 5,10 1,11 5)", "POLYGON((0 0,10 0,10 10,0 10,0 0))", "1110F0212");

    }

    {
        // SPIKES

        test_geometry<ls, poly>("LINESTRING(0 0,2 2,3 3,1 1)", "POLYGON((0 0,3 3,3 0,0 0))", "F1FF0F212");
        test_geometry<ls, poly>("LINESTRING(0 0,3 3,1 1)", "POLYGON((0 0,3 3,3 0,0 0))", "F1FF0F212");
        test_geometry<ls, poly>("LINESTRING(0 0,2 2,1 1)", "POLYGON((0 0,3 3,3 0,0 0))", "F1FF0F212");
        test_geometry<ls, poly>("LINESTRING(1 1,3 3,2 2)", "POLYGON((0 0,3 3,3 0,0 0))", "F1FF0F212");
        test_geometry<ls, poly>("LINESTRING(1 1,2 2,1 1)", "POLYGON((0 0,3 3,3 0,0 0))", "F1FFFF212");

        test_geometry<ls, poly>("LINESTRING(3 3,1 1,0 0,2 2)", "POLYGON((0 0,3 3,3 0,0 0))", "F1FF0F212");
        test_geometry<ls, poly>("LINESTRING(3 3,0 0,2 2)", "POLYGON((0 0,3 3,3 0,0 0))", "F1FF0F212");
        test_geometry<ls, poly>("LINESTRING(2 2,0 0,1 1)", "POLYGON((0 0,3 3,3 0,0 0))", "F1FF0F212");
        test_geometry<ls, poly>("LINESTRING(3 3,1 1,2 2)", "POLYGON((0 0,3 3,3 0,0 0))", "F1FF0F212");
        test_geometry<ls, poly>("LINESTRING(2 2,1 1,2 2)", "POLYGON((0 0,3 3,3 0,0 0))", "F1FFFF212");

        test_geometry<ls, poly>("LINESTRING(0 0,2 2,4 4,1 1)", "POLYGON((0 0,3 3,3 0,0 0))", "F11F0F212");

        test_geometry<ls, poly>("LINESTRING(0 1,1 1,0 1)", "POLYGON((0 0,3 3,3 0,0 0))", "F01FFF212");
        test_geometry<ls, poly>("LINESTRING(0 1,3 3,0 1)", "POLYGON((0 0,3 3,3 0,0 0))", "F01FFF212");
        test_geometry<ls, poly>("LINESTRING(0 1,0 0,0 1)", "POLYGON((0 0,3 3,3 0,0 0))", "F01FFF212");
        
        test_geometry<ls, poly>("LINESTRING(0 1,1 1,-1 1)", "POLYGON((0 0,3 3,3 0,0 0))", "F01FF0212");
    }
}

template <typename P>
void test_linestring_multi_polygon()
{
    typedef bg::model::linestring<P> ls;
    typedef bg::model::polygon<P> poly;
    typedef bg::model::multi_polygon<poly> mpoly;

    test_geometry<ls, mpoly>("LINESTRING(10 1,10 5,10 9)",
                            "MULTIPOLYGON(((0 20,0 30,10 30,10 20,0 20)),((0 0,0 10,10 10,10 0,0 0),(10 5,2 8,2 2,10 5)))",
                            "F1FF0F212");
    test_geometry<ls, mpoly>("LINESTRING(10 1,10 5,10 9)",
                            "MULTIPOLYGON(((0 20,0 30,10 30,10 20,0 20)),((0 0,0 10,10 10,10 0,0 0)))",
                            "F1FF0F212");

    test_geometry<ls, mpoly>("LINESTRING(10 1,10 5,2 2)",
                            "MULTIPOLYGON(((0 20,0 30,10 30,10 20,0 20)),((0 0,0 10,10 10,10 0,0 0),(10 5,2 8,2 2,10 5)))",
                            "F1FF0F212");
    test_geometry<ls, mpoly>("LINESTRING(10 1,10 5,2 2)",
                            "MULTIPOLYGON(((0 20,0 30,10 30,10 20,0 20)),((0 0,0 10,10 10,10 0,0 0)))",
                            "11F00F212");

    test_geometry<ls, mpoly>("LINESTRING(10 1,10 5,2 2)",
                            "MULTIPOLYGON(((0 0,0 10,10 10,10 0,0 0),(10 5,2 8,2 2,10 5)),((10 5,3 3,3 7,10 5)))",
                            "F1FF0F212");
    test_geometry<ls, mpoly>("LINESTRING(10 1,10 5,2 8)",
                            "MULTIPOLYGON(((0 0,0 10,10 10,10 0,0 0),(10 5,2 8,2 2,10 5)),((10 5,3 3,3 7,10 5)))",
                            "F1FF0F212");
    test_geometry<ls, mpoly>("LINESTRING(10 1,10 5,3 3)",
                            "MULTIPOLYGON(((0 0,0 10,10 10,10 0,0 0),(10 5,2 8,2 2,10 5)),((10 5,3 3,3 7,10 5)))",
                            "F1FF0F212");
    test_geometry<ls, mpoly>("LINESTRING(10 1,10 5,3 7)",
                            "MULTIPOLYGON(((0 0,0 10,10 10,10 0,0 0),(10 5,2 8,2 2,10 5)),((10 5,3 3,3 7,10 5)))",
                            "F1FF0F212");
    test_geometry<ls, mpoly>("LINESTRING(10 1,10 5,5 5)",
                            "MULTIPOLYGON(((0 0,0 10,10 10,10 0,0 0),(10 5,2 8,2 2,10 5)),((10 5,3 3,3 7,10 5)))",
                            "11F00F212");

    test_geometry<ls, mpoly>("LINESTRING(0 0,10 0,10 10,0 10,0 0)",
                             "MULTIPOLYGON(((0 0,0 10,10 10,10 0,0 0)),((20 0,20 10,30 20,20 0)))",
                             "F1FFFF212");

    // degenerated points
    test_geometry<ls, mpoly>("LINESTRING(5 5,10 10,10 10,10 10,15 15)",
                             "MULTIPOLYGON(((0 0,0 10,10 10,10 0,0 0)),((10 10,10 20,20 20,20 10,10 10)))",
                             "10F0FF212");

    // self-IP polygon with a hole and second polygon with a hole -> B to I to B to B to I to B to E
    test_geometry<ls, mpoly>("LINESTRING(0 0,3 3)",
                             "MULTIPOLYGON(((0 0,0 10,10 10,10 0,0 0),(0 0,9 1,9 9,1 9,0 0)),((0 0,2 8,8 8,8 2,0 0),(0 0,7 3,7 7,3 7,0 0)))",
                             "FF1F00212");
    // self-IP polygon with a hole and second polygon -> B to I to B to B to I
    test_geometry<ls, mpoly>("LINESTRING(0 0,3 3)",
                             "MULTIPOLYGON(((0 0,0 10,10 10,10 0,0 0),(0 0,9 1,9 9,1 9,0 0)),((0 0,2 8,8 8,8 2,0 0)))",
                             "1FF00F212");
    test_geometry<ls, mpoly>("LINESTRING(0 0,3 3)",
                             "MULTIPOLYGON(((0 0,2 8,8 8,8 2,0 0)),((0 0,0 10,10 10,10 0,0 0),(0 0,9 1,9 9,1 9,0 0)))",
                             "1FF00F212");
}

template <typename P>
void test_multi_linestring_multi_polygon()
{
    typedef bg::model::linestring<P> ls;
    typedef bg::model::polygon<P> poly;
    typedef bg::model::multi_linestring<ls> mls;
    typedef bg::model::multi_polygon<poly> mpoly;

    // polygons with some ring equal to the linestrings
    test_geometry<mls, mpoly>("MULTILINESTRING((0 0,10 0,10 10,0 10,0 0),(20 20,50 50,20 80,20 20))",
                              "MULTIPOLYGON(((0 0,0 10,10 10,10 0,0 0)))",
                              "F11FFF2F2");

    test_geometry<mls, mpoly>("MULTILINESTRING((0 0,10 0,10 10,0 10,0 0),(2 2,5 5,2 8,2 2))",
                              "MULTIPOLYGON(((0 0,0 10,10 10,10 0,0 0),(2 2,5 5,2 8,2 2)))",
                              "F1FFFF2F2");


    test_geometry<mls, mpoly>("MULTILINESTRING((0 0,10 0,10 10),(10 10,0 10,0 0))",
                              "MULTIPOLYGON(((0 0,0 10,10 10,10 0,0 0)))",
                              "F1FFFF2F2");
    test_geometry<mls, mpoly>("MULTILINESTRING((0 0,10 0,10 10),(10 10,0 10,0 0),(20 20,50 50,20 80,20 20))",
                              "MULTIPOLYGON(((0 0,0 10,10 10,10 0,0 0)))",
                              "F11FFF2F2");

    // disjoint
    test_geometry<mls, mpoly>("MULTILINESTRING((20 20,30 30),(30 30,40 40))",
                              "MULTIPOLYGON(((0 0,0 10,10 10,10 0,0 0)))",
                              "FF1FF0212");

    test_geometry<mls, mpoly>("MULTILINESTRING((5 5,0 5),(5 5,5 0),(10 10,10 5,5 5,5 10,10 10))",
                              "MULTIPOLYGON(((0 0,0 5,5 5,5 0,0 0)),((5 5,5 10,10 10,10 5,5 5)),((5 5,10 1,10 0,5 5)))",
                              "F1FF0F212");
    test_geometry<mls, mpoly>("MULTILINESTRING((5 5,0 5),(5 5,5 0),(0 5,0 0,5 0),(10 10,10 5,5 5,5 10,10 10))",
                              "MULTIPOLYGON(((0 0,0 5,5 5,5 0,0 0)),((5 5,5 10,10 10,10 5,5 5)),((5 5,10 1,10 0,5 5)))",
                              "F1FFFF212");
    test_geometry<mls, mpoly>("MULTILINESTRING((5 5,0 0),(5 5,5 0),(10 10,10 5,5 5,5 10,10 10))",
                              "MULTIPOLYGON(((0 0,0 5,5 5,5 0,0 0)),((5 5,5 10,10 10,10 5,5 5)),((5 5,10 1,10 0,5 5)))",
                              "11FF0F212");
}

template <typename P>
void test_all()
{
    test_linestring_polygon<P>();
    test_linestring_multi_polygon<P>();
    test_multi_linestring_multi_polygon<P>();
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
