// Boost.Geometry (aka GGL, Generic Geometry Library)
// Unit Test

// Copyright (c) 2014, Oracle and/or its affiliates.

// Contributed and/or modified by Menelaos Karavelas, on behalf of Oracle

// Licensed under the Boost Software License version 1.0.
// http://www.boost.org/users/license.html

#ifndef BOOST_TEST_MODULE
#define BOOST_TEST_MODULE test_is_valid
#endif

#include <iostream>

#include <boost/test/included/unit_test.hpp>

#include "from_wkt.hpp"
#include "test_is_valid.hpp"


BOOST_AUTO_TEST_CASE( test_is_valid_point )
{
#ifdef BOOST_GEOMETRY_TEST_DEBUG
    std::cout << std::endl << std::endl;
    std::cout << "************************************" << std::endl;
    std::cout << " is_valid: POINT " << std::endl;
    std::cout << "************************************" << std::endl;
#endif

    typedef point_type G;
    typedef default_validity_tester tester;
    typedef test_valid<tester, G> test;

    test::apply(from_wkt<G>("POINT(0 0)"), true);
}

BOOST_AUTO_TEST_CASE( test_is_valid_multipoint )
{
#ifdef BOOST_GEOMETRY_TEST_DEBUG
    std::cout << std::endl << std::endl;
    std::cout << "************************************" << std::endl;
    std::cout << " is_valid: MULTIPOINT " << std::endl;
    std::cout << "************************************" << std::endl;
#endif

    typedef multi_point_type G;
    typedef default_validity_tester tester;
    typedef test_valid<tester, G> test;

    test::apply(from_wkt<G>("MULTIPOINT()"), false);
    test::apply(from_wkt<G>("MULTIPOINT(0 0,0 0)"), true);
    test::apply(from_wkt<G>("MULTIPOINT(0 0,1 0,1 1,0 1)"), true);
    test::apply(from_wkt<G>("MULTIPOINT(0 0,1 0,1 1,1 0,0 1)"), true);
}

BOOST_AUTO_TEST_CASE( test_is_valid_segment )
{
#ifdef BOOST_GEOMETRY_TEST_DEBUG
    std::cout << std::endl << std::endl;
    std::cout << "************************************" << std::endl;
    std::cout << " is_valid: SEGMENT " << std::endl;
    std::cout << "************************************" << std::endl;
#endif

    typedef segment_type G;
    typedef default_validity_tester tester;
    typedef test_valid<tester, G> test;

    test::apply(from_wkt<G>("SEGMENT(0 0,0 0)"), false);
    test::apply(from_wkt<G>("SEGMENT(0 0,1 0)"), true);
}

BOOST_AUTO_TEST_CASE( test_is_valid_box )
{
#ifdef BOOST_GEOMETRY_TEST_DEBUG
    std::cout << std::endl << std::endl;
    std::cout << "************************************" << std::endl;
    std::cout << " is_valid: BOX " << std::endl;
    std::cout << "************************************" << std::endl;
#endif

    typedef box_type G;
    typedef default_validity_tester tester;
    typedef test_valid<tester, G> test;

    // boxes where the max corner and below and/or to the left of min corner
    test::apply(from_wkt<G>("BOX(0 0,-1 0)"), false);
    test::apply(from_wkt<G>("BOX(0 0,0 -1)"), false);
    test::apply(from_wkt<G>("BOX(0 0,-1 -1)"), false);

    // boxes of zero area; they are not 2-dimensional, so invalid
    test::apply(from_wkt<G>("BOX(0, 0, 0, 0)"), false);
    test::apply(from_wkt<G>("BOX(0 0,1 0)"), false);
    test::apply(from_wkt<G>("BOX(0 0,0 1)"), false);

    test::apply(from_wkt<G>("BOX(0 0,1 1)"), true);
}

