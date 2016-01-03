// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2014-2015, Oracle and/or its affiliates.

// Licensed under the Boost Software License version 1.0.
// http://www.boost.org/users/license.html

// Contributed and/or modified by Menelaos Karavelas, on behalf of Oracle

#include <iostream>

#ifndef BOOST_TEST_MODULE
#define BOOST_TEST_MODULE test_difference_linear_linear
#endif

#ifdef BOOST_GEOMETRY_TEST_DEBUG
#define BOOST_GEOMETRY_DEBUG_TURNS
#define BOOST_GEOMETRY_DEBUG_SEGMENT_IDENTIFIER
#endif

#include <boost/test/included/unit_test.hpp>

#include "test_difference_linear_linear.hpp"

#include <boost/geometry/geometries/linestring.hpp>
#include <boost/geometry/multi/geometries/multi_linestring.hpp>
#include <boost/geometry/algorithms/difference.hpp>

typedef bg::model::point<double,2,bg::cs::cartesian>  point_type;
typedef bg::model::segment<point_type>                segment_type;
typedef bg::model::linestring<point_type>             linestring_type;
typedef bg::model::multi_linestring<linestring_type>  multi_linestring_type;



//===========================================================================
//===========================================================================
//===========================================================================


BOOST_AUTO_TEST_CASE( test_difference_linestring_linestring )
{
#ifdef BOOST_GEOMETRY_TEST_DEBUG
    std::cout << std::endl << std::endl << std::endl;
    std::cout << "*** LINESTRING / LINESTRING DIFFERENCE ***" << std::endl;
    std::cout << std::endl;
#endif

    typedef linestring_type L;
    typedef multi_linestring_type ML;

    typedef test_difference_of_geometries<L, L, ML> tester;

    tester::apply
        (from_wkt<L>("LINESTRING(0 0,1 1,2 1,3 2)"),
         from_wkt<L>("LINESTRING(0 2,1 1,2 1,3 0)"),
         from_wkt<ML>("MULTILINESTRING((0 0,1 1),(2 1,3 2))"),
         "lldf00");

    tester::apply
        (from_wkt<L>("LINESTRING(0 2,1 1,2 1,3 0)"),
         from_wkt<L>("LINESTRING(0 0,1 1,2 1,3 2)"),
         from_wkt<ML>("MULTILINESTRING((0 2,1 1),(2 1,3 0))"),
         "lldf00-1");

    tester::apply
        (from_wkt<L>("LINESTRING(0 0,5 0)"),
         from_wkt<L>("LINESTRING(3 0,4 0)"),
         from_wkt<ML>("MULTILINESTRING((0 0,3 0),(4 0,5 0))"),
         "lldf01");

    tester::apply
        (from_wkt<L>("LINESTRING(3 0,4 0)"),
         from_wkt<L>("LINESTRING(0 0,5 0)"),
         from_wkt<ML>("MULTILINESTRING()"),
         "lldf01-1");

    tester::apply
        (from_wkt<L>("LINESTRING(0 0,4 0)"),
         from_wkt<L>("LINESTRING(3 0,6 0)"),
         from_wkt<ML>("MULTILINESTRING((0 0,3 0))"),
         "lldf01-2");

    tester::apply
        (from_wkt<L>("LINESTRING(3 0,6 0)"),
         from_wkt<L>("LINESTRING(0 0,4 0)"),
         from_wkt<ML>("MULTILINESTRING((4 0,6 0))"),
         "lldf01-3");

    tester::apply
        (from_wkt<L>("LINESTRING(0 0,6 0)"),
         from_wkt<L>("LINESTRING(0 0,4 0)"),
         from_wkt<ML>("MULTILINESTRING((4 0,6 0))"),
         "lldf01-4");

    tester::apply
        (from_wkt<L>("LINESTRING(0 0,4 0)"),
         from_wkt<L>("LINESTRING(0 0,6 0)"),
         from_wkt<ML>("MULTILINESTRING()"),
         "lldf01-5");

    tester::apply
        (from_wkt<L>("LINESTRING(0 0,20 0)"),
         from_wkt<L>("LINESTRING(0 0,1 1,2 0,3 1,4 0,5 0,6 1,7 -1,8 0)"),
         from_wkt<ML>("MULTILINESTRING((0 0,4 0),(5 0,20 0))"),
         "lldf01-6");

    tester::apply
        (from_wkt<L>("LINESTRING(-20 0,20 0)"),
         from_wkt<L>("LINESTRING(0 0,1 1,2 0,3 1,4 0,5 0,6 1,7 -1,8 0)"),
         from_wkt<ML>("MULTILINESTRING((-20 0,4 0),(5 0,20 0))"),
         "lldf01-7");

    tester::apply
        (from_wkt<L>("LINESTRING(0 0,4 0)"),
         from_wkt<L>("LINESTRING(2 0,4 0)"),
         from_wkt<ML>("MULTILINESTRING((0 0,2 0))"),
         "lldf01-8");

    tester::apply
        (from_wkt<L>("LINESTRING(2 0,4 0)"),
         from_wkt<L>("LINESTRING(0 0,4 0)"),
         from_wkt<ML>("MULTILINESTRING()"),
         "lldf01-9");

    tester::apply
        (from_wkt<L>("LINESTRING(0 0,2 0)"),
         from_wkt<L>("LINESTRING(4 0,5 0)"),
         from_wkt<ML>("MULTILINESTRING((0 0,2 0))"),
         "lldf01-10");

    tester::apply
        (from_wkt<L>("LINESTRING(0 0,2 0)"),
         from_wkt<L>("LINESTRING(2 0,5 0)"),
         from_wkt<ML>("MULTILINESTRING((0 0,2 0))"),
         "lldf01-11");

    tester::apply
        (from_wkt<L>("LINESTRING(0 0,1 0,4 0)"),
         from_wkt<L>("LINESTRING(3 0,5 0)"),
         from_wkt<ML>("MULTILINESTRING((0 0,1 0,3 0))"),
         "lldf01-11a");

    tester::apply
        (from_wkt<L>("LINESTRING(0 0,1 0,4 0)"),
         from_wkt<L>("LINESTRING(3 0,4 0,5 0)"),
         from_wkt<ML>("MULTILINESTRING((0 0,1 0,3 0))"),
         "lldf01-11b");

    tester::apply
        (from_wkt<L>("LINESTRING(0 0,6 0)"),
         from_wkt<L>("LINESTRING(2 0,4 0,5 0)"),
         from_wkt<ML>("MULTILINESTRING((0 0,2 0),(5 0,6 0))"),
         "lldf01-12");

    tester::apply
        (from_wkt<L>("LINESTRING(0 0,1 0,5 5,10 5,15 0)"),
         from_wkt<L>("LINESTRING(-1 6,0 5,15 5)"),
         from_wkt<ML>("MULTILINESTRING((0 0,1 0,5 5),(10 5,15 0))"),
         "lldf02");

    tester::apply
        (from_wkt<L>("LINESTRING(-1 1,0 0,1 0,5 5,10 5,15 0,20 0)"),
         from_wkt<L>("LINESTRING(-1 0,30 0)"),
         from_wkt<ML>("MULTILINESTRING((-1 1,0 0),(1 0,5 5,10 5,15 0))"),
         "lldf03");

    tester::apply
        (from_wkt<L>("LINESTRING(0 0,1 0,5 5,10 5,15 0,20 0)"),
         from_wkt<L>("LINESTRING(-1 0,30 0)"),
         from_wkt<ML>("MULTILINESTRING((1 0,5 5,10 5,15 0))"),
         "lldf04");

    tester::apply
        (from_wkt<L>("LINESTRING(-1 1,0 0,1 0,5 5,10 5,15 0,20 0,25 1)"),
         from_wkt<L>("LINESTRING(-1 0,30 0)"),
         from_wkt<ML>("MULTILINESTRING((-1 1,0 0),(1 0,5 5,10 5,15 0),\
                      (20 0,25 1))"),
         "lldf05");

    tester::apply
        (from_wkt<L>("LINESTRING(-1 1,0 0,1 0,5 5,10 5,15 0,20 0,30 0)"),
         from_wkt<L>("LINESTRING(-1 0,30 0)"),
         from_wkt<ML>("MULTILINESTRING((-1 1,0 0),(1 0,5 5,10 5,15 0))"),
         "lldf05-1");

    tester::apply
        (from_wkt<L>("LINESTRING(-1 1,0 0,1 0,5 5,10 5,15 0,20 0,31 0)"),
         from_wkt<L>("LINESTRING(-1 0,30 0)"),
         from_wkt<ML>("MULTILINESTRING((-1 1,0 0),(1 0,5 5,10 5,15 0),\
                      (30 0,31 0))"),
         "lldf06");

    tester::apply
        (from_wkt<L>("LINESTRING(-1 1,0 0,1 0,5 5,10 5,15 0,20 0,31 0)"),
         from_wkt<L>("LINESTRING(-1 0,25 0,30 0)"),
         from_wkt<ML>("MULTILINESTRING((-1 1,0 0),(1 0,5 5,10 5,15 0),\
                      (30 0,31 0))"),
         "lldf07");

    tester::apply
        (from_wkt<L>("LINESTRING(-1 1,0 0,1 0,5 5,10 5,15 0,20 0,31 0)"),
         from_wkt<L>("LINESTRING(-1 0,19 0,30 0)"),
         from_wkt<ML>("MULTILINESTRING((-1 1,0 0),(1 0,5 5,10 5,15 0),\
                      (30 0,31 0))"),
         "lldf08");

    tester::apply
        (from_wkt<L>("LINESTRING(-1 1,0 0,1 0,5 5,10 5,15 0,20 0,30 0,31 1)"),
         from_wkt<L>("LINESTRING(-1 0,30 0)"),
         from_wkt<ML>("MULTILINESTRING((-1 1,0 0),(1 0,5 5,10 5,15 0),\
                      (30 0,31 1))"),
         "lldf09");

    tester::apply
        (from_wkt<L>("LINESTRING(-1 1,0 0,1 0,5 5,10 5,15 0,20 0,30 0,31 1)"),
         from_wkt<L>("LINESTRING(-1 -1,0 0,1 0,2 1,3 0,30 0)"),
         from_wkt<ML>("MULTILINESTRING((-1 1,0 0),(1 0,5 5,10 5,15 0),\
                      (30 0,31 1))"),
         "lldf10");

    tester::apply
        (from_wkt<L>("LINESTRING(-1 1,0 0,1 0,4 0,5 5,10 5,15 0,20 0,\
                                 30 0,31 1)"),
         from_wkt<L>("LINESTRING(-1 -1,0 0,1 0,2 0,2.5 1,3 0,30 0)"),
         from_wkt<ML>("MULTILINESTRING((-1 1,0 0),(2 0,3 0),\
                      (4 0,5 5,10 5,15 0),(30 0,31 1))"),
         "lldf11");

    tester::apply
        (from_wkt<L>("LINESTRING(-1 1,0 0,1 0,4 0,5 5,10 5,15 0,31 0)"),
         from_wkt<L>("LINESTRING(-1 -1,0 0,1 0,2 0,2.5 1,3 0,30 0)"),
         from_wkt<ML>("MULTILINESTRING((-1 1,0 0),(2 0,3 0),\
                      (4 0,5 5,10 5,15 0),(30 0,31 0))"),
         "lldf11-1");

    tester::apply
        (from_wkt<L>("LINESTRING(0 0,2 0,3 1)"),
         from_wkt<L>("LINESTRING(0 0,2 0,3 1)"),
         from_wkt<ML>("MULTILINESTRING()"),
         "lldf12");

    tester::apply
        (from_wkt<L>("LINESTRING(0 0,2 0,3 1)"),
         from_wkt<L>("LINESTRING(3 1,2 0,0 0)"),
         from_wkt<ML>("MULTILINESTRING()"),
         "lldf12-1");

   tester::apply
        (from_wkt<L>("LINESTRING(0 0,1 0,2 1,3 5,4 0)"),
         from_wkt<L>("LINESTRING(1 0,2 1,3 5,4 0,5 10)"),
         from_wkt<ML>("MULTILINESTRING((0 0,1 0))"),
         "lldf13");

    tester::apply
        (from_wkt<L>("LINESTRING(0 0,1 0,2 0,2.5 0,3 1)"),
         from_wkt<L>("LINESTRING(0 0,2 0,2.5 0,3 1)"),
         from_wkt<ML>("MULTILINESTRING()"),
         "lldf14");

    tester::apply
        (from_wkt<L>("LINESTRING(0 0,1 0,2 1,3 5,4 0)"),
         from_wkt<L>("LINESTRING(1 0,2 1,3 5)"),
         from_wkt<ML>("MULTILINESTRING((0 0,1 0),(3 5,4 0))"),
         "lldf15");

    tester::apply
        (from_wkt<L>("LINESTRING(0 0,1 0,2 1,3 2)"),
         from_wkt<L>("LINESTRING(0.5 0,1 0,3 2,4 5)"),
         from_wkt<ML>("MULTILINESTRING((0 0,0.5 0))"),
         "lldf16");

    tester::apply
        (from_wkt<L>("LINESTRING(0 0,1 0,2 1,3 2)"),
         from_wkt<L>("LINESTRING(4 5,3 2,1 0,0.5 0)"),
         from_wkt<ML>("MULTILINESTRING((0 0,0.5 0))"),
         "lldf16-r");

    tester::apply
        (from_wkt<L>("LINESTRING(0 0,10 0,20 1,30 1)"),
         from_wkt<L>("LINESTRING(1 1,2 0,3 1,20 1,25 1)"),
         from_wkt<ML>("MULTILINESTRING((0 0,10 0,20 1),(25 1,30 1))"),
         "lldf17");

    tester::apply
        (from_wkt<L>("LINESTRING(0 0,10 0,20 1,21 0,30 0)"),
         from_wkt<L>("LINESTRING(1 1,2 0,3 1,20 1,25 0)"),
         from_wkt<ML>("MULTILINESTRING((0 0,10 0,20 1,21 0,30 0))"),
         "lldf18");

    tester::apply
        (from_wkt<L>("LINESTRING(0 0,30 0)"),
         from_wkt<L>("LINESTRING(1 0,5 0,20 1,4 1,4 0,5 1)"),
         from_wkt<ML>("MULTILINESTRING((0 0,1 0),(5 0,30 0))"),
         "lldf19");

    tester::apply
        (from_wkt<L>("LINESTRING(0 0,30 0)"),
         from_wkt<L>("LINESTRING(5 1,4 0,4 1,20 1,5 0,1 0)"),
         from_wkt<ML>("MULTILINESTRING((0 0,1 0),(5 0,30 0))"),
         "lldf19-r");

    tester::apply
        (from_wkt<L>("LINESTRING(0 0,30 0)"),
         from_wkt<L>("LINESTRING(1 0,5 0,20 1,4 1,4 0)"),
         from_wkt<ML>("MULTILINESTRING((0 0,1 0),(5 0,30 0))"),
         "lldf19a");

    tester::apply
        (from_wkt<L>("LINESTRING(0 0,30 0)"),
         from_wkt<L>("LINESTRING(4 0,4 1,20 1,5 0,1 0)"),
         from_wkt<ML>("MULTILINESTRING((0 0,1 0),(5 0,30 0))"),
         "lldf19a-r");

    tester::apply
        (from_wkt<L>("LINESTRING(0 0,30 0)"),
         from_wkt<L>("LINESTRING(1 0,5 0,20 1,4 1,4 0,5 0)"),
         from_wkt<ML>("MULTILINESTRING((0 0,1 0),(5 0,30 0))"),
         "lldf19b");

    tester::apply
        (from_wkt<L>("LINESTRING(0 0,30 0)"),
         from_wkt<L>("LINESTRING(1 0,5 0,20 1,4 1,4 0,5 0,6 1)"),
         from_wkt<ML>("MULTILINESTRING((0 0,1 0),(5 0,30 0))"),
         "lldf19c");

    tester::apply
        (from_wkt<L>("LINESTRING(0 0,30 0)"),
         from_wkt<L>("LINESTRING(1 0,5 0,20 1,4 1,4 0,3 0)"),
         from_wkt<ML>("MULTILINESTRING((0 0,1 0),(5 0,30 0))"),
         "lldf19d");

    tester::apply
        (from_wkt<L>("LINESTRING(0 0,30 0)"),
         from_wkt<L>("LINESTRING(1 0,5 0,20 1,4 1,4 0,3 0,3 1)"),
         from_wkt<ML>("MULTILINESTRING((0 0,1 0),(5 0,30 0))"),
         "lldf19e");

    tester::apply
        (from_wkt<L>("LINESTRING(0 0,30 0)"),
         from_wkt<L>("LINESTRING(1 0,5 0,20 1,4 1,4 0,5 0,5 1)"),
         from_wkt<ML>("MULTILINESTRING((0 0,1 0),(5 0,30 0))"),
         "lldf19f");

    tester::apply
        (from_wkt<L>("LINESTRING(0 0,30 0)"),
         from_wkt<L>("LINESTRING(5 1,5 0,4 0,4 1,20 1,5 0,1 0)"),
         from_wkt<ML>("MULTILINESTRING((0 0,1 0),(5 0,30 0))"),
         "lldf19f-r");

    tester::apply
        (from_wkt<L>("LINESTRING(0 0,30 0)"),
         from_wkt<L>("LINESTRING(1 0,5 0,20 1,4 1,5 0,5 1)"),
         from_wkt<ML>("MULTILINESTRING((0 0,1 0),(5 0,30 0))"),
         "lldf19g");

    tester::apply
        (from_wkt<L>("LINESTRING(0 0,30 0)"),
         from_wkt<L>("LINESTRING(5 1,5 0,4 1,20 1,5 0,1 0)"),
         from_wkt<ML>("MULTILINESTRING((0 0,1 0),(5 0,30 0))"),
         "lldf19g-r");

    tester::apply
        (from_wkt<L>("LINESTRING(0 0,30 0,30 30,10 30,10 -10,15 0,40 0)"),
         from_wkt<L>("LINESTRING(5 5,10 0,10 30,20 0,25 0,25 25,50 0,35 0)"),
         from_wkt<ML>("MULTILINESTRING((0 0,20 0),(25 0,30 0,30 30,10 30),\
                       (10 0,10 -10,15 0,20 0),(25 0,35 0))"),
         "lldf20");

    tester::apply
        (from_wkt<L>("LINESTRING(0 0,30 0,30 30,10 30,10 -10,15 0,40 0)"),
         from_wkt<L>("LINESTRING(5 5,10 0,10 30,20 0,25 0,25 25,50 0,15 0)"),
         from_wkt<ML>("MULTILINESTRING((0 0,15 0),(30 0,30 30,10 30),\
                       (10 0,10 -10,15 0))"),
         "lldf20a");

    tester::apply
        (from_wkt<L>("LINESTRING(0 0,18 0,19 0,30 0)"),
         from_wkt<L>("LINESTRING(2 2,5 -1,15 2,18 0,20 0)"),
         from_wkt<ML>("MULTILINESTRING((0 0,18 0),(20 0,30 0))"),
         "lldf21"
         );

    tester::apply
        (from_wkt<L>("LINESTRING(2 2,5 -1,15 2,18 0,20 0)"),
         from_wkt<L>("LINESTRING(0 0,18 0,19 0,30 0)"),
         from_wkt<ML>("MULTILINESTRING((2 2,5 -1,15 2,18 0))"),
         "lldf21a"
         );

    tester::apply
        (from_wkt<L>("LINESTRING(0 0,10 0)"),
         from_wkt<L>("LINESTRING(1 0,4 0,2 1,5 1,4 0,8 0)"),
         from_wkt<ML>("MULTILINESTRING((0 0,1 0),(8 0,10 0))"),
         "lldf22"
         );

    tester::apply
        (from_wkt<L>("LINESTRING(0 0,10 0)"),
         from_wkt<L>("LINESTRING(4 0,5 0,5 1,1 1,1 0,4 0)"),
         from_wkt<ML>("MULTILINESTRING((0 0,1 0),(5 0,10 0))"),
         "lldf23"
         );

    // the following two tests have been discussed with by Adam
    tester::apply
        (from_wkt<L>("LINESTRING(1 0,1 1,2 1)"),
         from_wkt<L>("LINESTRING(2 1,1 1,1 0)"),
         from_wkt<ML>("MULTILINESTRING()"),
         "lldf24"
         );

    tester::apply
        (from_wkt<L>("LINESTRING(1 0,1 1,2 1)"),
         from_wkt<L>("LINESTRING(1 2,1 1,1 0)"),
         from_wkt<ML>("MULTILINESTRING((1 1,2 1))"),
         "lldf25"
         );

    tester::apply
        (from_wkt<L>("LINESTRING(6 0,0 0,5 0)"),
         from_wkt<L>("LINESTRING(2 0,-10 0)"),
         from_wkt<ML>("MULTILINESTRING((6 0,2 0),(2 0,5 0))"),
         "lldf27a"
         );

    tester::apply
        (from_wkt<L>("LINESTRING(6 0,0 0,5 0)"),
         from_wkt<L>("LINESTRING(2 0,-1 0,-10 0)"),
         from_wkt<ML>("MULTILINESTRING((6 0,2 0),(2 0,5 0))"),
         "lldf27b"
         );

    tester::apply
        (from_wkt<L>("LINESTRING(6 0,0 0,5 0)"),
         from_wkt<L>("LINESTRING(2 0,0 0,-10 0)"),
         from_wkt<ML>("MULTILINESTRING((6 0,2 0),(2 0,5 0))"),
         "lldf27c"
         );

    tester::apply
        (from_wkt<L>("LINESTRING(2 0,0 0,-10 0)"),
         from_wkt<L>("LINESTRING(6 0,0 0,5 0)"),
         from_wkt<ML>("MULTILINESTRING((0 0,-10 0))"),
         "lldf27d"
         );

    tester::apply
        (from_wkt<L>("LINESTRING(-3 6,-3 0,-3 5)"),
         from_wkt<L>("LINESTRING(-3 2,-3 0,-3 -10)"),
         from_wkt<ML>("MULTILINESTRING((-3 6,-3 2),(-3 2,-3 5))"),
         "lldf28a"
         );

    tester::apply
        (from_wkt<L>("LINESTRING(-3 2,-3 0,-3 -10)"),
         from_wkt<L>("LINESTRING(-3 6,-3 0,-3 5)"),
         from_wkt<ML>("MULTILINESTRING((-3 0,-3 -10))"),
         "lldf28b"
         );

    tester::apply
        (from_wkt<L>("LINESTRING(-3 6,-3 0,-3 5)"),
         from_wkt<L>("LINESTRING(-3 2,-3 0,-3 -10)"),
         from_wkt<ML>("MULTILINESTRING((-3 6,-3 2),(-3 2,-3 5))"),
         "lldf28c"
         );

    tester::apply
        (from_wkt<L>("LINESTRING(-7 -8,3 0,4 -1)"),
         from_wkt<L>("LINESTRING(-5 -4,3 0,4 -1,7 -4)"),
         from_wkt<ML>("MULTILINESTRING((-7 -8,3 0))"),
         "lldf29a"
         );

    tester::apply
        (from_wkt<L>("LINESTRING(-7 -8,3 0,4 -1,-7 10)"),
         from_wkt<L>("LINESTRING(-5 -4,3 0,4 -1,2 -1)"),
         from_wkt<ML>("MULTILINESTRING((-7 -8,3 0),(3 0,-7 10))"),
         "lldf29b"
         );

    tester::apply
        (from_wkt<L>("LINESTRING(-7 -8,3 0,4 -1,-7 10)"),
         from_wkt<L>("LINESTRING(-5 -4,3 0,4 -1,7 -4,2 -1)"),
         from_wkt<ML>("MULTILINESTRING((-7 -8,3 0),(3 0,-7 10))"),
         "lldf29c"
         );

    tester::apply
        (from_wkt<L>("LINESTRING(-5 -4,3 0,4 -1,7 -4,2 -1)"),
         from_wkt<L>("LINESTRING(-7 -8,3 0,4 -1,-7 10)"),
         from_wkt<ML>("MULTILINESTRING((-5 -4,3 0),(4 -1,7 -4,2 -1))"),
         "lldf29c-r"
         );

    tester::apply
        (from_wkt<L>("LINESTRING(-2 -2,-4 0,1 -8,-2 6,8 5,-7 -8,\
                     3 0,4 -1,-7 10,-4 10)"),
         from_wkt<L>("LINESTRING(-5 -4,3 0,4 -1,7 -4,2 -1,-4 -1,-2 6)"),
         from_wkt<ML>("MULTILINESTRING((-2 -2,-4 0,1 -8,-2 6,8 5,-7 -8,\
                      3 0),(3 0,-7 10,-4 10))"),
         "lldf29d"
         );

#ifdef GEOMETRY_TEST_INCLUDE_FAILING_TESTS
     tester::apply
         (from_wkt<L>("LINESTRING(8 5,5 1,-2 3,1 10)"),
          from_wkt<L>("LINESTRING(1.9375 1.875,\
                      1.7441860465116283 1.9302325581395348,\
                      -0.7692307692307692 2.6483516483516487,\
                      -2 3,-1.0071942446043165 5.316546762589928)"),
          from_wkt<ML>("MULTILINESTRING((8 5,5 1,-2 3,1 10))"),
          "lldf30a"
          );

     tester::apply
         (from_wkt<L>("LINESTRING(1.9375 1.875,\
                      1.7441860465116283 1.9302325581395348,\
                      -0.7692307692307692 2.6483516483516487,\
                      -2 3,-1.0071942446043165 5.316546762589928)"),
          from_wkt<L>("LINESTRING(8 5,5 1,-2 3,1 10)"),
          from_wkt<ML>("MULTILINESTRING((1.9375 1.875,\
                       1.7441860465116283 1.9302325581395348,\
                       -0.7692307692307692 2.6483516483516487,\
                       -2 3,-1.0071942446043165 5.316546762589928))"),
          "lldf30b"
          );

   tester::apply
        (from_wkt<L>("LINESTRING(5 -8,-7 -6,-3 6,-3 1,-5 4,-1 0,8 5,\
                     5 1,-2 3,1 10,8 5,6 2,7 4)"),
         from_wkt<L>("LINESTRING(1.9375 1.875,\
                     1.7441860465116283 1.9302325581395348,\
                     -0.7692307692307692 2.6483516483516487,\
                     -2 3,-1.0071942446043165 5.316546762589928)"),
         from_wkt<ML>("MULTILINESTRING((5 -8,-7 -6,-3 6,-3 1,-5 4,-1 0,8 5,\
                      5 1,-2 3,1 10,8 5,6 2,7 4))"),

         "lldf30c"
         );
#endif

    tester::apply
        (from_wkt<L>("LINESTRING(8 1, 4 .4)"),
         from_wkt<L>("LINESTRING(0 -.2, 8 1)"),
         from_wkt<ML>("MULTILINESTRING()"),
         "lldf31s"
         );

    tester::apply
        (from_wkt<L>("LINESTRING(8 1, 4 .4,2 8)"),
         from_wkt<L>("LINESTRING(0 -.2, 8 1)"),
         from_wkt<ML>("MULTILINESTRING((4 .4,2 8))"),
         "lldf31x"
         );

    tester::apply
        (from_wkt<L>("LINESTRING(2 8,4 .4,8 1)"),
         from_wkt<L>("LINESTRING(0 -.2, 8 1)"),
         from_wkt<ML>("MULTILINESTRING((2 8,4 .4))"),
         "lldf31x-r"
         );

    tester::apply
        (from_wkt<L>("LINESTRING(0 5, 8 1, 4 .4, 2 8)"),
         from_wkt<L>("LINESTRING(0 -.2, 8 1, -.5 7)"),
         from_wkt<ML>("MULTILINESTRING((0 5,8 1),(4 .4,2 8))"),
         "lldf31y",
         1e-10
         );

    tester::apply
        (from_wkt<L>("LINESTRING(0 -.2, 8 1, -.5 7)"),
         from_wkt<L>("LINESTRING(0 5, 8 1, 4 .4, 2 8)"),
         from_wkt<ML>("MULTILINESTRING((0 -.2,4 .4),(8 1,-.5 7))"),
         "lldf31y-r",
         1e-10
         );

    tester::apply
        (from_wkt<L>("LINESTRING(0 5, 8 1, 4 .4, 2 8)"),
         from_wkt<L>("LINESTRING(0 -.2, 8 1, -.5 7, 6 +.2)"),
         from_wkt<ML>("MULTILINESTRING((0 5,8 1),(4 .4,2 8))"),
         "lldf31y+",
         1e-10
         );

    tester::apply
        (from_wkt<L>("LINESTRING(10.0002 2,9 -1032.34324, .3 8, 0 5, 8 1, 4 .4, 2 8)"),
         from_wkt<L>("LINESTRING(0 -.2, 8 1, -.5 7, 6 +.2)"),
         from_wkt<ML>("MULTILINESTRING((10.0002 2,9 -1032.34324,.3 8,0 5,8 1),(4 .4,2 8))"),
         "lldf31z",
         1e-10
         );

    tester::apply
        (from_wkt<L>("LINESTRING(0 -.2, 8 1, -.5 7, 6 +.2)"),
         from_wkt<L>("LINESTRING(10.0002 2,9 -1032.34324, .3 8, 0 5, 8 1, 4 .4, 2 8)"),
         from_wkt<ML>("MULTILINESTRING((0 -.2,4 .4),(8 1,-.5 7,6 .2))"),
         "lldf31z-r",
         1e-10
         );

    tester::apply
        (from_wkt<L>("LINESTRING(0 0, 8 1, -.5 7)"),
         from_wkt<L>("LINESTRING(0 5, 8 1, 4 .5, 2 8)"),
         from_wkt<ML>("MULTILINESTRING((0 0,4 .5),(8 1,-.5 7))"),
         "lldf32"
         );
}



