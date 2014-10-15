// Boost.Geometry (aka GGL, Generic Geometry Library)
// Unit Test

// Copyright (c) 2012-2014 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <test_buffer.hpp>

#include <boost/geometry/algorithms/buffer.hpp>
#include <boost/geometry/core/coordinate_type.hpp>

#include <boost/geometry/strategies/strategies.hpp>

#include <boost/geometry/geometries/point.hpp>
#include <boost/geometry/geometries/box.hpp>
#include <test_common/test_point.hpp>


static std::string const simplex = "LINESTRING(0 0,4 5)";
static std::string const straight = "LINESTRING(0 0,4 5,8 10)";
static std::string const one_bend = "LINESTRING(0 0,4 5,7 4)";
static std::string const two_bends = "LINESTRING(0 0,4 5,7 4,10 6)";
static std::string const overlapping = "LINESTRING(0 0,4 5,7 4,10 6, 10 2,2 2)";
static std::string const curve = "LINESTRING(2 7,3 5,5 4,7 5,8 7)";
static std::string const tripod = "LINESTRING(5 0,5 5,1 8,5 5,9 8)"; // with spike

static std::string const for_collinear = "LINESTRING(2 0,0 0,0 4,6 4,6 0,4 0)";
static std::string const for_collinear2 = "LINESTRING(2 1,2 0,0 0,0 4,6 4,6 0,4 0,4 1)";

static std::string const chained2 = "LINESTRING(0 0,1 1,2 2)";
static std::string const chained3 = "LINESTRING(0 0,1 1,2 2,3 3)";
static std::string const chained4 = "LINESTRING(0 0,1 1,2 2,3 3,4 4)";

static std::string const field_sprayer1 = "LINESTRING(76396.40464822574 410095.6795147947,76397.85016212701 410095.211865792,76401.30666443033 410095.0466387949,76405.05892643372 410096.1007777959,76409.45103273794 410098.257640797,76412.96309264141 410101.6522238015)";
static std::string const aimes120 = "LINESTRING(-2.505218 52.189211,-2.505069 52.189019,-2.504941 52.188854)";
static std::string const aimes167 = "LINESTRING(-2.378569 52.312133,-2.37857 52.312127,-2.378544 52.31209)";
static std::string const aimes175 = "LINESTRING(-2.3116 52.354326,-2.311555 52.35417,-2.311489 52.354145,-2.311335 52.354178)";
static std::string const aimes171 = "LINESTRING(-2.393161 52.265087,-2.393002 52.264965,-2.392901 52.264891)";
static std::string const aimes181 = "LINESTRING(-2.320686 52.43505,-2.320678 52.435016,-2.320697 52.434978,-2.3207 52.434977,-2.320741 52.434964,-2.320807 52.434964,-2.320847 52.434986,-2.320903 52.435022)";