template <typename G, bool AllowSpikes>
void test_linestrings()
{
#ifdef BOOST_GEOMETRY_TEST_DEBUG
    std::cout << "SPIKES ALLOWED? "
              << std::boolalpha
              << AllowSpikes
              << std::noboolalpha
              << std::endl;
#endif

    typedef validity_tester_linear<AllowSpikes> tester;
    typedef test_valid<tester, G> test;

    // empty linestring
    test::apply(from_wkt<G>("LINESTRING()"), false);

    // 1-point linestrings
    test::apply(from_wkt<G>("LINESTRING(0 0)"), false);
    test::apply(from_wkt<G>("LINESTRING(0 0,0 0)"), false);
    test::apply(from_wkt<G>("LINESTRING(0 0,0 0,0 0)"), false);

    // 2-point linestrings
    test::apply(from_wkt<G>("LINESTRING(0 0,1 2)"), true);
    test::apply(from_wkt<G>("LINESTRING(0 0,1 2,1 2)"), true);
    test::apply(from_wkt<G>("LINESTRING(0 0,0 0,1 2,1 2)"), true);
    test::apply(from_wkt<G>("LINESTRING(0 0,0 0,0 0,1 2,1 2)"), true);

    // 3-point linestrings
    test::apply(from_wkt<G>("LINESTRING(0 0,1 0,2 10)"), true);
    test::apply(from_wkt<G>("LINESTRING(0 0,1 0,2 10,0 0)"), true);
    test::apply(from_wkt<G>("LINESTRING(0 0,10 0,10 10,5 0)"), true);

    // linestrings with spikes
    test::apply(from_wkt<G>("LINESTRING(0 0,1 2,0 0)"), AllowSpikes);
    test::apply(from_wkt<G>("LINESTRING(0 0,1 2,1 2,0 0)"), AllowSpikes);
    test::apply(from_wkt<G>("LINESTRING(0 0,0 0,1 2,1 2,0 0)"),
                AllowSpikes);
    test::apply(from_wkt<G>("LINESTRING(0 0,0 0,0 0,1 2,1 2,0 0,0 0)"),
                AllowSpikes);
    test::apply(from_wkt<G>("LINESTRING(0 0,10 0,5 0)"), AllowSpikes);    
    test::apply(from_wkt<G>("LINESTRING(0 0,10 0,10 10,5 0,0 0)"),
                AllowSpikes);
    test::apply(from_wkt<G>("LINESTRING(0 0,10 0,10 10,5 0,4 0,6 0)"),
                AllowSpikes);
    test::apply(from_wkt<G>("LINESTRING(0 0,1 0,1 1,5 5,4 4)"),
                AllowSpikes);
    test::apply(from_wkt<G>("LINESTRING(0 0,1 0,1 1,5 5,4 4,6 6)"),
                AllowSpikes);
    test::apply(from_wkt<G>("LINESTRING(0 0,1 0,1 1,5 5,4 4,4 0)"),
                AllowSpikes);
    test::apply(from_wkt<G>("LINESTRING(0 0,0 0,1 0,1 0,1 0,0 0,0 0,2 0)"),
                AllowSpikes);
    test::apply(from_wkt<G>("LINESTRING(0 0,1 0,0 0,2 0,0 0,3 0,0 0,4 0)"),
                AllowSpikes);
    test::apply(from_wkt<G>("LINESTRING(0 0,1 0,0 0,2 0,0 0,3 0,0 0,4 0,0 0)"),
                AllowSpikes);

    // other examples
    test::apply(from_wkt<G>("LINESTRING(0 0,10 0,10 10,5 0,4 0)"), true);
    test::apply(from_wkt<G>("LINESTRING(0 0,10 0,10 10,5 0,4 0,3 0)"),
                true);
    test::apply(from_wkt<G>("LINESTRING(0 0,10 0,10 10,5 0,4 0,-1 0)"),
                true);
    test::apply(from_wkt<G>("LINESTRING(0 0,1 0,1 1,-1 1,-1 0,0 0)"),
                true);

    test::apply(from_wkt<G>("LINESTRING(0 0,1 0,1 1,-1 1,-1 0,0.5 0)"),
                true);
}

BOOST_AUTO_TEST_CASE( test_is_valid_linestring )
{
#ifdef BOOST_GEOMETRY_TEST_DEBUG
    std::cout << std::endl << std::endl;
    std::cout << "************************************" << std::endl;
    std::cout << " is_valid: LINESTRING " << std::endl;
    std::cout << "************************************" << std::endl;
#endif

    bool const allow_spikes = true;
    bool const do_not_allow_spikes = !allow_spikes;

    test_linestrings<linestring_type, allow_spikes>();
    test_linestrings<linestring_type, do_not_allow_spikes>();
}

template <typename G, bool AllowSpikes>
void test_multilinestrings()
{
#ifdef BOOST_GEOMETRY_TEST_DEBUG
    std::cout << "SPIKES ALLOWED? "
              << std::boolalpha
              << AllowSpikes
              << std::noboolalpha
              << std::endl;
#endif

    typedef validity_tester_linear<AllowSpikes> tester;
    typedef test_valid<tester, G> test;

    // empty multilinestring
    test::apply(from_wkt<G>("MULTILINESTRING()"), false);

    // multilinestring with empty linestring(s)
    test::apply(from_wkt<G>("MULTILINESTRING(())"), false);
    test::apply(from_wkt<G>("MULTILINESTRING((),(),())"), false);
    test::apply(from_wkt<G>("MULTILINESTRING((),(0 1,1 0))"), false);

    // multilinestring with invalid linestrings
    test::apply(from_wkt<G>("MULTILINESTRING((0 0),(0 1,1 0))"), false);
    test::apply(from_wkt<G>("MULTILINESTRING((0 0,0 0),(0 1,1 0))"), false);
    test::apply(from_wkt<G>("MULTILINESTRING((0 0),(1 0))"), false);
    test::apply(from_wkt<G>("MULTILINESTRING((0 0,0 0),(1 0,1 0))"), false);
    test::apply(from_wkt<G>("MULTILINESTRING((0 0),(0 0))"), false);
    test::apply(from_wkt<G>("MULTILINESTRING((0 0,1 0,0 0),(5 0))"), false);

    // multilinstring that has linestrings with spikes
    test::apply(from_wkt<G>("MULTILINESTRING((0 0,1 0,0 0),(5 0,1 0,4 1))"),
                AllowSpikes);
    test::apply(from_wkt<G>("MULTILINESTRING((0 0,1 0,0 0),(1 0,2 0))"),
                AllowSpikes);

    // valid multilinestrings
    test::apply(from_wkt<G>("MULTILINESTRING((0 0,1 0,2 0),(5 0,1 0,4 1))"),
                true);
    test::apply(from_wkt<G>("MULTILINESTRING((0 0,1 0,2 0),(1 0,2 0))"),
                true);
    test::apply(from_wkt<G>("MULTILINESTRING((0 0,1 1),(0 1,1 0))"), true);
    test::apply(from_wkt<G>("MULTILINESTRING((0 0,1 1,2 2),(0 1,1 0,2 2))"),   
                true);
}