BOOST_AUTO_TEST_CASE( test_difference_linestring_multilinestring )
{
#ifdef BOOST_GEOMETRY_TEST_DEBUG
    std::cout << std::endl << std::endl << std::endl;
    std::cout << "*** LINESTRING / MULTILINESTRING DIFFERENCE ***"
              << std::endl;
    std::cout << std::endl;
#endif

    typedef linestring_type L;
    typedef multi_linestring_type ML;

    typedef test_difference_of_geometries<L, ML, ML> tester;

    // disjoint linestrings
    tester::apply
        (from_wkt<L>("LINESTRING(0 0,10 0,20 1)"),
         from_wkt<ML>("MULTILINESTRING((1 1,2 2,4 3),(1 1,2 2,5 3))"),
         from_wkt<ML>("MULTILINESTRING((0 0,10 0,20 1))"),
         "lmldf01"
        );

    tester::apply
        (from_wkt<L>("LINESTRING(0 0,10 0,20 1)"),
         from_wkt<ML>("MULTILINESTRING((1 1,2 0,4 0),(1 1,3 0,4 0))"),
         from_wkt<ML>("MULTILINESTRING((0 0,2 0),(4 0,10 0,20 1))"),
         "lmldf02"
        );

    tester::apply
        (from_wkt<L>("LINESTRING(0 0,10 0,20 1)"),
         from_wkt<ML>("MULTILINESTRING((1 1,2 0,4 0),(1 1,3 0,5 0))"),
         from_wkt<ML>("MULTILINESTRING((0 0,2 0),(5 0,10 0,20 1))"),
         "lmldf03"
        );

    tester::apply
        (from_wkt<L>("LINESTRING(0 0,10 0,20 1)"),
         from_wkt<ML>("MULTILINESTRING((1 1,2 0,4 0))"),
         from_wkt<ML>("MULTILINESTRING((0 0,2 0),(4 0,10 0,20 1))"),
         "lmldf04"
        );

    tester::apply
        (from_wkt<L>("LINESTRING(0 0,101 0)"),
         from_wkt<ML>("MULTILINESTRING((-1 -1,1 0,101 0,200 -1))"),
         from_wkt<ML>("MULTILINESTRING((0 0,1 0))"),
         "lmldf07"
        );

    tester::apply
        (from_wkt<L>("LINESTRING(-1 1,0 0,101 0)"),
         from_wkt<ML>("MULTILINESTRING((-1 -1,0 0,50 0),\
                      (19 -1,20 0,101 0,200 -1))"),
         from_wkt<ML>("MULTILINESTRING((-1 1,0 0))"),
         "lmldf07a"
        );

    tester::apply
        (from_wkt<L>("LINESTRING(0 0,101 0)"),
         from_wkt<ML>("MULTILINESTRING((-1 -1,0 0,50 0),\
                      (19 -1,20 0,101 0,200 -1))"),
         from_wkt<ML>("MULTILINESTRING()"),
         "lmldf07b"
        );

    tester::apply
        (from_wkt<L>("LINESTRING(0 0,101 0)"),
         from_wkt<ML>("MULTILINESTRING((0 1,1 1,2 0),\
                       (-1 -1,1 0,101 0,200 -1))"),
         from_wkt<ML>("MULTILINESTRING((0 0,1 0))"),
         "lmldf08"
        );

    tester::apply
        (from_wkt<L>("LINESTRING(0 0,1 0,2 0.5,3 0,101 0)"),
         from_wkt<ML>("MULTILINESTRING((0 1,1 1,2 0.5),\
                       (-1 -1,1 0,101 0,200 -1))"),
         from_wkt<ML>("MULTILINESTRING((0 0,1 0,2 0.5,3 0))"),
         "lmldf09"
        );

    tester::apply
        (from_wkt<L>("LINESTRING(0 0,1 0,1.5 0,2 0.5,3 0,101 0)"),
         from_wkt<ML>("MULTILINESTRING((0 1,1 1,1 0,2 0.5),\
                       (-1 -1,1 0,101 0,200 -1))"),
         from_wkt<ML>("MULTILINESTRING((0 0,1 0),(1.5 0,2 0.5,3 0))"),
         "lmldf10"
        );

    tester::apply
        (from_wkt<L>("LINESTRING(0 0,20 0)"),
         from_wkt<ML>("MULTILINESTRING((0 1,1 0,19 0,20 1),\
                      (1 1,2 0,18 0,19 1),(2 1,3 0,17 0,18 1),\
                      (3 1,4 0,16 0,17 1))"),
         from_wkt<ML>("MULTILINESTRING((0 0,1 0),(19 0,20 0))"),
         "lmldf12"
         );

    tester::apply
        (from_wkt<L>("LINESTRING(0 0,20 0)"),
         from_wkt<ML>("MULTILINESTRING((1 0,19 0,20 1),\
                      (2 0,18 0,19 1),(3 0,17 0,18 1),\
                      (4 0,16 0,17 1))"),
         from_wkt<ML>("MULTILINESTRING((0 0,1 0),(19 0,20 0))"),
         "lmldf13"
         );

    tester::apply
        (from_wkt<L>("LINESTRING(0 0,20 0)"),
         from_wkt<ML>("MULTILINESTRING((0 1,1 0,19 0,20 1,19 1,18 0,2 0,\
                       1 1,2 1,3 0,17 0,18 1,17 1,16 0,4 0,3 1))"),
         from_wkt<ML>("MULTILINESTRING((0 0,1 0),(19 0,20 0))"),
         "lmldf14"
         );

    tester::apply
        (from_wkt<L>("LINESTRING(0 0,20 0)"),
         from_wkt<ML>("MULTILINESTRING((0 1,1 0,19 0,20 1),\
                       (2 2,4 2,6 0))"),
         from_wkt<ML>("MULTILINESTRING((0 0,1 0),(19 0,20 0))"),
         "lmldf15"
         );

    tester::apply
        (from_wkt<L>("LINESTRING(0 0,20 0)"),
         from_wkt<ML>("MULTILINESTRING((0 1,1 0,19 0,20 1),\
                       (6 0,4 2,2 2))"),
         from_wkt<ML>("MULTILINESTRING((0 0,1 0),(19 0,20 0))"),
         "lmldf15a"
         );

    tester::apply
        (from_wkt<L>("LINESTRING(0 0,20 0)"),
         from_wkt<ML>("MULTILINESTRING((0 1,1 0,19 0,20 1),\
                       (2 2,4 2,5 0,6 0))"),
         from_wkt<ML>("MULTILINESTRING((0 0,1 0),(19 0,20 0))"),
         "lmldf16"
         );

    tester::apply
        (from_wkt<L>("LINESTRING(0 0,20 0)"),
         from_wkt<ML>("MULTILINESTRING((0 1,1 0,19 0,20 1),\
                       (6 0,5 0,4 2,2 2))"),
         from_wkt<ML>("MULTILINESTRING((0 0,1 0),(19 0,20 0))"),
         "lmldf16a"
         );

    tester::apply
        (from_wkt<L>("LINESTRING(0 0,30 0)"),
         from_wkt<ML>("MULTILINESTRING((0 1,1 0,19 0,20 1),\
                       (2 2,4 0,5 2,20 2,25 0))"),
         from_wkt<ML>("MULTILINESTRING((0 0,1 0),(19 0,30 0))"),
         "lmldf17"
         );

    tester::apply
        (from_wkt<L>("LINESTRING(0 0,30 0)"),
         from_wkt<ML>("MULTILINESTRING((0 1,1 0,19 0,20 1),\
                       (2 2,4 0,5 2,20 2,25 0,26 2))"),
         from_wkt<ML>("MULTILINESTRING((0 0,1 0),(19 0,30 0))"),
         "lmldf17a"
         );

    tester::apply
        (from_wkt<L>("LINESTRING(0 0,30 0)"),
         from_wkt<ML>("MULTILINESTRING((0 1,1 0,19 0,20 1),\
                       (2 2,5 -1,15 2,18 0))"),
         from_wkt<ML>("MULTILINESTRING((0 0,1 0),(19 0,30 0))"),
         "lmldf18"
         );

    tester::apply
        (from_wkt<L>("LINESTRING(0 0,18 0,19 0,30 0)"),
         from_wkt<ML>("MULTILINESTRING((0 1,1 0,19 0,20 1),\
                       (2 2,5 -1,15 2,18 0))"),
         from_wkt<ML>("MULTILINESTRING((0 0,1 0),(19 0,30 0))"),
         "lmldf18a"
         );

    tester::apply
        (from_wkt<L>("LINESTRING(0 0,18 0,19 0,30 0)"),
         from_wkt<ML>("MULTILINESTRING((0 1,1 0,19 0,20 1),\
                       (2 2,5 -1,15 2,18 0,20 0))"),
         from_wkt<ML>("MULTILINESTRING((0 0,1 0),(20 0,30 0))"),
         "lmldf18b"
         );

    tester::apply
        (from_wkt<L>("LINESTRING(0 0,18 0,19 0,30 0)"),
         from_wkt<ML>("MULTILINESTRING((0 1,1 0,19 0,20 1),\
                       (2 2,5 -1,15 2,25 0,26 0))"),
         from_wkt<ML>("MULTILINESTRING((0 0,1 0),(19 0,25 0),(26 0,30 0))"),
         "lmldf18c"
         );

    tester::apply
        (from_wkt<L>("LINESTRING(0 0,18 0,19 0,30 0)"),
         from_wkt<ML>("MULTILINESTRING((0 1,1 0,19 0,20 1),\
                       (2 2,5 -1,15 2,25 0,21 0))"),
         from_wkt<ML>("MULTILINESTRING((0 0,1 0),(19 0,21 0),(25 0,30 0))"),
         "lmldf18d"
         );

    tester::apply
        (from_wkt<L>("LINESTRING(0 0,10 0)"),
         from_wkt<ML>("MULTILINESTRING((0 5,1 0,9 0,10 5),(0 1,2 0,3 1),\
                      (0 -2,3 0,4 4),(0 -5,4 0,5 0,6 3))"),
         from_wkt<ML>("MULTILINESTRING((0 0,1 0),(9 0,10))"),
         "lmldf19"
         );

    tester::apply
        (from_wkt<L>("LINESTRING(0 0,10 0)"),
         from_wkt<ML>("MULTILINESTRING((-1 0,0 0),(10 0,12 0))"),
         from_wkt<ML>("MULTILINESTRING((0 0,10 0))"),
         "lmldf20"
         );
}