template <typename P>
void test_all()
{
    typedef bg::model::linestring<P> linestring;
    typedef bg::model::polygon<P> polygon;

    bg::strategy::buffer::join_miter join_miter;
    bg::strategy::buffer::join_round join_round(100);
    bg::strategy::buffer::join_round_by_divide join_round_by_divide(4);
    bg::strategy::buffer::end_flat end_flat;
    bg::strategy::buffer::end_round end_round(100);

    // Simplex (join-type is not relevant)
    test_one<linestring, polygon>("simplex", simplex, join_miter, end_flat, 19.209, 1.5, 1.5);
    test_one<linestring, polygon>("simplex", simplex, join_miter, end_round, 26.2733, 1.5, 1.5);

    test_one<linestring, polygon>("simplex_asym_neg", simplex, join_miter, end_flat, 3.202, +1.5, -1.0);
    test_one<linestring, polygon>("simplex_asym_pos", simplex, join_miter, end_flat, 3.202, -1.0, +1.5);
    // Do not work yet:
    //    test_one<linestring, polygon>("simplex_asym_neg", simplex, join_miter, end_round, 3.202, +1.5, -1.0);
    //    test_one<linestring, polygon>("simplex_asym_pos", simplex, join_miter, end_round, 3.202, -1.0, +1.5);

    // Generates a reverse polygon, with a negative area, which will be made empty TODO decide about this
    test_one<linestring, polygon>("simplex_asym_neg_rev", simplex, join_miter, end_flat, 0, +1.0, -1.5);
    test_one<linestring, polygon>("simplex_asym_pos_rev", simplex, join_miter, end_flat, 0, -1.5, +1.0);

    test_one<linestring, polygon>("straight", straight, join_round, end_flat, 38.4187, 1.5, 1.5);
    test_one<linestring, polygon>("straight", straight, join_miter, end_flat, 38.4187, 1.5, 1.5);

    // One bend/two bends (tests join-type)
    test_one<linestring, polygon>("one_bend", one_bend, join_round, end_flat, 28.488, 1.5, 1.5);
    test_one<linestring, polygon>("one_bend", one_bend, join_miter, end_flat, 28.696, 1.5, 1.5);
    test_one<linestring, polygon>("one_bend", one_bend, join_round_by_divide, end_flat, 28.488, 1.5, 1.5);

    test_one<linestring, polygon>("one_bend", one_bend, join_round, end_round, 35.5603, 1.5, 1.5);
    test_one<linestring, polygon>("one_bend", one_bend, join_miter, end_round, 35.7601, 1.5, 1.5);

    test_one<linestring, polygon>("two_bends", two_bends, join_round, end_flat, 39.235, 1.5, 1.5);
    test_one<linestring, polygon>("two_bends", two_bends, join_round_by_divide, end_flat, 39.235, 1.5, 1.5);
    test_one<linestring, polygon>("two_bends", two_bends, join_miter, end_flat, 39.513, 1.5, 1.5);
    test_one<linestring, polygon>("two_bends_left", two_bends, join_round, end_flat, 20.028, 1.5, 0.0);
    test_one<linestring, polygon>("two_bends_left", two_bends, join_miter, end_flat, 20.225, 1.5, 0.0);
    test_one<linestring, polygon>("two_bends_right", two_bends, join_round, end_flat, 19.211, 0.0, 1.5);
    test_one<linestring, polygon>("two_bends_right", two_bends, join_miter, end_flat, 19.288, 0.0, 1.5);


    // Next (and all similar cases) which a offsetted-one-sided buffer has to be fixed. TODO
    //test_one<linestring, polygon>("two_bends_neg", two_bends, join_miter, end_flat, 99, +1.5, -1.0);
    //test_one<linestring, polygon>("two_bends_pos", two_bends, join_miter, end_flat, 99, -1.5, +1.0);
    //test_one<linestring,  polygon>("two_bends_neg", two_bends, join_round, end_flat,99, +1.5, -1.0);
    //test_one<linestring, polygon>("two_bends_pos", two_bends, join_round, end_flat, 99, -1.5, +1.0);

    test_one<linestring, polygon>("overlapping150", overlapping, join_round, end_flat, 65.6786, 1.5, 1.5);
    test_one<linestring, polygon>("overlapping150", overlapping, join_miter, end_flat, 68.140, 1.5, 1.5);

    // Different cases with intersection points on flat and (left/right from line itself)
    test_one<linestring, polygon>("overlapping_asym_150_010", overlapping, join_round, end_flat, 48.308, 1.5, 0.25);
    test_one<linestring, polygon>("overlapping_asym_150_010", overlapping, join_miter, end_flat, 50.770, 1.5, 0.25);
    test_one<linestring, polygon>("overlapping_asym_150_075", overlapping, join_round, end_flat, 58.506, 1.5, 0.75);
    test_one<linestring, polygon>("overlapping_asym_150_075", overlapping, join_miter, end_flat, 60.985, 1.5, 0.75);
    test_one<linestring, polygon>("overlapping_asym_150_100", overlapping, join_round, end_flat, 62.514, 1.5, 1.0);
    test_one<linestring, polygon>("overlapping_asym_150_100", overlapping, join_miter, end_flat, 64.984, 1.5, 1.0);

    // Having flat end
    test_one<linestring, polygon>("for_collinear", for_collinear, join_round, end_flat, 68.561, 2.0, 2.0);
    test_one<linestring, polygon>("for_collinear", for_collinear, join_miter, end_flat, 72, 2.0, 2.0);
#if defined(BOOST_GEOMETRY_BUFFER_INCLUDE_FAILING_TESTS)
    test_one<linestring, polygon>("for_collinear2", for_collinear2, join_round, end_flat, 74.387, 2.0, 2.0);
    test_one<linestring, polygon>("for_collinear2", for_collinear2, join_miter, end_flat, 78.0, 2.0, 2.0);
#endif

#if defined(BOOST_GEOMETRY_BUFFER_INCLUDE_FAILING_TESTS)
    // Having flat end causing self-intersection
    test_one<linestring, polygon>("curve", curve, join_round, end_flat, 54.8448, 5.0, 3.0);
    test_one<linestring, polygon>("curve", curve, join_miter, end_flat, 55.3875, 5.0, 3.0);
#endif

    test_one<linestring, polygon>("tripod", tripod, join_miter, end_flat, 74.25, 3.0);
    test_one<linestring, polygon>("tripod", tripod, join_miter, end_round, 116.6336, 3.0);

    test_one<linestring, polygon>("chained2", chained2, join_round, end_flat, 11.3137, 2.5, 1.5);
    test_one<linestring, polygon>("chained3", chained3, join_round, end_flat, 16.9706, 2.5, 1.5);
    test_one<linestring, polygon>("chained4", chained4, join_round, end_flat, 22.6274, 2.5, 1.5);

#if defined(BOOST_GEOMETRY_BUFFER_INCLUDE_FAILING_TESTS)
    // Having flat end causing self-intersection
    test_one<linestring, polygon>("field_sprayer1", reallife1, join_round, end_flat, 99, 16.5, 6.5);
#endif
    test_one<linestring, polygon>("field_sprayer1", field_sprayer1, join_round, end_round, 718.761877, 16.5, 6.5);
    test_one<linestring, polygon>("field_sprayer1", field_sprayer1, join_miter, end_round, 718.939628, 16.5, 6.5);

    double tolerance = 1.0e-10;

    test_one<linestring, polygon>("aimes120", aimes120, join_miter, end_flat, 1.62669948622351512e-08, 0.000018, 0.000018, false, tolerance);
    test_one<linestring, polygon>("aimes120", aimes120, join_round, end_round, 1.72842078427493107e-08, 0.000018, 0.000018, true, tolerance);

#if defined(BOOST_GEOMETRY_BUFFER_INCLUDE_FAILING_TESTS)
    // Having flat end causing self-intersection
    test_one<linestring, polygon>("aimes167", aimes167, join_miter, end_flat, 1.62669948622351512e-08, 0.000018, 0.000018, true, tolerance);
#endif
    test_one<linestring, polygon>("aimes167", aimes167, join_round, end_round, 2.85734813587623648e-09, 0.000018, 0.000018, true, tolerance);

    test_one<linestring, polygon>("aimes175", aimes175, join_miter, end_flat, 2.81111809385947709e-08, 0.000036, 0.000036, true, tolerance);
    test_one<linestring, polygon>("aimes175", aimes175, join_round, end_round, 3.21215765097804251e-08, 0.000036, 0.000036, true, tolerance);

    test_one<linestring, polygon>("aimes171", aimes171, join_miter, end_flat, 1.1721873249825876e-08, 0.000018, 0.000018, true, tolerance);
    test_one<linestring, polygon>("aimes171", aimes171, join_round, end_round, 1.2739093335767393e-08, 0.000018, 0.000018, true, tolerance);
    test_one<linestring, polygon>("aimes171", aimes171, join_round_by_divide, end_round, 1.2739093335767393e-08, 0.000018, 0.000018, true, tolerance);

    test_one<linestring, polygon>("aimes181", aimes181, join_miter, end_flat, 2.1729405830228643e-08, 0.000036, 0.000036, true, tolerance);
    test_one<linestring, polygon>("aimes181", aimes181, join_round, end_round, 2.57415564419716247e-08, 0.000036, 0.000036, true, tolerance);
    test_one<linestring, polygon>("aimes181", aimes181, join_round_by_divide, end_round, 2.57415564419716247e-08, 0.000036, 0.000036, true, tolerance);
}


//#define HAVE_TTMATH
#ifdef HAVE_TTMATH
#include <ttmath_stub.hpp>
#endif


int test_main(int, char* [])
{
    test_all<bg::model::point<double, 2, bg::cs::cartesian> >();
    //test_all<bg::model::point<tt, 2, bg::cs::cartesian> >();
    return 0;
}