BOOST_AUTO_TEST_CASE( test_is_valid_multilinestring )
{
#ifdef BOOST_GEOMETRY_TEST_DEBUG
    std::cout << std::endl << std::endl;
    std::cout << "************************************" << std::endl;
    std::cout << " is_valid: MULTILINESTRING " << std::endl;
    std::cout << "************************************" << std::endl;
#endif

    bool const allow_spikes = true;
    bool const do_not_allow_spikes = !allow_spikes;

    test_multilinestrings<multi_linestring_type, allow_spikes>();
    test_multilinestrings<multi_linestring_type, do_not_allow_spikes>();
}


template <typename Point, bool AllowDuplicates>
void test_open_rings()
{
#ifdef BOOST_GEOMETRY_TEST_DEBUG
    std::cout << std::endl << std::endl;
    std::cout << "************************************" << std::endl;
    std::cout << " is_valid: RING (open) " << std::endl;
    std::cout << "************************************" << std::endl;
    std::cout << "DUPLICATES ALLOWED? "
              << std::boolalpha
              << AllowDuplicates
              << std::noboolalpha
              << std::endl;
#endif

    typedef bg::model::ring<Point, false, false> OG; // ccw, open ring
    typedef bg::model::ring<Point, false, true> CG; // ccw, closed ring
    typedef bg::model::ring<Point, true, false> CW_OG; // cw, open ring
    typedef bg::model::ring<Point, true, true> CW_CG; // cw, closed ring
 
    typedef validity_tester_areal<AllowDuplicates> tester;
    typedef test_valid<tester, OG, CG, CW_OG, CW_CG> test;

    // not enough points
    test::apply(from_wkt<OG>("POLYGON(())"), false);
    test::apply(from_wkt<OG>("POLYGON((0 0))"), false);
    test::apply(from_wkt<OG>("POLYGON((0 0,1 0))"), false);

    // duplicate points
    test::apply(from_wkt<OG>("POLYGON((0 0,0 0,0 0))"), false);
    test::apply(from_wkt<OG>("POLYGON((0 0,1 0,1 0))"), false);
    test::apply(from_wkt<OG>("POLYGON((0 0,1 0,0 0))"), false);
    test::apply(from_wkt<OG>("POLYGON((0 0,1 0,1 1,0 0))"),
                AllowDuplicates);
    test::apply(from_wkt<OG>("POLYGON((0 0,1 0,1 0,1 1))"),
                AllowDuplicates);
    test::apply(from_wkt<OG>("POLYGON((0 0,1 0,1 0,1 1,0 0))"),
                AllowDuplicates);

    // with spikes
    test::apply(from_wkt<OG>("POLYGON((0 0,2 0,2 2,0 2,1 2))"), false);
    test::apply(from_wkt<OG>("POLYGON((0 0,2 0,1 0,2 2))"), false);
    test::apply(from_wkt<OG>("POLYGON((0 0,1 0,2 0,1 0,4 0,4 4))"), false);
    test::apply(from_wkt<OG>("POLYGON((0 0,2 0,2 2,1 0))"), false);
    test::apply(from_wkt<OG>("POLYGON((0 0,2 0,1 0))"), false);
    test::apply(from_wkt<OG>("POLYGON((0 0,5 0,5 5,4 4,5 5,0 5))"), false);
    test::apply(from_wkt<OG>("POLYGON((0 0,5 0,5 5,4 4,3 3,5 5,0 5))"), false);

    // with spikes and duplicate points
    test::apply(from_wkt<OG>("POLYGON((0 0,0 0,2 0,2 0,1 0,1 0))"), false);

    // with self-crossings
    test::apply(from_wkt<OG>("POLYGON((0 0,5 0,5 5,3 -1,0 5))"), false);

    // with self-crossings and duplicate points
    test::apply(from_wkt<OG>("POLYGON((0 0,5 0,5 5,5 5,3 -1,0 5,0 5))"),
                false);

    // with self-intersections
    test::apply(from_wkt<OG>("POLYGON((0 0,5 0,5 5,3 5,3 0,2 0,2 5,0 5))"),
                false);

    test::apply(from_wkt<OG>("POLYGON((0 0,5 0,5 5,3 5,3 0,2 5,0 5))"),
                false);

    test::apply(from_wkt<OG>("POLYGON((0 0,5 0,5 1,1 1,1 2,2 2,3 1,4 2,5 2,5 5,0 5))"),
                false);

    // with self-intersections and duplicate points
    test::apply(from_wkt<OG>("POLYGON((0 0,5 0,5 5,3 5,3 5,3 0,3 0,2 0,2 0,2 5,2 5,0 5))"),
                false);

    // next two suggested by Adam Wulkiewicz
    test::apply(from_wkt<OG>("POLYGON((0 0,5 0,5 5,0 5,4 4,2 2,0 5))"), false);
    test::apply(from_wkt<OG>("POLYGON((0 0,5 0,5 5,1 4,4 4,4 1,0 5))"),
                false);

    // and a few more
    test::apply(from_wkt<OG>("POLYGON((0 0,5 0,5 5,4 4,1 4,1 1,4 1,4 4,0 5))"),
                false);
    test::apply(from_wkt<OG>("POLYGON((0 0,5 0,5 5,4 4,4 1,1 1,1 4,4 4,0 5))"),
                false);

    // valid rings
    test::apply(from_wkt<OG>("POLYGON((0 0,1 0,1 1))"), true);
    test::apply(from_wkt<OG>("POLYGON((1 0,1 1,0 0))"), true);
    test::apply(from_wkt<OG>("POLYGON((0 0,1 0,1 1,0 1))"), true);
    test::apply(from_wkt<OG>("POLYGON((1 0,1 1,0 1,0 0))"), true);
}