BOOST_AUTO_TEST_CASE( test_difference_multilinestring_linestring )
{
#ifdef BOOST_GEOMETRY_TEST_DEBUG
    std::cout << std::endl << std::endl << std::endl;
    std::cout << "*** MULTILINESTRING / LINESTRING DIFFERENCE ***"
              << std::endl;
    std::cout << std::endl;
#endif

    typedef linestring_type L;
    typedef multi_linestring_type ML;

    typedef test_difference_of_geometries<ML, L, ML> tester;

    // disjoint linestrings
    tester::apply
        (from_wkt<ML>("MULTILINESTRING((0 0,10 0,20 1),(1 0,7 0))"),
         from_wkt<L>("LINESTRING(1 1,2 2,4 3)"),
         from_wkt<ML>("MULTILINESTRING((0 0,10 0,20 1),(1 0,7 0))"),
         "mlldf01"
        );

    tester::apply
        (from_wkt<ML>("MULTILINESTRING((0 0,10 0,20 1),(1 0,7 0))"),
         from_wkt<L>("LINESTRING(1 1,2 0,4 0)"),
         from_wkt<ML>("MULTILINESTRING((0 0,2 0),(4 0,10 0,20 1),\
                      (1 0,2 0),(4 0,7 0))"),
         "mlldf02"
        );

    tester::apply
        (from_wkt<ML>("MULTILINESTRING((0 0,101 0))"),
         from_wkt<L>("LINESTRING(-1 -1,1 0,101 0,200 -1)"),
         from_wkt<ML>("MULTILINESTRING((0 0,1 0))"),
         "mlldf03"
        );

    tester::apply
        (from_wkt<ML>("MULTILINESTRING((0 0,20 0))"),
         from_wkt<L>("LINESTRING(0 1,1 0,19 0,20 1,19 1,18 0,2 0,\
                       1 1,2 1,3 0,17 0,18 1,17 1,16 0,4 0,3 1)"),
         from_wkt<ML>("MULTILINESTRING((0 0,1 0),(19 0,20 0))"),
         "mlldf04"
         );

    tester::apply
        (from_wkt<ML>("MULTILINESTRING((-1 0,-1 10),(0 0,20 0),(25 0,30 0))"),
         from_wkt<L>("LINESTRING(0 1,1 0,19 0,20 1,19 1,18 0,2 0,\
                       1 1,2 1,3 0,17 0,18 1,17 1,16 0,4 0,3 1)"),
         from_wkt<ML>("MULTILINESTRING((-1 0,-1 10),(0 0,1 0),(19 0,20 0),(25 0,30 0))"),
         "mlldf05"
         );

    tester::apply
        (from_wkt<ML>("MULTILINESTRING((-3 2,-3 0,-3 -10))"),
         from_wkt<L>("LINESTRING(-3 6,-3 0,-3 5)"),
         from_wkt<ML>("MULTILINESTRING((-3 0,-3 -10))"),
         "mlldf06a"
         );

    tester::apply
        (from_wkt<ML>("MULTILINESTRING((6 6,-3 2,-3 0,-3 -10,9 -2))"),
         from_wkt<L>("LINESTRING(-3 6,-3 0,-3 5,2 -3,-6 10,5 0,2 8,\
                     -6 1,10 -6)"),
         from_wkt<ML>("MULTILINESTRING((6 6,-3 2),(-3 0,-3 -10,9 -2))"),
         "mlldf06b"
         );

    tester::apply
        (from_wkt<ML>("MULTILINESTRING((0 -3,5 4,6 6,-3 2,-3 0,-3 -10,\
                      9 -2,9 5,5 -5,-4 -8,9 0))"),
         from_wkt<L>("LINESTRING(-3 6,-3 0,-3 5,2 -3,-6 10,5 0,2 8,\
                     -6 1,10 -6)"),
         from_wkt<ML>("MULTILINESTRING((0 -3,5 4,6 6,-3 2),\
                      (-3 0,-3 -10,9 -2,9 5,5 -5,-4 -8,9 0))"),
         "mlldf06c"
         );
}







