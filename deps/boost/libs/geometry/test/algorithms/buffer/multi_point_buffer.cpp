// Boost.Geometry (aka GGL, Generic Geometry Library)
// Unit Test

// Copyright (c) 2012-2015 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <test_buffer.hpp>

#include <boost/geometry/multi/geometries/multi_geometries.hpp>

static std::string const simplex = "MULTIPOINT((5 5),(7 7))";
static std::string const three = "MULTIPOINT((5 8),(9 8),(7 11))";

// Generated error (extra polygon on top of rest) at distance 14.0:
static std::string const multipoint_a = "MULTIPOINT((39 44),(38 37),(41 29),(15 33),(58 39))";

// Just one with holes at distance ~ 15
static std::string const multipoint_b = "MULTIPOINT((5 56),(98 67),(20 7),(58 60),(10 4),(75 68),(61 68),(75 62),(92 26),(74 6),(67 54),(20 43),(63 30),(45 7))";

// Grid, U-form, generates error for square point at 0.54 (top cells to control rescale)
static std::string const grid_a = "MULTIPOINT(5 0,6 0,7 0,  5 1,7 1,  0 13,8 13)";

static std::string const mysql_report_2015_02_25_1 = "MULTIPOINT(-9 19,9 -6,-4 4,16 -14,-3 16,14 9)";
static std::string const mysql_report_2015_02_25_2 = "MULTIPOINT(-2 11,-15 3,6 4,-14 0,20 -7,-17 -1)";

template <bool Clockwise, typename P>
void test_all()
{
    typedef bg::model::polygon<P, Clockwise> polygon;
    typedef bg::model::multi_point<P> multi_point_type;

    bg::strategy::buffer::join_miter join_miter;
    bg::strategy::buffer::end_flat end_flat;
    typedef bg::strategy::buffer::distance_symmetric
    <
        typename bg::coordinate_type<P>::type
    > distance_strategy;
    bg::strategy::buffer::side_straight side_strategy;

    double const pi = boost::geometry::math::pi<double>();

    test_one<multi_point_type, polygon>("simplex1", simplex, join_miter, end_flat, 2.0 * pi, 1.0, 1.0);
    test_one<multi_point_type, polygon>("simplex2", simplex, join_miter, end_flat, 22.8372, 2.0, 2.0);
    test_one<multi_point_type, polygon>("simplex3", simplex, join_miter, end_flat, 44.5692, 3.0, 3.0);

    test_one<multi_point_type, polygon>("three1", three, join_miter, end_flat, 3.0 * pi, 1.0, 1.0);
    test_one<multi_point_type, polygon>("three2", three, join_miter, end_flat, 36.7592, 2.0, 2.0);
    test_one<multi_point_type, polygon>("three19", three, join_miter, end_flat, 33.6914, 1.9, 1.9);
    test_one<multi_point_type, polygon>("three21", three, join_miter, end_flat, 39.6394, 2.1, 2.1);
    test_one<multi_point_type, polygon>("three3", three, join_miter, end_flat, 65.533, 3.0, 3.0);

    test_one<multi_point_type, polygon>("multipoint_a", multipoint_a, join_miter, end_flat, 2049.98, 14.0, 14.0);
    test_one<multi_point_type, polygon>("multipoint_b", multipoint_b, join_miter, end_flat, 7109.88, 15.0, 15.0);
    test_one<multi_point_type, polygon>("multipoint_b1", multipoint_b, join_miter, end_flat, 6911.89, 14.7, 14.7);
    test_one<multi_point_type, polygon>("multipoint_b2", multipoint_b, join_miter, end_flat, 7174.79, 15.1, 15.1);


    // Grid tests
    {
        bg::strategy::buffer::point_square point_strategy;


        test_with_custom_strategies<multi_point_type, polygon>("grid_a50",
                grid_a, join_miter, end_flat,
                distance_strategy(0.5), side_strategy, point_strategy, 7.0);

#if defined(BOOST_GEOMETRY_BUFFER_INCLUDE_FAILING_TESTS)
        test_with_custom_strategies<multi_point_type, polygon>("grid_a54",
                grid_a, join_miter, end_flat,
                distance_strategy(0.54), side_strategy, point_strategy, 99);
#endif

    }

    test_with_custom_strategies<multi_point_type, polygon>("mysql_report_2015_02_25_1_800",
            mysql_report_2015_02_25_1, join_miter, end_flat,
            distance_strategy(6051788), side_strategy,
            bg::strategy::buffer::point_circle(800), 115057490003226.125, 1.0);
}