template <typename Point, bool AllowDuplicates>
void test_closed_rings()
{
#ifdef BOOST_GEOMETRY_TEST_DEBUG
    std::cout << std::endl << std::endl;
    std::cout << "************************************" << std::endl;
    std::cout << " is_valid: RING (closed) " << std::endl;
    std::cout << "************************************" << std::endl;
    std::cout << "DUPLICATES ALLOWED? "
              << std::boolalpha
              << AllowDuplicates
              << std::noboolalpha
              << std::endl;
#endif

    typedef bg::model::ring<Point, false, true> CG; // ccw, closed ring
    typedef bg::model::ring<Point, true, true> CW_CG; // cw, closed ring

    typedef validity_tester_areal<AllowDuplicates> tester;
    typedef test_valid<tester, CG, CG, CW_CG> test;

    // not enough points
    test::apply(from_wkt<CG>("POLYGON(())"), false);
    test::apply(from_wkt<CG>("POLYGON((0 0))"), false);
    test::apply(from_wkt<CG>("POLYGON((0 0,0 0))"), false);
    test::apply(from_wkt<CG>("POLYGON((0 0,1 0))"), false);
    test::apply(from_wkt<CG>("POLYGON((0 0,1 0,1 0))"), false);
    test::apply(from_wkt<CG>("POLYGON((0 0,1 0,2 0))"), false);
    test::apply(from_wkt<CG>("POLYGON((0 0,1 0,1 0,2 0))"), false);
    test::apply(from_wkt<CG>("POLYGON((0 0,1 0,2 0,2 0))"), false);

    // boundary not closed
    test::apply(from_wkt<CG>("POLYGON((0 0,1 0,1 1,1 2))"), false);
    test::apply(from_wkt<CG>("POLYGON((0 0,1 0,1 0,1 1,1 1,1 2))"),
                false);
}

BOOST_AUTO_TEST_CASE( test_is_valid_ring )
{
    bool const allow_duplicates = true;
    bool const do_not_allow_duplicates = !allow_duplicates;

    test_open_rings<point_type, allow_duplicates>();
    test_open_rings<point_type, do_not_allow_duplicates>();

    test_closed_rings<point_type, allow_duplicates>();
    test_closed_rings<point_type, do_not_allow_duplicates>();
}