BOOST_AUTO_TEST_CASE( test_difference_multilinestring_multilinestring )
{
#ifdef BOOST_GEOMETRY_TEST_DEBUG
    std::cout << std::endl << std::endl << std::endl;
    std::cout << "*** MULTILINESTRING / MULTILINESTRING DIFFERENCE ***"
              << std::endl;
    std::cout << std::endl;
#endif

    typedef multi_linestring_type ML;

    typedef test_difference_of_geometries<ML, ML, ML> tester;

    // disjoint linestrings
    tester::apply
        (from_wkt<ML>("MULTILINESTRING((0 0,10 0,20 1),(1 0,7 0))"),
         from_wkt<ML>("MULTILINESTRING((1 1,2 2,4 3),(1 1,2 2,5 3))"),
         from_wkt<ML>("MULTILINESTRING((0 0,10 0,20 1),(1 0,7 0))"),
         "mlmldf01"
        );

    tester::apply
        (from_wkt<ML>("MULTILINESTRING((0 0,10 0,20 1),(1 0,7 0))"),
         from_wkt<ML>("MULTILINESTRING((1 1,2 0,4 0),(1 1,3 0,4 0))"),
         from_wkt<ML>("MULTILINESTRING((0 0,2 0),(4 0,10 0,20 1),\
                      (1 0,2 0),(4 0,7 0))"),
         "mlmldf02"
        );

    tester::apply
        (from_wkt<ML>("MULTILINESTRING((0 0,10 0,20 1),(1 0,7 0))"),
         from_wkt<ML>("MULTILINESTRING((1 1,2 0,4 0),(1 1,3 0,5 0))"),
         from_wkt<ML>("MULTILINESTRING((0 0,2 0),(5 0,10 0,20 1),\
                      (1 0,2 0),(5 0,7 0))"),
         "mlmldf03"
        );

    tester::apply
        (from_wkt<ML>("MULTILINESTRING((0 0,10 0,20 1),(1 0,7 0))"),
         from_wkt<ML>("MULTILINESTRING((1 1,2 0,4 0))"),
         from_wkt<ML>("MULTILINESTRING((0 0,2 0),(4 0,10 0,20 1),\
                      (1 0,2 0),(4 0,7 0))"),
         "mlmldf04"
        );

    tester::apply
        (from_wkt<ML>("MULTILINESTRING((0 0,10 0,20 1),(1 0,7 0),\
                       (10 10,20 10,30 20))"),
         from_wkt<ML>("MULTILINESTRING((1 1,2 0,4 0),\
                       (10 20,15 10,25 10,30 15))"),
         from_wkt<ML>("MULTILINESTRING((0 0,2 0),(4 0,10 0,20 1),\
                      (1 0,2 0),(4 0,7 0),(10 10,15 10),(20 10,30 20))"),
         "mlmldf05"
        );

    tester::apply
        (from_wkt<ML>("MULTILINESTRING((0 0,10 0,20 10),(1 0,7 0),\
                       (10 10,20 10,30 20))"),
         from_wkt<ML>("MULTILINESTRING((1 1,2 0,4 0),\
                       (-1 -1,0 0,9 0,11 10,12 10,13 3,14 4,15 5),\
                       (10 20,15 10,25 10,30 15))"),
         from_wkt<ML>("MULTILINESTRING((9 0,10 0,13 3),(15 5,20 10),\
                      (10 10,11 10),(12 10,15 10),(20 10,30 20))"),
         "mlmldf06"
        );

    tester::apply
        (from_wkt<ML>("MULTILINESTRING((1 1,2 0,4 0),\
                      (-1 -1,0 0,9 0,11 10,12 10,13 3,14 4,15 5),\
                      (10 20,15 10,25 10,30 15))"),
         from_wkt<ML>("MULTILINESTRING((0 0,10 0,20 10),(1 0,7 0),\
                      (10 10,20 10,30 20))"),
         from_wkt<ML>("MULTILINESTRING((1 1,2 0),(-1 -1,0 0),(9 0,11 10),\
                      (12 10,13 3),(10 20,15 10),(20 10,25 10,30 15))"),
         "mlmldf06a"
        );

    tester::apply
        (from_wkt<ML>("MULTILINESTRING((0 0,101 0))"),
         from_wkt<ML>("MULTILINESTRING((-1 -1,1 0,101 0,200 -1))"),
         from_wkt<ML>("MULTILINESTRING((0 0,1 0))"),
         "mlmldf07"
        );

    tester::apply
        (from_wkt<ML>("MULTILINESTRING((-1 1,0 0,101 0))"),
         from_wkt<ML>("MULTILINESTRING((-1 -1,0 0,50 0),\
                      (19 -1,20 0,101 0,200 -1))"),
         from_wkt<ML>("MULTILINESTRING((-1 1,0 0))"),
         "mlmldf07a"
        );

    tester::apply
        (from_wkt<ML>("MULTILINESTRING((0 0,101 0))"),
         from_wkt<ML>("MULTILINESTRING((-1 -1,0 0,50 0),\
                      (19 -1,20 0,101 0,200 -1))"),
         from_wkt<ML>("MULTILINESTRING()"),
         "mlmldf07b"
        );

    tester::apply
        (from_wkt<ML>("MULTILINESTRING((0 0,101 0))"),
         from_wkt<ML>("MULTILINESTRING((0 1,1 1,2 0),\
                       (-1 -1,1 0,101 0,200 -1))"),
         from_wkt<ML>("MULTILINESTRING((0 0,1 0))"),
         "mlmldf08"
        );

    tester::apply
        (from_wkt<ML>("MULTILINESTRING((0 0,1 0,2 0.5,3 0,101 0))"),
         from_wkt<ML>("MULTILINESTRING((0 1,1 1,2 0.5),\
                       (-1 -1,1 0,101 0,200 -1))"),
         from_wkt<ML>("MULTILINESTRING((0 0,1 0,2 0.5,3 0))"),
         "mlmldf09"
        );

    tester::apply
        (from_wkt<ML>("MULTILINESTRING((0 0,1 0,1.5 0,2 0.5,3 0,101 0))"),
         from_wkt<ML>("MULTILINESTRING((0 1,1 1,1 0,2 0.5),\
                       (-1 -1,1 0,101 0,200 -1))"),
         from_wkt<ML>("MULTILINESTRING((0 0,1 0),(1.5 0,2 0.5,3 0))"),
         "mlmldf10"
        );

    tester::apply
        (from_wkt<ML>("MULTILINESTRING((0 0,1 1,100 1,101 0),\
                       (0 0,101 0))"),
         from_wkt<ML>("MULTILINESTRING((1 0,1 1,2 1,3 0,4 0,5 1,6 1,\
                       7 0,8 0,9 1,10 1,11 0,12 0,13 1,14 1,15 0),\
                       (-1 -1,1 0,101 0,200 -1))"),
         from_wkt<ML>("MULTILINESTRING((0 0,1 1),(2 1,5 1),(6 1,9 1),\
                       (10 1,13 1),(14 1,100 1,101 0),(0 0,1 0))"),
         "mlmldf11"
        );

    tester::apply
        (from_wkt<ML>("MULTILINESTRING((0 0,20 0))"),
         from_wkt<ML>("MULTILINESTRING((0 1,1 0,19 0,20 1),\
                      (1 1,2 0,18 0,19 1),(2 1,3 0,17 0,18 1),\
                      (3 1,4 0,16 0,17 1))"),
         from_wkt<ML>("MULTILINESTRING((0 0,1 0),(19 0,20 0))"),
         "mlmldf12"
         );

    tester::apply
        (from_wkt<ML>("MULTILINESTRING((0 0,20 0))"),
         from_wkt<ML>("MULTILINESTRING((1 0,19 0,20 1),\
                      (2 0,18 0,19 1),(3 0,17 0,18 1),\
                      (4 0,16 0,17 1))"),
         from_wkt<ML>("MULTILINESTRING((0 0,1 0),(19 0,20 0))"),
         "mlmldf13"
         );

    tester::apply
        (from_wkt<ML>("MULTILINESTRING((0 0,20 0))"),
         from_wkt<ML>("MULTILINESTRING((0 1,1 0,19 0,20 1,19 1,18 0,2 0,\
                       1 1,2 1,3 0,17 0,18 1,17 1,16 0,4 0,3 1))"),
         from_wkt<ML>("MULTILINESTRING((0 0,1 0),(19 0,20 0))"),
         "mlmldf14"
         );

    tester::apply
        (from_wkt<ML>("MULTILINESTRING((0 0,20 0))"),
         from_wkt<ML>("MULTILINESTRING((0 1,1 0,19 0,20 1),\
                       (2 2,4 2,6 0))"),
         from_wkt<ML>("MULTILINESTRING((0 0,1 0),(19 0,20 0))"),
         "mlmldf15"
         );

    tester::apply
        (from_wkt<ML>("MULTILINESTRING((0 0,20 0))"),
         from_wkt<ML>("MULTILINESTRING((0 1,1 0,19 0,20 1),\
                       (6 0,4 2,2 2))"),
         from_wkt<ML>("MULTILINESTRING((0 0,1 0),(19 0,20 0))"),
         "mlmldf15a"
         );

    tester::apply
        (from_wkt<ML>("MULTILINESTRING((0 0,20 0))"),
         from_wkt<ML>("MULTILINESTRING((0 1,1 0,19 0,20 1),\
                       (2 2,4 2,5 0,6 0))"),
         from_wkt<ML>("MULTILINESTRING((0 0,1 0),(19 0,20 0))"),
         "mlmldf16"
         );

    tester::apply
        (from_wkt<ML>("MULTILINESTRING((0 0,20 0))"),
         from_wkt<ML>("MULTILINESTRING((0 1,1 0,19 0,20 1),\
                       (6 0,5 0,4 2,2 2))"),
         from_wkt<ML>("MULTILINESTRING((0 0,1 0),(19 0,20 0))"),
         "mlmldf16a"
         );

    tester::apply
        (from_wkt<ML>("MULTILINESTRING((0 0,30 0))"),
         from_wkt<ML>("MULTILINESTRING((0 1,1 0,19 0,20 1),\
                       (2 2,4 0,5 2,20 2,25 0))"),
         from_wkt<ML>("MULTILINESTRING((0 0,1 0),(19 0,30 0))"),
         "mlmldf17"
         );

    tester::apply
        (from_wkt<ML>("MULTILINESTRING((0 0,30 0))"),
         from_wkt<ML>("MULTILINESTRING((0 1,1 0,19 0,20 1),\
                       (2 2,4 0,5 2,20 2,25 0,26 2))"),
         from_wkt<ML>("MULTILINESTRING((0 0,1 0),(19 0,30 0))"),
         "mlmldf17a"
         );

    tester::apply
        (from_wkt<ML>("MULTILINESTRING((0 0,30 0))"),
         from_wkt<ML>("MULTILINESTRING((0 1,1 0,19 0,20 1),\
                       (2 2,5 -1,15 2,18 0))"),
         from_wkt<ML>("MULTILINESTRING((0 0,1 0),(19 0,30 0))"),
         "mlmldf18"
         );

    tester::apply
        (from_wkt<ML>("MULTILINESTRING((0 0,18 0,19 0,30 0))"),
         from_wkt<ML>("MULTILINESTRING((0 1,1 0,19 0,20 1),\
                       (2 2,5 -1,15 2,18 0))"),
         from_wkt<ML>("MULTILINESTRING((0 0,1 0),(19 0,30 0))"),
         "mlmldf18a"
         );

    tester::apply
        (from_wkt<ML>("MULTILINESTRING((0 0,10 0))"),
         from_wkt<ML>("MULTILINESTRING((-1 0,0 0),(10 0,12 0))"),
         from_wkt<ML>("MULTILINESTRING((0 0,10 0))"),
         "mlmldf19"
         );

    tester::apply
        (from_wkt<ML>("MULTILINESTRING((1 5, -4.3 -.1), (0 6, 8.6 6, 189.7654 5, 1 3, 6 3, 3 5, 6 2.232432, 0 4), (-6 5, 1 2.232432), (3 -1032.34324, 9 0, 189.7654 1, -1.4 3, 3 189.7654, +.3 10.0002, 1 5, 6 3, 5 1, 9 1, 10.0002 -1032.34324, -0.7654 0, 5 3, 3 4), (2.232432 2.232432, 8.6 +.4, 0.0 2.232432, 4 0, -8.8 10.0002), (1 0, 6 6, 7 2, -0 8.4), (-0.7654 3, +.6 8, 4 -1032.34324, 1 6, 0 4), (0 7, 2 1, 8 -7, 7 -.7, -1032.34324 9), (5 0, 10.0002 4, 8 7, 3 3, -8.1 5))"),
         from_wkt<ML>("MULTILINESTRING((5 10.0002, 2 7, -0.7654 0, 5 3), (0 -0.7654, 4 10.0002, 4 +.1, -.8 3, -.1 8, 10.0002 2, +.9 -1032.34324))"),
         from_wkt<ML>("MULTILINESTRING((1 5,-4.3 -0.1),(0 6,8.6 6,189.7654 5,1 3,6 3,3 5,6 2.232432,0 4),(-6 5,1 2.232432),(5 3,3 4),(3 -1032.34324,9 0,189.7654 1,-1.4 3,3 189.7654,0.3 10.0002,1 5,6 3,5 1,9 1,10.0002 -1032.34324,-0.7654 0),(2.232432 2.232432,8.6 0.4,0 2.232432,4 0,-8.8 10.0002),(1 0,6 6,7 2,-0 8.4),(-0.7654 3,0.6 8,4 -1032.34324,1 6,0 4),(0 7,2 1,8 -7,7 -0.7,-1032.34324 9),(5 0,10.0002 4,8 7,3 3,-8.1 5))"),
         "mlmldf24",
         1e-10
         );

    tester::apply
        (from_wkt<ML>("MULTILINESTRING((5 10.0002, 2 7, -0.7654 0, 5 3), (0 -0.7654, 4 10.0002, 4 +.1, -.8 3, -.1 8, 10.0002 2, +.9 -1032.34324))"),
         from_wkt<ML>("MULTILINESTRING((1 5, -4.3 -.1), (0 6, 8.6 6, 189.7654 5, 1 3, 6 3, 3 5, 6 2.232432, 0 4), (-6 5, 1 2.232432), (3 -1032.34324, 9 0, 189.7654 1, -1.4 3, 3 189.7654, +.3 10.0002, 1 5, 6 3, 5 1, 9 1, 10.0002 -1032.34324, -0.7654 0, 5 3, 3 4), (2.232432 2.232432, 8.6 +.4, 0.0 2.232432, 4 0, -8.8 10.0002), (1 0, 6 6, 7 2, -0 8.4), (-0.7654 3, +.6 8, 4 -1032.34324, 1 6, 0 4), (0 7, 2 1, 8 -7, 7 -.7, -1032.34324 9), (5 0, 10.0002 4, 8 7, 3 3, -8.1 5))"),
         from_wkt<ML>("MULTILINESTRING((5 10.0002,2 7,-0.7654 8.88178e-16),(0 -0.7654,4 10.0002,4 0.1,-0.8 3,-0.1 8,10.0002 2,0.9 -1032.34324))"),
         "mlmldf24-r",
         1e-10
         );

    tester::apply
        (from_wkt<ML>("MULTILINESTRING((-.4 2, 2.232432 3, 6 9, 8 189.7654, -1032.34324 5.4, 2.232432 9), (-1032.34324 3, 8 -1.6), (0 -.2, 8 1, -.5 7, 6 +.2))"),
         from_wkt<ML>("MULTILINESTRING((-8 1, 4.8 6, 2 +.5), (10.0002 2,9 -1032.34324, .3 8, 0 5, 8 1, 4 .4, 2 8), (6 7, +.1 7, 0 -.5))"),
         from_wkt<ML>("MULTILINESTRING((-0.4 2,2.232432 3,6 9,8 189.7654,-1032.34324 5.4,2.232432 9),(-1032.34324 3,8 -1.6),(0 -0.2,4 0.4),(8 1,-0.5 7,6 0.2))"),
         "mlmldf25",
         1e-10
         );

    tester::apply
        (from_wkt<ML>("MULTILINESTRING((-8 1, 4.8 6, 2 +.5), (10.0002 2,9 -1032.34324, .3 8, 0 5, 8 1, 4 .4, 2 8), (6 7, +.1 7, 0 -.5))"),
         from_wkt<ML>("MULTILINESTRING((-.4 2, 2.232432 3, 6 9, 8 189.7654, -1032.34324 5.4, 2.232432 9), (-1032.34324 3, 8 -1.6), (0 -.2, 8 1, -.5 7, 6 +.2))"),
         from_wkt<ML>("MULTILINESTRING((-8 1,4.8 6,2 0.5),(10.0002 2,9 -1032.34324,0.3 8,0 5,8 1),(4 0.4,2 8),(6 7,0.1 7,0 -0.5))"),
         "mlmldf25-r",
         1e-10
         );
}