template <typename P>
void test_many_points_per_circle()
{
    // Tests for large distances / many points in circles.
    // Before Boost 1.58, this would (seem to) hang. It is solved by using monotonic sections in get_turns for buffer
    // This is more time consuming, only calculate this for counter clockwise
    // Reported by MySQL 2015-02-25
    //   SELECT ST_ASTEXT(ST_BUFFER(ST_GEOMFROMTEXT(''), 6051788, ST_BUFFER_STRATEGY('point_circle', 83585)));
    //   SELECT ST_ASTEXT(ST_BUFFER(ST_GEOMFROMTEXT(''), 5666962, ST_BUFFER_STRATEGY('point_circle', 46641))) ;

    typedef bg::model::polygon<P, false> polygon;
    typedef bg::model::multi_point<P> multi_point_type;

    bg::strategy::buffer::join_miter join_miter;
    bg::strategy::buffer::end_flat end_flat;
    typedef bg::strategy::buffer::distance_symmetric
    <
        typename bg::coordinate_type<P>::type
    > distance_strategy;
    bg::strategy::buffer::side_straight side_strategy;

    using bg::strategy::buffer::point_circle;

    double const tolerance = 1.0;

    // Strategies with many points, which are (very) slow in debug mode
    test_with_custom_strategies<multi_point_type, polygon>(
            "mysql_report_2015_02_25_1_8000",
            mysql_report_2015_02_25_1, join_miter, end_flat,
            distance_strategy(6051788), side_strategy, point_circle(8000),
            115058661065242.812, tolerance);

    test_with_custom_strategies<multi_point_type, polygon>(
            "mysql_report_2015_02_25_1",
            mysql_report_2015_02_25_1, join_miter, end_flat,
            distance_strategy(6051788), side_strategy, point_circle(83585),
            115058672785611.219, tolerance);

    // Takes about 20 seconds in release mode
    test_with_custom_strategies<multi_point_type, polygon>(
            "mysql_report_2015_02_25_1_250k",
            mysql_report_2015_02_25_1, join_miter, end_flat,
            distance_strategy(6051788), side_strategy, point_circle(250000),
            115058672880671.531, tolerance);

#if defined(BOOST_GEOMETRY_BUFFER_INCLUDE_FAILING_TESTS)
    // Takes too long, TODO improve turn_in_piece_visitor
    test_with_custom_strategies<multi_point_type, polygon>(
            "mysql_report_2015_02_25_1",
            mysql_report_2015_02_25_1, join_miter, end_flat,
            distance_strategy(6051788), side_strategy, point_circle(800000),
            115058672799999.999, tolerance); // area to be determined
#endif

    test_with_custom_strategies<multi_point_type, polygon>(
            "mysql_report_2015_02_25_2",
            mysql_report_2015_02_25_2, join_miter, end_flat,
            distance_strategy(5666962), side_strategy, point_circle(46641),
            100891031341757.344, tolerance);

}

int test_main(int, char* [])
{
    test_all<true, bg::model::point<double, 2, bg::cs::cartesian> >();
    test_all<false, bg::model::point<double, 2, bg::cs::cartesian> >();

#if defined(BOOST_GEOMETRY_COMPILER_MODE_RELEASE) && ! defined(BOOST_GEOMETRY_COMPILER_MODE_DEBUG)
    test_many_points_per_circle<bg::model::point<double, 2, bg::cs::cartesian> >();
#else
    std::cout << "Skipping some tests in debug or unknown mode" << std::endl;
#endif

    return 0;
}