template <typename Point, bool AllowDuplicates>
void test_open_polygons()
{
#ifdef BOOST_GEOMETRY_TEST_DEBUG
    std::cout << std::endl << std::endl;
    std::cout << "************************************" << std::endl;
    std::cout << " is_valid: POLYGON (open) " << std::endl;
    std::cout << "************************************" << std::endl;
    std::cout << "DUPLICATES ALLOWED? "
              << std::boolalpha
              << AllowDuplicates
              << std::noboolalpha
              << std::endl;
#endif

    typedef bg::model::polygon<Point, false, false> OG; // ccw, open
    typedef bg::model::polygon<Point, false, true> CG; // ccw, closed
    typedef bg::model::polygon<Point, true, false> CW_OG; // cw, open
    typedef bg::model::polygon<Point, true, true> CW_CG; // cw, closed

    typedef validity_tester_areal<AllowDuplicates> tester;
    typedef test_valid<tester, OG, CG, CW_OG, CW_CG> test;

    // not enough points in exterior ring
    test::apply(from_wkt<OG>("POLYGON(())"), false);
    test::apply(from_wkt<OG>("POLYGON((0 0))"), false);
    test::apply(from_wkt<OG>("POLYGON((0 0,1 0))"), false);

    // not enough points in interior ring
    test::apply(from_wkt<OG>("POLYGON((0 0,10 0,10 10,0 10),())"), false);
    test::apply(from_wkt<OG>("POLYGON((0 0,10 0,10 10,0 10),(1 1))"), false);
    test::apply(from_wkt<OG>("POLYGON((0 0,10 0,10 10,0 10),(1 1,2 2))"),
                false);

    // duplicate points in exterior ring
    test::apply(from_wkt<OG>("POLYGON((0 0,0 0,0 0))"), false);
    test::apply(from_wkt<OG>("POLYGON((0 0,1 0,1 0))"), false);
    test::apply(from_wkt<OG>("POLYGON((0 0,1 0,0 0))"), false);
    test::apply(from_wkt<OG>("POLYGON((0 0,1 0,1 1,0 0))"), AllowDuplicates);
    test::apply(from_wkt<OG>("POLYGON((0 0,1 0,1 0,1 1))"), AllowDuplicates);
    test::apply(from_wkt<OG>("POLYGON((0 0,1 0,1 0,1 1,0 0))"),
                AllowDuplicates);

    // duplicate points in interior ring
    test::apply(from_wkt<OG>("POLYGON((0 0,10 0,10 10,0 10),(1 1,1 1,1 1))"),
                false);
    test::apply(from_wkt<OG>("POLYGON((0 0,10 0,10 10,0 10),(1 1,2 1,2 1))"),
                false);
    test::apply(from_wkt<OG>("POLYGON((0 0,10 0,10 10,0 10),(1 1,2 1,1 1))"),
                false);
    test::apply(from_wkt<OG>("POLYGON((0 0,10 0,10 10,0 10),(1 1,2 2,2 1,1 1))"),
                AllowDuplicates);
    test::apply(from_wkt<OG>("POLYGON((0 0,10 0,10 10,0 10),(1 1,2 2,2 2,2 1))"),
                AllowDuplicates);
    test::apply(from_wkt<OG>("POLYGON((0 0,10 0,10 10,0 10),(1 1,2 2,2 1,2 1,1 1))"),
                AllowDuplicates);

    // with spikes in exterior ring
    test::apply(from_wkt<OG>("POLYGON((0 0,2 0,2 2,0 2,1 2))"), false);
    test::apply(from_wkt<OG>("POLYGON((0 0,2 0,1 0,2 2))"), false);
    test::apply(from_wkt<OG>("POLYGON((0 0,1 0,2 0,1 0,4 0,4 4))"), false);
    test::apply(from_wkt<OG>("POLYGON((0 0,2 0,2 2,1 0))"), false);
    test::apply(from_wkt<OG>("POLYGON((0 0,2 0,1 0))"), false);
    test::apply(from_wkt<OG>("POLYGON((0 0,5 0,5 5,4 4,5 5,0 5))"), false);
    test::apply(from_wkt<OG>("POLYGON((0 0,5 0,5 5,4 4,3 3,5 5,0 5))"), false);

    // with spikes in interior ring
    test::apply(from_wkt<OG>("POLYGON((0 0,10 0,10 10,0 10),(1 1,3 1,3 3,1 3,2 3))"),
                false);
    test::apply(from_wkt<OG>("POLYGON((0 0,10 0,10 10,0 10),(1 1,3 1,2 1,3 3))"),
                false);
    test::apply(from_wkt<OG>("POLYGON((0 0,10 0,10 10,0 10),(1 1,2 1,3 1,2 1,4 1,4 4))"),
                false);
    test::apply(from_wkt<OG>("POLYGON((0 0,10 0,10 10,0 10),(1 1,3 1,3 3,2 1))"),
                false);
    test::apply(from_wkt<OG>("POLYGON((0 0,10 0,10 10,0 10),(1 1,3 1,2 1))"),
                false);

    // with self-crossings in exterior ring
    test::apply(from_wkt<OG>("POLYGON((0 0,5 0,5 5,3 -1,0 5))"),
                false);

    // example from Norvald Ryeng
    test::apply(from_wkt<OG>("POLYGON((100 1300,140 1300,140 170,100 1700))"),
                false);
    // and with point order reversed
    test::apply(from_wkt<OG>("POLYGON((100 1300,100 1700,140 170,140 1300))"),
                false);

    // with self-crossings in interior ring
    test::apply(from_wkt<OG>("POLYGON((0 0,10 0,10 10,0 10),(3 3,3 7,4 6,2 6))"),
                false);

    // with self-crossings between rings
    test::apply(from_wkt<OG>("POLYGON((0 0,5 0,5 5,0 5),(1 1,2 1,1 -1))"),
                false);

    // with self-intersections in exterior ring
    test::apply(from_wkt<OG>("POLYGON((0 0,5 0,5 5,3 5,3 0,2 0,2 5,0 5))"),
                false);

    test::apply(from_wkt<OG>("POLYGON((0 0,5 0,5 5,3 5,3 0,2 5,0 5))"),
                false);

    test::apply(from_wkt<OG>("POLYGON((0 0,5 0,5 1,1 1,1 2,2 2,3 1,4 2,5 2,5 5,0 5))"),
                false);

    // next two suggested by Adam Wulkiewicz
    test::apply(from_wkt<OG>("POLYGON((0 0,5 0,5 5,0 5,4 4,2 2,0 5))"), false);
    test::apply(from_wkt<OG>("POLYGON((0 0,5 0,5 5,1 4,4 4,4 1,0 5))"),
               false);
    test::apply(from_wkt<OG>("POLYGON((0 0,5 0,5 5,4 4,1 4,1 1,4 1,4 4,0 5))"),
               false);
    test::apply(from_wkt<OG>("POLYGON((0 0,5 0,5 5,4 4,4 1,1 1,1 4,4 4,0 5))"),
               false);

    // with self-intersections in interior ring
    test::apply(from_wkt<OG>("POLYGON((-10 -10,10 -10,10 10,-10 10),(0 0,5 0,5 5,3 5,3 0,2 0,2 5,0 5))"),
                false);
    test::apply(from_wkt<OG>("POLYGON((-10 -10,10 -10,10 10,-10 10),(0 0,5 0,5 5,3 5,3 0,2 5,0 5))"),
                false);

    test::apply(from_wkt<OG>("POLYGON((-10 -10,10 -10,10 10,-10 10),(0 0,5 0,5 1,1 1,1 2,2 2,3 1,4 2,5 2,5 5,0 5))"),
                false);

    // with self-intersections between rings
    // hole has common segment with exterior ring
    test::apply(from_wkt<OG>("POLYGON((0 0,10 0,10 10,0 10),(1 1,1 10,2 10,2 1))"),
                false);
    test::apply(from_wkt<OG>("POLYGON((0 0,0 0,10 0,10 10,0 10,0 10),(1 1,1 10,1 10,2 10,2 10,2 1))"),
                false);
    // hole touches exterior ring at one point
    test::apply(from_wkt<OG>("POLYGON((0 0,10 0,10 10,0 10),(1 1,1 10,2 1))"),
                true);
    // "hole" is outside the exterior ring, but touches it
    test::apply(from_wkt<OG>("POLYGON((0 0,10 0,10 10,0 10),(5 10,4 11,6 11))"),
                false);
    // hole touches exterior ring at vertex
    test::apply(from_wkt<OG>("POLYGON((0 0,10 0,10 10,0 10),(0 0,1 4,4 1))"),
                true);
    // "hole" is completely outside the exterior ring
    test::apply(from_wkt<OG>("POLYGON((0 0,10 0,10 10,0 10),(20 20,20 21,21 21,21 20))"),
                false);
    // two "holes" completely outside the exterior ring, that touch
    // each other
    test::apply(from_wkt<OG>("POLYGON((0 0,10 0,10 10,0 10),(20 0,25 10,21 0),(30 0,25 10,31 0))"),
                false);

    // example from Norvald Ryeng
    test::apply(from_wkt<OG>("POLYGON((58 31,56.57 30,62 33),(35 9,28 14,31 16),(23 11,29 5,26 4))"),
                false);
    // and with points reversed
    test::apply(from_wkt<OG>("POLYGON((58 31,62 33,56.57 30),(35 9,31 16,28 14),(23 11,26 4,29 5))"),
                false);

    // "hole" is completely inside another "hole"
    test::apply(from_wkt<OG>("POLYGON((0 0,10 0,10 10,0 10),(1 1,1 9,9 9,9 1),(2 2,2 8,8 8,8 2))"),
                false);
    test::apply(from_wkt<OG>("POLYGON((0 0,10 0,10 10,0 10),(1 1,1 9,9 9,9 1),(2 2,8 2,8 8,2 8))"),
               false);

    // "hole" is inside another "hole" (touching)
    test::apply(from_wkt<OG>("POLYGON((0 0,10 0,10 10,0 10),(1 1,1 9,9 9,9 1),(2 2,2 8,8 8,9 6,8 2))"),
                false);
    test::apply(from_wkt<OG>("POLYGON((0 0,10 0,10 10,0 10),(1 1,1 9,9 9,9 1),(2 2,8 2,9 6,8 8,2 8))"),
                false);
    test::apply(from_wkt<OG>("POLYGON((0 0,10 0,10 10,0 10),(1 1,9 1,9 9,1 9),(2 2,2 8,8 8,9 6,8 2))"),
                false);
    test::apply(from_wkt<OG>("POLYGON((0 0,10 0,10 10,0 10),(1 1,9 1,9 9,1 9),(2 2,8 2,9 6,8 8,2 8))"),
                false);
    // hole touches exterior ring at two points
    test::apply(from_wkt<OG>("POLYGON((0 0,10 0,10 10,0 10),(5 0,0 5,5 5))"),
                false);

    // cases with more holes
    // two holes, touching the exterior at the same point
    test::apply(from_wkt<OG>("POLYGON((0 0,10 0,10 10,0 10),(0 0,1 9,2 9),(0 0,9 2,9 1))"),
                true);
    test::apply(from_wkt<OG>("POLYGON((0 0,0 0,10 0,10 10,0 10,0 0),(0 0,0 0,1 9,2 9),(0 0,0 0,9 2,9 1))"),
                AllowDuplicates);
    test::apply(from_wkt<OG>("POLYGON((0 10,0 0,0 0,0 0,10 0,10 10),(2 9,0 0,0 0,1 9),(9 1,0 0,0 0,9 2))"),
                AllowDuplicates);
    // two holes, one inside the other
    test::apply(from_wkt<OG>("POLYGON((0 0,10 0,10 10,0 10),(0 0,1 9,9 1),(0 0,4 5,5 4))"),
                false);
    // 1st hole touches has common segment with 2nd hole
    test::apply(from_wkt<OG>("POLYGON((0 0,10 0,10 10,0 10),(1 1,1 5,5 5,5 1),(5 4,5 8,8 8,8 4))"),
                false);
    // 1st hole touches 2nd hole at two points
    test::apply(from_wkt<OG>("POLYGON((0 0,10 0,10 10,0 10),(1 1,1 9,9 9,9 8,2 8,2 1),(2 5,5 8,5 5))"),
                false);
    // polygon with many holes, where the last two touch at two points
    test::apply(from_wkt<OG>("POLYGON((0 0,20 0,20 20,0 20),(1 18,1 19,2 19,2 18),(3 18,3 19,4 19,4 18),(5 18,5 19,6 19,6 18),(7 18,7 19,8 19,8 18),(9 18,9 19,10 19,10 18),(11 18,11 19,12 19,12 18),(13 18,13 19,14 19,14 18),(15 18,15 19,16 19,16 18),(17 18,17 19,18 19,18 18),(1 1,1 9,9 9,9 8,2 8,2 1),(2 5,5 8,5 5))"),
                false);
    // two holes completely inside exterior ring but touching each
    // other at a point
    test::apply(from_wkt<OG>("POLYGON((0 0,10 0,10 10,0 10),(1 1,1 9,2 9),(1 1,9 2,9 1))"),
                true);    
    // four holes, each two touching at different points
    test::apply(from_wkt<OG>("POLYGON((0 0,10 0,10 10,0 10),(0 10,2 1,1 1),(0 10,4 1,3 1),(10 10,9 1,8 1),(10 10,7 1,6 1))"),
                true);
    // five holes, with two pairs touching each at some point, and
    // fifth hole creating a disconnected component for the interior
    test::apply(from_wkt<OG>("POLYGON((0 0,10 0,10 10,0 10),(0 10,2 1,1 1),(0 10,4 1,3 1),(10 10,9 1,8 1),(10 10,7 1,6 1),(4 1,4 4,6 4,6 1))"),
                false);
    // five holes, with two pairs touching each at some point, and
    // fifth hole creating three disconnected components for the interior
    test::apply(from_wkt<OG>("POLYGON((0 0,10 0,10 10,0 10),(0 10,2 1,1 1),(0 10,4 1,3 1),(10 10,9 1,8 1),(10 10,7 1,6 1),(4 1,4 4,6 4,6 1,5 0))"),
                false);

    // both examples: a polygon with one hole, where the hole contains
    // the exterior ring
    test::apply(from_wkt<OG>("POLYGON((0 0,1 0,1 1,0 1),(-10 -10,-10 10,10 10,10 -10))"),
                false);
    test::apply(from_wkt<OG>("POLYGON((-10 -10,1 0,1 1,0 1),(-10 -10,-10 10,10 10,10 -10))"),
                false);
}