#ifndef BOOST_GEOMETRY_TEST_NO_DEGENERATE
BOOST_AUTO_TEST_CASE( test_difference_ml_ml_degenerate )
{
#ifdef BOOST_GEOMETRY_TEST_DEBUG
    std::cout << std::endl << std::endl << std::endl;
    std::cout << "*** MULTILINESTRING / MULTILINESTRING DIFFERENCE" 
              << " (DEGENERATE) ***"
              << std::endl;
    std::cout << std::endl;
#endif

    typedef multi_linestring_type ML;

    typedef test_difference_of_geometries<ML, ML, ML> tester;

    // the following test cases concern linestrings with duplicate
    // points and possibly linestrings with zero length.

    tester::apply
        (from_wkt<ML>("MULTILINESTRING((5 5,5 5),(0 0,18 0,18 0,\
                      19 0,19 0,19 0,30 0),(2 0,2 0),(4 10,4 10))"),
         from_wkt<ML>("MULTILINESTRING((-10 0,-9 0),(0 10,5 0,20 0,20 0,30 10),\
                      (1 1,2 2),(1 10,1 10,1 0,1 0,1 -10),\
                      (2 0,2 0),(3 0,3 0,3 0),(0 0,0 10),\
                      (4 0,4 10),(5 5,5 5))"),
         from_wkt<ML>("MULTILINESTRING((0 0,5 0),(20 0,30 0))"),
         "mlmldf20a"
         );

    tester::apply
        (from_wkt<ML>("MULTILINESTRING((5 5,5 5),(0 0,0 0,18 0,18 0,\
                      19 0,19 0,19 0,30 0,30 0),(2 0,2 0),(4 10,4 10))"),
         from_wkt<ML>("MULTILINESTRING((-10 0,-9 0),(0 10,5 0,20 0,20 0,30 10),\
                      (1 1,1 1,2 2,2 2),(1 10,1 10,1 0,1 0,1 -10),\
                      (2 0,2 0),(3 0,3 0,3 0),(0 0,0 0,0 10,0 10),\
                      (4 0,4 10,4 10),(5 5,5 5))"),
         from_wkt<ML>("MULTILINESTRING((0 0,5 0),(20 0,30 0))"),
         "mlmldf20aa"
         );

    tester::apply
        (from_wkt<ML>("MULTILINESTRING((10 10,10 10),(0 0,0 0,18 0,18 0,\
                      19 0,19 0,19 0,30 0,30 0),(2 0,2 0),(4 10,4 10))"),
         from_wkt<ML>("MULTILINESTRING((-10 0,-9 0),(0 10,5 0,20 0,20 0,30 10),\
                      (1 1,1 1,2 2,2 2),(1 10,1 10,1 0,1 0,1 -10),\
                      (2 0,2 0),(3 0,3 0,3 0),(0 0,0 0,0 10,0 10),\
                      (4 0,4 10,4 10),(5 5,5 5))"),
         from_wkt<ML>("MULTILINESTRING((10 10,10 10),(0 0,5 0),(20 0,30 0))"),
         "mlmldf20aaa"
         );

    tester::apply
        (from_wkt<ML>("MULTILINESTRING((10 10),(0 0,0 0,18 0,18 0,\
                      19 0,19 0,19 0,30 0,30 0),(2 0,2 0),(4 10,4 10))"),
         from_wkt<ML>("MULTILINESTRING((-10 0,-9 0),(0 10,5 0,20 0,20 0,30 10),\
                      (1 1,1 1,2 2,2 2),(1 10,1 10,1 0,1 0,1 -10),\
                      (2 0,2 0),(3 0,3 0,3 0),(0 0,0 0,0 10,0 10),\
                      (4 0,4 10,4 10),(5 5,5 5))"),
         from_wkt<ML>("MULTILINESTRING((10 10,10 10),(0 0,5 0),(20 0,30 0))"),
         "mlmldf20aaaa"
         );

    tester::apply
        (from_wkt<ML>("MULTILINESTRING((0 0,0 0),(1 1,1 1))"),
         from_wkt<ML>("MULTILINESTRING((1 1,1 1))"),
         from_wkt<ML>("MULTILINESTRING((0 0,0 0))"),
         "mlmldf21"
         );

    tester::apply
        (from_wkt<ML>("MULTILINESTRING((0 0,0 0),(2 2,2 2),(1 1,1 1))"),
         from_wkt<ML>("MULTILINESTRING((1 1,1 1))"),
         from_wkt<ML>("MULTILINESTRING((0 0,0 0),(2 2,2 2))"),
         "mlmldf22"
         );

    tester::apply
        (from_wkt<ML>("MULTILINESTRING((0 0,0 0),(1 1,1 1),(2 2,2 2))"),
         from_wkt<ML>("MULTILINESTRING((1 1,1 1))"),
         from_wkt<ML>("MULTILINESTRING((0 0,0 0),(2 2,2 2))"),
         "mlmldf23"
         );
}
#endif // BOOST_GEOMETRY_TEST_NO_DEGENERATE




BOOST_AUTO_TEST_CASE( test_difference_ml_ml_spikes )
{
#ifdef BOOST_GEOMETRY_TEST_DEBUG
    std::cout << std::endl << std::endl << std::endl;
    std::cout << "*** MULTILINESTRING / MULTILINESTRING DIFFERENCE" 
              << " (WITH SPIKES) ***"
              << std::endl;
    std::cout << std::endl;
#endif

    typedef multi_linestring_type ML;

    typedef test_difference_of_geometries<ML, ML, ML> tester;

    // the following test cases concern linestrings with spikes

    tester::apply
        (from_wkt<ML>("MULTILINESTRING((0 0,10 0))"),
         from_wkt<ML>("MULTILINESTRING((1 0,9 0,5 0))"),
         from_wkt<ML>("MULTILINESTRING((0 0,1 0),(9 0,10 0))"),
         "mlmldf-spikes-01"
         );

    tester::apply
        (from_wkt<ML>("MULTILINESTRING((0 0,10 0))"),
         from_wkt<ML>("MULTILINESTRING((9 0,1 0,5 0))"),
         from_wkt<ML>("MULTILINESTRING((0 0,1 0),(9 0,10 0))"),
         "mlmldf-spikes-02"
         );

    tester::apply
        (from_wkt<ML>("MULTILINESTRING((0 0,10 0))"),
         from_wkt<ML>("MULTILINESTRING((1 0,9 0,2 0,8 0,3 0,7 0,4 0,5 0))"),
         from_wkt<ML>("MULTILINESTRING((0 0,1 0),(9 0,10 0))"),
         "mlmldf-spikes-03"
         );

    tester::apply
        (from_wkt<ML>("MULTILINESTRING((0 0,10 0))"),
         from_wkt<ML>("MULTILINESTRING((1 0,3 0,2 0,4 0,3 0,5 0,4 0,6 0,\
                      5 0,7 0,6 0,8 0,7 0,9 0))"),
         from_wkt<ML>("MULTILINESTRING((0 0,1 0),(9 0,10 0))"),
         "mlmldf-spikes-04"
         );

    tester::apply
        (from_wkt<ML>("MULTILINESTRING((0 0,10 0))"),
         from_wkt<ML>("MULTILINESTRING((1 0,6 0,5 0),(7 0,8 0,7 0),\
                      (9 1,9 0,9 2))"),
         from_wkt<ML>("MULTILINESTRING((0 0,1 0),(6 0,7 0),(8 0,10 0))"),
         "mlmldf-spikes-05"
         );

    tester::apply
        (from_wkt<ML>("MULTILINESTRING((0 0,10 0))"),
         from_wkt<ML>("MULTILINESTRING((1 0,6 0,5 0),(7 0,8 0,7 0),\
                      (9 0,9 2,9 1))"),
         from_wkt<ML>("MULTILINESTRING((0 0,1 0),(6 0,7 0),(8 0,10 0))"),
         "mlmldf-spikes-05a"
         );

    tester::apply
        (from_wkt<ML>("MULTILINESTRING((0 0,10 0))"),
         from_wkt<ML>("MULTILINESTRING((1 0,6 0,5 0),(9 0,6 0,8 0),\
                      (11 0,8 0,12 0))"),
         from_wkt<ML>("MULTILINESTRING((0 0,1 0))"),
         "mlmldf-spikes-06"
         );

    tester::apply
        (from_wkt<ML>("MULTILINESTRING((0 0,10 0))"),
         from_wkt<ML>("MULTILINESTRING((-1 0,0 0,-2 0),(11 0,10 0,12 0))"),
         from_wkt<ML>("MULTILINESTRING((0 0,10 0))"),
         "mlmldf-spikes-07"
         );

    tester::apply
        (from_wkt<ML>("MULTILINESTRING((0 0,10 0))"),
         from_wkt<ML>("MULTILINESTRING((-1 -1,0 0,-2 -2),(11 1,10 0,12 2))"),
         from_wkt<ML>("MULTILINESTRING((0 0,10 0))"),
         "mlmldf-spikes-07a"
         );

    tester::apply
        (from_wkt<ML>("MULTILINESTRING((0 0,10 0))"),
         from_wkt<ML>("MULTILINESTRING((1 0,6 0,5 0),(11 0,10 0,12 0),\
                      (7 5,7 0,8 0,6.5 0,8.5 0,8.5 5))"),
         from_wkt<ML>("MULTILINESTRING((0 0,1 0),(6 0,6.5 0),(8.5 0,10 0))"),
         "mlmldf-spikes-08"
         );

    // now the first geometry has a spike
    tester::apply
        (from_wkt<ML>("MULTILINESTRING((0 0,7 0,4 0,10 0))"),
         from_wkt<ML>("MULTILINESTRING((1 0,8 0))"),
         from_wkt<ML>("MULTILINESTRING((0 0,1 0),(8 0,10 0))"),
         "mlmldf-spikes-09"
         );

    tester::apply
        (from_wkt<ML>("MULTILINESTRING((0 0,7 0,4 0,10 0))"),
         from_wkt<ML>("MULTILINESTRING((0 0,1 0),(9 0,10 0))"),
         from_wkt<ML>("MULTILINESTRING((1 0,7 0,4 0,9 0))"),
         "mlmldf-spikes-09a"
         );

    tester::apply
        (from_wkt<ML>("MULTILINESTRING((0 0,7 0,4 0,10 0))"),
         from_wkt<ML>("MULTILINESTRING((0 0,5 0),(9 0,10 0))"),
         from_wkt<ML>("MULTILINESTRING((5 0,7 0,5 0),(5 0,9 0))"),
         "mlmldf-spikes-09b"
         );

    tester::apply
        (from_wkt<ML>("MULTILINESTRING((0 0,7 0,4 0,10 0))"),
         from_wkt<ML>("MULTILINESTRING((0 0,5 0),(6 0,10 0))"),
         from_wkt<ML>("MULTILINESTRING((5 0,6 0),(6 0,5 0),(5 0,6 0))"),
         "mlmldf-spikes-09c"
         );

    tester::apply
        (from_wkt<ML>("MULTILINESTRING((0 0,10 0,5 0))"),
         from_wkt<ML>("MULTILINESTRING((1 0,8 0))"),
         from_wkt<ML>("MULTILINESTRING((0 0,1 0),(8 0,10 0,8 0))"),
         "mlmldf-spikes-10"
         );

    tester::apply
        (from_wkt<ML>("MULTILINESTRING((0 0,10 0,5 0))"),
         from_wkt<ML>("MULTILINESTRING((1 0,8 0,4 0),(2 0,9 0,5 0))"),
         from_wkt<ML>("MULTILINESTRING((0 0,1 0),(9 0,10 0,9 0))"),
         "mlmldf-spikes-11"
         );

    tester::apply
        (from_wkt<ML>("MULTILINESTRING((0 0,10 0,5 0))"),
         from_wkt<ML>("MULTILINESTRING((11 1,10 0,12 2))"),
         from_wkt<ML>("MULTILINESTRING((0 0,10 0,5 0))"),
         "mlmldf-spikes-12"
         );

    tester::apply
        (from_wkt<ML>("MULTILINESTRING((0 0,10 0,5 0))"),
         from_wkt<ML>("MULTILINESTRING((11 -1,10 0,12 -2))"),
         from_wkt<ML>("MULTILINESTRING((0 0,10 0,5 0))"),
         "mlmldf-spikes-12a"
         );

    tester::apply
        (from_wkt<ML>("MULTILINESTRING((0 0,10 0,5 0))"),
         from_wkt<ML>("MULTILINESTRING((11 0,10 0,12 0))"),
         from_wkt<ML>("MULTILINESTRING((0 0,10 0,5 0))"),
         "mlmldf-spikes-13"
         );

    // the following three tests have been discussed with Adam
    tester::apply
        (from_wkt<ML>("MULTILINESTRING((1 0,1 1,2 1))"),
         from_wkt<ML>("MULTILINESTRING((1 2,1 1,1 2))"),
         from_wkt<ML>("MULTILINESTRING((1 0,1 1,2 1))"),
         "mlmldf-spikes-14"
         );

    tester::apply
        (from_wkt<ML>("MULTILINESTRING((0 0,1 0,0 0))"),
         from_wkt<ML>("MULTILINESTRING((2 0,1 0,2 0))"),
         from_wkt<ML>("MULTILINESTRING((0 0,1 0,0 0))"),
         "mlmldf-spikes-15"
         );

    tester::apply
        (from_wkt<ML>("MULTILINESTRING((1 0,1 1,2 1))"),
         from_wkt<ML>("MULTILINESTRING((2 0,1 1,2 0))"),
         from_wkt<ML>("MULTILINESTRING((1 0,1 1,2 1))"),
         "mlmldf-spikes-16"
         );

    tester::apply
        (from_wkt<ML>("MULTILINESTRING((1 0,1 1,2 1))"),
         from_wkt<ML>("MULTILINESTRING((2 1,1 1,2 1))"),
         from_wkt<ML>("MULTILINESTRING((1 0,1 1))"),
         "mlmldf-spikes-17"
         );

    // test cases sent by Adam on the mailing list (equal slikes)
    tester::apply
        (from_wkt<ML>("MULTILINESTRING((0 0,1 1,0 0))"),
         from_wkt<ML>("MULTILINESTRING((0 0,1 1,0 0))"),
         from_wkt<ML>("MULTILINESTRING()"),
         "mlmldf-spikes-18"
         );

    tester::apply
        (from_wkt<ML>("MULTILINESTRING((0 0,1 1,0 0))"),
         from_wkt<ML>("MULTILINESTRING((1 1,0 0,1 1))"),
         from_wkt<ML>("MULTILINESTRING()"),
         "mlmldf-spikes-19"
         );
}