template <typename Point>
inline void test_doc_example_polygon()
{
#ifdef BOOST_GEOMETRY_TEST_DEBUG
    std::cout << std::endl << std::endl;
    std::cout << "************************************" << std::endl;
    std::cout << " is_valid: doc example polygon " << std::endl;
    std::cout << "************************************" << std::endl;
#endif

    typedef bg::model::polygon<Point> CCW_CG;

    CCW_CG poly;

    typedef validity_tester_areal<true> tester;
    typedef test_valid<tester, CCW_CG> test;

    test::apply(from_wkt<CCW_CG>("POLYGON((0 0,0 10,10 10,10 0,0 0),(0 0,9 1,9 2,0 0),(0 0,2 9,1 9,0 0),(2 9,9 2,9 9,2 9))"),
                false);
}

BOOST_AUTO_TEST_CASE( test_is_valid_polygon )
{
    bool const allow_duplicates = true;
    bool const do_not_allow_duplicates = !allow_duplicates;

    test_open_polygons<point_type, allow_duplicates>();
    test_open_polygons<point_type, do_not_allow_duplicates>();
    test_doc_example_polygon<point_type>();
}

template <typename Point, bool AllowDuplicates>
void test_open_multipolygons()
{
#ifdef BOOST_GEOMETRY_TEST_DEBUG
    std::cout << std::endl << std::endl;
    std::cout << "************************************" << std::endl;
    std::cout << " is_valid: MULTIPOLYGON (open) " << std::endl;
    std::cout << "************************************" << std::endl;
    std::cout << "DUPLICATES ALLOWED? "
              << std::boolalpha
              << AllowDuplicates
              << std::noboolalpha
              << std::endl;
#endif

    // cw, ccw, open and closed polygons
    typedef bg::model::polygon<point_type,false,false> ccw_open_polygon_type;
    typedef bg::model::polygon<point_type,false,true>  ccw_closed_polygon_type;
    typedef bg::model::polygon<point_type,true,false>  cw_open_polygon_type;
    typedef bg::model::polygon<point_type,true,true>   cw_closed_polygon_type;

    typedef bg::model::multi_polygon<ccw_open_polygon_type> OG;
    typedef bg::model::multi_polygon<ccw_closed_polygon_type> CG;
    typedef bg::model::multi_polygon<cw_open_polygon_type> CW_OG;
    typedef bg::model::multi_polygon<cw_closed_polygon_type> CW_CG;

    typedef validity_tester_areal<AllowDuplicates> tester;
    typedef test_valid<tester, OG, CG, CW_OG, CW_CG> test;

    // not enough points
    test::apply(from_wkt<OG>("MULTIPOLYGON((()))"), false);
    test::apply(from_wkt<OG>("MULTIPOLYGON(((0 0)),(()))"), false);
    test::apply(from_wkt<OG>("MULTIPOLYGON(((0 0,1 0)))"), false);

    // two disjoint polygons
    test::apply(from_wkt<OG>("MULTIPOLYGON(((0 0,1 0,1 1,0 1)),((2 2,3 2,3 3,2 3)))"),
                true);

    // two disjoint polygons with multiple points
    test::apply(from_wkt<OG>("MULTIPOLYGON(((0 0,1 0,1 0,1 1,0 1)),((2 2,3 2,3 3,3 3,2 3)))"),
                AllowDuplicates);

    // two polygons touch at a point
    test::apply(from_wkt<OG>("MULTIPOLYGON(((0 0,1 0,1 1,0 1)),((1 1,2 1,2 2,1 2)))"),
                true);

    // two polygons share a segment at a point
    test::apply(from_wkt<OG>("MULTIPOLYGON(((0 0,1.5 0,1.5 1,0 1)),((1 1,2 1,2 2,1 2)))"),
                false);

    // one polygon inside another and boundaries touching
    test::apply(from_wkt<OG>("MULTIPOLYGON(((0 0,10 0,10 10,0 10)),((0 0,9 1,9 2)))"),
                false);

    // one polygon inside another and boundaries not touching
    test::apply(from_wkt<OG>("MULTIPOLYGON(((0 0,10 0,10 10,0 10)),((1 1,9 1,9 2)))"),
                false);

    // free space is disconnected
    test::apply(from_wkt<OG>("MULTIPOLYGON(((0 0,1 0,1 1,0 1)),((1 1,2 1,2 2,1 2)),((0 1,0 2,-1 2,-1 -1)),((1 2,1 3,0 3,0 2)))"),
                true);

    // multi-polygon with a polygon inside the hole of another polygon
    test::apply(from_wkt<OG>("MULTIPOLYGON(((0 0,100 0,100 100,0 100),(1 1,1 99,99 99,99 1)),((2 2,98 2,98 98,2 98)))"),
                true);
    test::apply(from_wkt<OG>("MULTIPOLYGON(((0 0,100 0,100 100,0 100),(1 1,1 99,99 99,99 1)),((1 1,98 2,98 98,2 98)))"),
                true);

    // test case suggested by Barend Gehrels: take two valid polygons P1 and
    // P2 with holes H1 and H2, respectively, and consider P2 to be
    // fully inside H1; now invalidate the multi-polygon by
    // considering H2 as a hole of P1 and H1 as a hole of P2; this
    // should be invalid
    //
    // first the valid case:
    test::apply(from_wkt<OG>("MULTIPOLYGON(((0 0,100 0,100 100,0 100),(1 1,1 99,99 99,99 1)),((2 2,98 2,98 98,2 98),(3 3,3 97,97 97,97 3)))"),
                true);
    // and the invalid case:
    test::apply(from_wkt<OG>("MULTIPOLYGON(((0 0,100 0,100 100,0 100),(3 3,3 97,97 97,97 3)),((2 2,98 2,98 98,2 98),(1 1,1 99,99 99,99 1)))"),
                false);
}

BOOST_AUTO_TEST_CASE( test_is_valid_multipolygon )
{
    bool const allow_duplicates = true;
    bool const do_not_allow_duplicates = !allow_duplicates;

    test_open_multipolygons<point_type, allow_duplicates>();
    test_open_multipolygons<point_type, do_not_allow_duplicates>();
}

BOOST_AUTO_TEST_CASE( test_is_valid_variant )
{
#ifdef BOOST_GEOMETRY_TEST_DEBUG
    std::cout << std::endl << std::endl;
    std::cout << "************************************" << std::endl;
    std::cout << " is_valid: variant support" << std::endl;
    std::cout << "************************************" << std::endl;
#endif

    typedef bg::model::polygon<point_type> polygon_type; // cw, closed

    typedef boost::variant
        <
            linestring_type, multi_linestring_type, polygon_type
        > variant_geometry;
    typedef test_valid_variant<variant_geometry> test;

    variant_geometry vg;

    linestring_type valid_linestring =
        from_wkt<linestring_type>("LINESTRING(0 0,1 0)");
    multi_linestring_type invalid_multi_linestring =
        from_wkt<multi_linestring_type>("MULTILINESTRING((0 0,1 0),(0 0))");
    polygon_type valid_polygon =
        from_wkt<polygon_type>("POLYGON((0 0,1 1,1 0,0 0))");
    polygon_type invalid_polygon =
        from_wkt<polygon_type>("POLYGON((0 0,1 1,1 0))");

    vg = valid_linestring;
    test::apply(vg, true);
    vg = invalid_multi_linestring;
    test::apply(vg, false);
    vg = valid_polygon;
    test::apply(vg, true);
    vg = invalid_polygon;
    test::apply(vg, false);
}
