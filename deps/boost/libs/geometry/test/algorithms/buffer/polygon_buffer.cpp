// Boost.Geometry (aka GGL, Generic Geometry Library)
// Unit Test

// Copyright (c) 2012-2014 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <test_buffer.hpp>


static std::string const simplex
    = "POLYGON ((0 0,1 5,6 1,0 0))";
static std::string const concave_simplex
    = "POLYGON ((0 0,3 5,3 3,5 3,0 0))";
static std::string const spike_simplex
    = "POLYGON ((0 0,1 5,3 3,5 5,3 3,5 1,0 0))";
static std::string const chained_box
    = "POLYGON((0 0,0 4,4 4,8 4,12 4,12 0,8 0,4 0,0 0))";

static std::string const join_types
    = "POLYGON ((0 0,0 4,4 4,2 6,0 8,2 6,4 8,8 4,4 0,0 0))"; // first 4 join types are all different: convex, concave, continue, spike

static std::string const donut_simplex
    = "POLYGON ((0 0,1 9,8 1,0 0),(1 1,4 1,1 4,1 1))";
static std::string const donut_diamond
    = "POLYGON((15 0,15 15,30 15,30 0,15 0),(26 11,22 14,19 10,23 07,26 11))";
static std::string const letter_L
    = "POLYGON ((0 0,0 4,1 4,1 1,3 1,3 0,0 0))";
static std::string const indentation
    = "POLYGON ((0 0,0 5,4 5,4 4,3 3,2 4,2 1,3 2,4 1,4 0,0 0))";
static std::string const funnelgate
    = "POLYGON((0 0,0 7,7 7,7 0,5 0,5 1,6 6,1 6,2 1,2 0,0 0))";
static std::string const gammagate
    = "POLYGON((0 0,0 6,9 6,9 0,4 0,4 2,7 2,7 4,2 4,2 0,0 0))";
static std::string const fork_a
    = "POLYGON((0 0,0 6,9 6,9 0,4 0,4 2,7 2,7 4,6 4,6 5,5 5,5 4,4 4,4 5,3 5,3 4,2 4,2 0,0 0))";
static std::string const fork_b
    = "POLYGON((0 0,0 8,14 8,14 0,4 0,4 2,13 2,13 4,12 4,12 7,9 7,9 4,7 4,7 7,4 7,4 4,2 4,2 0,0 0))";
static std::string const fork_c
    = "POLYGON((0 0,0 9,12 9,12 0,4 0,4 4,6 4,6 2,8 2,8 4,10 4,10 7,6 7,6 6,2 6,2 0,0 0))";

static std::string const arrow
    = "POLYGON ((1 0,1 5,0.5 4.5,2 10,3.5 4.5,3 5,3 0,1 0))";
static std::string const tipped_aitch
    = "POLYGON ((0 0,0 3,3 3,3 4,0 4,0 7,7 7,7 4,4 4,4 3,7 3,7 0,0 0))";
static std::string const snake
    = "POLYGON ((0 0,0 3,3 3,3 4,0 4,0 7,8 7,8 4,6 4,6 3,8 3,8 0,7 0,7 2,5 2"
                ",5 5,7 5,7 6,1 6,1 5,4 5,4 2,1 2,1 1,6 1,6 0,0 0))";
static std::string const church
    = "POLYGON ((0 0,0 3,2.999 3,3 8,3 0,0 0))";
static std::string const flower
    = "POLYGON ((1 0,1 10,9 10,9 0,4.99 0,4.99 5.5,4.5 6,5 6.5,5.5 6,5.01 5.5,5.01 0.01,5.25 0.01,5.25 5,6 3,8 5,6 6,8 7,6 9,5 7,4 9,2 7,4 6,2 5,4 3,4.75 5,4.75 0,1 0))";

static std::string const saw
    = "POLYGON((1 3,1 8,1.5 6,5 8,5.5 6,9 8,9.5 6,13 8,13 3,1 3))";

static std::string const bowl
    = "POLYGON((1 2,1 7,2 7,3 5,5 4,7 5,8 7,9 7,9 2,1 2))";

// Triangle with segmented sides, closing point at longest side
static std::string const triangle
    = "POLYGON((4 5,5 4,4 4,3 4,3 5,3 6,4 5))";

// Real-life examples
static std::string const county1
    = "POLYGON((-111.700 41.200 ,-111.681388 41.181739 ,-111.682453 41.181506 ,-111.684052 41.180804 ,-111.685295 41.180538 ,-111.686318 41.180776 ,-111.687517 41.181416 ,-111.688982 41.181520 ,-111.690670 41.181523 ,-111.692135 41.181460 ,-111.693646 41.182034 ,-111.695156 41.182204 ,-111.696489 41.182274 ,-111.697775 41.182075 ,-111.698974 41.181539 ,-111.700485 41.182348 ,-111.701374 41.182955 ,-111.700 41.200))";

//static std::string const invalid_parcel
//    =  "POLYGON((116042.20 464335.07,116056.35 464294.59,116066.41 464264.16,116066.44 464264.09,116060.35 464280.93,116028.89 464268.43,116028.89 464268.44,116024.74 464280.7,116018.91 464296.71,116042.2 464335.07))";

static std::string const parcel1
    = "POLYGON((225343.489 585110.376,225319.123 585165.731,225323.497 585167.287,225323.134 585167.157,225313.975 585169.208,225321.828 585172,225332.677 585175.83,225367.032 585186.977,225401.64 585196.671,225422.799 585201.029,225429.784 585202.454,225418.859 585195.112,225423.803 585196.13,225425.389 585196.454,225397.027 585165.48,225363.802 585130.372,225354.086 585120.261,225343.489 585110.376))";
static std::string const parcel2
    = "POLYGON((173356.986490154 605912.122380707,173358.457939143 605902.891897507,173358.458257372 605902.889901239,173214.162964795 605901.13020255,173214.162746654 605901.132200038,173213.665 605905.69,173212.712441616 605913.799985923,173356.986490154 605912.122380707))";
static std::string const parcel3
    = "POLYGON((120528.56 462115.62,120533.4 462072.1,120533.4 462072.01,120533.39 462071.93,120533.36 462071.86,120533.33 462071.78,120533.28 462071.72,120533.22 462071.66,120533.15 462071.61,120533.08 462071.58,120533 462071.55,120532.92 462071.54,120467.68 462068.66,120468.55 462059.04,120517.39 462062.87,120517.47 462062.87,120517.55 462062.86,120517.62 462062.83,120517.69 462062.79,120517.76 462062.74,120517.81 462062.68,120517.86 462062.62,120517.89 462062.55,120517.92 462062.47,120530.49 461998.63,120530.5 461998.55,120530.49 461998.47,120530.47 461998.39,120530.44 461998.31,120530.4 461998.24,120530.35 461998.18,120530.28 461998.13,120530.21 461998.09,120530.13 461998.06,120482.19 461984.63,120485 461963.14,120528.2 461950.66,120528.28 461950.63,120528.35 461950.59,120528.42 461950.53,120528.47 461950.47,120528.51 461950.4,120528.54 461950.32,120528.56 461950.24,120528.56 461950.15,120528.55 461950.07,120528.53 461949.99,120528.49 461949.92,120528.44 461949.85,120497.49 461915.03,120497.43 461914.98,120497.37 461914.93,120497.3 461914.9,120497.23 461914.88,120497.15 461914.86,120424.61 461910.03,120424.53 461910.03,120424.45 461910.05,120424.37 461910.07,120424.3 461910.11,120424.24 461910.16,120424.18 461910.22,120424.14 461910.29,120424.11 461910.37,120424.09 461910.45,120424.08 461910.53,120424.08 461967.59,120424.08 461967.67,120424.1 461967.75,120424.14 461967.82,120424.18 461967.89,120424.23 461967.95,120424.3 461968,120424.37 461968.04,120424.44 461968.07,120424.52 461968.09,120473.31 461973.83,120469.63 461993.16,120399.48 461986.43,120399.4 461986.43,120399.32 461986.44,120399.25 461986.47,120399.17 461986.5,120399.11 461986.55,120399.05 461986.61,120399.01 461986.67,120398.97 461986.74,120398.95 461986.82,120398.93 461986.9,120394.1 462057.5,120394.1 462057.58,120394.11 462057.66,120394.14 462057.74,120394.18 462057.81,120394.23 462057.87,120394.29 462057.93,120394.35 462057.97,120394.43 462058,120394.5 462058.03,120394.58 462058.03,120458.74 462059.95,120455.16 462072.48,120396.57 462067.68,120396.49 462067.68,120396.4 462067.69,120396.32 462067.72,120396.25 462067.76,120396.18 462067.82,120396.13 462067.88,120396.08 462067.96,120396.05 462068.04,120396.03 462068.12,120392.17 462103.9,120392.16 462103.99,120392.18 462104.07,120392.2 462104.15,120392.24 462104.22,120392.29 462104.29,120392.35 462104.35,120392.42 462104.4,120392.5 462104.43,120392.58 462104.45,120392.66 462104.46,120393.63 462104.46,120393.63 462103.46,120393.22 462103.46,120396.98 462068.71,120455.49 462073.51,120455.57 462073.51,120455.66 462073.49,120455.74 462073.46,120455.81 462073.42,120455.88 462073.37,120455.93 462073.3,120455.98 462073.23,120456.01 462073.15,120459.88 462059.61,120459.89 462059.52,120459.9 462059.44,120459.88 462059.36,120459.86 462059.28,120459.82 462059.21,120459.77 462059.14,120459.72 462059.08,120459.65 462059.04,120459.57 462059,120459.49 462058.98,120459.41 462058.97,120395.13 462057.05,120399.9 461987.48,120469.99 461994.2,120470.07 461994.2,120470.15 461994.19,120470.23 461994.16,120470.3 461994.13,120470.37 461994.08,120470.42 461994.02,120470.47 461993.95,120470.5 461993.88,120470.53 461993.8,120474.4 461973.48,120474.4 461973.4,120474.4 461973.32,120474.38 461973.24,120474.35 461973.16,120474.31 461973.09,120474.25 461973.03,120474.19 461972.98,120474.12 461972.94,120474.04 461972.91,120473.96 461972.9,120425.08 461967.14,120425.08 461911.06,120496.88 461915.85,120527.16 461949.92,120484.4 461962.27,120484.33 461962.3,120484.25 461962.35,120484.19 461962.4,120484.14 461962.46,120484.09 461962.53,120484.06 461962.61,120484.05 461962.69,120481.14 461984.93,120481.14 461985.01,120481.15 461985.09,120481.17 461985.17,120481.2 461985.24,120481.25 461985.31,120481.3 461985.36,120481.36 461985.41,120481.43 461985.45,120481.51 461985.48,120529.42 461998.9,120517.02 462061.84,120468.14 462058,120468.05 462058,120467.97 462058.02,120467.89 462058.05,120467.81 462058.09,120467.75 462058.15,120467.69 462058.22,120467.65 462058.29,120467.62 462058.37,120467.6 462058.46,120466.64 462069.1,120466.63 462069.18,120466.65 462069.26,120466.67 462069.33,120466.71 462069.4,120466.76 462069.47,120466.81 462069.53,120466.88 462069.57,120466.95 462069.61,120467.03 462069.63,120467.11 462069.64,120532.34 462072.52,120527.62 462115.03,120391.73 462106.36,120391.66 462107.36,120528.03 462116.06,120528.12 462116.06,120528.2 462116.04,120528.28 462116.02,120528.35 462115.97,120528.42 462115.92,120528.47 462115.85,120528.51 462115.78,120528.54 462115.7,120528.56 462115.62))";

static std::string const parcel3_bend // of parcel_3 - clipped
    = "POLYGON((120399.40000152588 461986.43000030518, 120399.47999954224 461986.43000030518, 120403 461986.76769953477, 120403 461987.777217312, 120399.90000152588 461987.47999954224, 120399.72722010587 461990, 120398.71791817161 461990, 120398.93000030518 461986.90000152588, 120398.95000076294 461986.81999969482, 120398.9700012207 461986.74000167847, 120399.00999832153 461986.66999816895, 120399.04999923706 461986.61000061035, 120399.11000061035 461986.54999923706, 120399.16999816895 461986.5, 120399.25 461986.4700012207, 120399.31999969482 461986.43999862671, 120399.40000152588 461986.43000030518))";



template <typename P>
void test_all()
{
    typedef bg::model::polygon<P> polygon_type;

    bg::strategy::buffer::join_miter join_miter(10.0);
    bg::strategy::buffer::join_round join_round(100);
    bg::strategy::buffer::end_flat end_flat;
    bg::strategy::buffer::end_round end_round(100);

    test_one<polygon_type, polygon_type>("simplex", simplex, join_round, end_flat, 47.9408, 1.5);
    test_one<polygon_type, polygon_type>("simplex", simplex, join_miter, end_flat, 52.8733, 1.5);

    test_one<polygon_type, polygon_type>("concave_simplex", concave_simplex, join_round, end_flat, 14.5616, 0.5);
    test_one<polygon_type, polygon_type>("concave_simplex", concave_simplex, join_miter, end_flat, 16.3861, 0.5);

    test_one<polygon_type, polygon_type>("spike_simplex15", spike_simplex, join_round, end_round, 50.3633, 1.5);
    test_one<polygon_type, polygon_type>("spike_simplex15", spike_simplex, join_miter, end_flat, 51.5509, 1.5);

#if defined(BOOST_GEOMETRY_BUFFER_INCLUDE_FAILING_TESTS)
    test_one<polygon_type, polygon_type>("spike_simplex30", spike_simplex, join_round, end_round, 100.9199, 3.0);
    test_one<polygon_type, polygon_type>("spike_simplex30", spike_simplex, join_miter, end_flat, 120.9859, 3.0);
#endif
    test_one<polygon_type, polygon_type>("spike_simplex150", spike_simplex, join_round, end_round, 998.9530, 15.0);
#if defined(BOOST_GEOMETRY_BUFFER_INCLUDE_FAILING_TESTS)
    test_one<polygon_type, polygon_type>("spike_simplex150", spike_simplex, join_miter, end_flat, 1532.6543, 15.0);
#endif

    test_one<polygon_type, polygon_type>("join_types", join_types, join_round, end_flat, 88.2060, 1.5);

    test_one<polygon_type, polygon_type>("chained_box", chained_box, join_round, end_flat, 83.1403, 1.0);
    test_one<polygon_type, polygon_type>("chained_box", chained_box, join_miter, end_flat, 84, 1.0);
    test_one<polygon_type, polygon_type>("L", letter_L, join_round, end_flat, 13.7314, 0.5);
    test_one<polygon_type, polygon_type>("L", letter_L, join_miter, end_flat, 14.0, 0.5);

    test_one<polygon_type, polygon_type>("chained_box", chained_box, join_miter, end_flat, 84, 1.0);
    test_one<polygon_type, polygon_type>("chained_box", chained_box, join_round, end_flat, 83.1403, 1.0);

    test_one<polygon_type, polygon_type>("indentation4", indentation, join_miter, end_flat, 25.7741, 0.4);
    test_one<polygon_type, polygon_type>("indentation4", indentation, join_round, end_flat, 25.5695, 0.4);
    test_one<polygon_type, polygon_type>("indentation5", indentation, join_miter, end_flat, 28.2426, 0.5);
    test_one<polygon_type, polygon_type>("indentation5", indentation, join_round, end_flat, 27.9953, 0.5);
    test_one<polygon_type, polygon_type>("indentation6", indentation, join_miter, end_flat, 30.6712, 0.6);

    // SQL Server gives 30.34479159164
    test_one<polygon_type, polygon_type>("indentation6", indentation, join_round, end_flat, 30.3445, 0.6);

    test_one<polygon_type, polygon_type>("indentation7", indentation, join_miter, end_flat, 33.0958, 0.7);
    test_one<polygon_type, polygon_type>("indentation7", indentation, join_round, end_flat, 32.6533, 0.7);

    test_one<polygon_type, polygon_type>("indentation8", indentation, join_miter, end_flat, 35.5943, 0.8);
    test_one<polygon_type, polygon_type>("indentation8", indentation, join_round, end_flat, 35.0164, 0.8);
    test_one<polygon_type, polygon_type>("indentation12", indentation, join_miter, end_flat, 46.3541, 1.2);
    test_one<polygon_type, polygon_type>("indentation12", indentation, join_round, end_flat, 45.0537, 1.2);

    // TODO: fix, the buffered pieces are currently counterclockwise, that should be reversed
    //test_one<polygon_type, polygon_type>("indentation4_neg", indentation, join_miter, end_flat, 6.99098413022335, -0.4);
    //test_one<polygon_type, polygon_type>("indentation4_neg", indentation, join_round, end_flat, 7.25523322189147, -0.4);
    //test_one<polygon_type, polygon_type>("indentation8_neg", indentation, join_miter, end_flat, 1.36941992048731, -0.8);
    //test_one<polygon_type, polygon_type>("indentation8_neg", indentation, join_round, end_flat, 1.37375487490664, -0.8);
    //test_one<polygon_type, polygon_type>("indentation12_neg", indentation, join_miter, end_flat, 0, -1.2);
    //test_one<polygon_type, polygon_type>("indentation12_neg", indentation, join_round, end_flat, 0, -1.2);

    test_one<polygon_type, polygon_type>("donut_simplex6", donut_simplex, join_miter, end_flat, 53.648, 0.6);
    test_one<polygon_type, polygon_type>("donut_simplex6", donut_simplex, join_round, end_flat, 52.820, 0.6);
    test_one<polygon_type, polygon_type>("donut_simplex8", donut_simplex, join_miter, end_flat, 61.132, 0.8);
    test_one<polygon_type, polygon_type>("donut_simplex8", donut_simplex, join_round, end_flat, 59.6713, 0.8);
    test_one<polygon_type, polygon_type>("donut_simplex10", donut_simplex, join_miter, end_flat, 68.670, 1.0);
    test_one<polygon_type, polygon_type>("donut_simplex10", donut_simplex, join_round, end_flat, 66.387, 1.0);
    test_one<polygon_type, polygon_type>("donut_simplex12", donut_simplex, join_miter, end_flat, 76.605, 1.2);
    test_one<polygon_type, polygon_type>("donut_simplex12", donut_simplex, join_round, end_flat, 73.3179, 1.2);
    test_one<polygon_type, polygon_type>("donut_simplex14", donut_simplex, join_miter, end_flat, 84.974, 1.4);
    test_one<polygon_type, polygon_type>("donut_simplex14", donut_simplex, join_round, end_flat, 80.500, 1.4);
    test_one<polygon_type, polygon_type>("donut_simplex16", donut_simplex, join_miter, end_flat, 93.777, 1.6);
    test_one<polygon_type, polygon_type>("donut_simplex16", donut_simplex, join_round, end_flat, 87.933, 1.6);

    test_one<polygon_type, polygon_type>("donut_diamond1", donut_diamond, join_miter, end_flat, 280.0, 1.0);
    test_one<polygon_type, polygon_type>("donut_diamond4", donut_diamond, join_miter, end_flat, 529.0, 4.0);
    test_one<polygon_type, polygon_type>("donut_diamond5", donut_diamond, join_miter, end_flat, 625.0, 5.0);
    test_one<polygon_type, polygon_type>("donut_diamond6", donut_diamond, join_miter, end_flat, 729.0, 6.0);

    test_one<polygon_type, polygon_type>("arrow4", arrow, join_miter, end_flat, 28.265, 0.4);
    test_one<polygon_type, polygon_type>("arrow4", arrow, join_round, end_flat, 27.039, 0.4);
    test_one<polygon_type, polygon_type>("arrow5", arrow, join_miter, end_flat, 31.500, 0.5);
    test_one<polygon_type, polygon_type>("arrow5", arrow, join_round, end_flat, 29.621, 0.5);
    test_one<polygon_type, polygon_type>("arrow6", arrow, join_miter, end_flat, 34.903, 0.6);
    test_one<polygon_type, polygon_type>("arrow6", arrow, join_round, end_flat, 32.268, 0.6);

    test_one<polygon_type, polygon_type>("tipped_aitch3", tipped_aitch, join_miter, end_flat, 55.36, 0.3);
    test_one<polygon_type, polygon_type>("tipped_aitch9", tipped_aitch, join_miter, end_flat, 77.44, 0.9);
    test_one<polygon_type, polygon_type>("tipped_aitch13", tipped_aitch, join_miter, end_flat, 92.16, 1.3);

    // SQL Server: 55.205415532967 76.6468846383224 90.642916957136
    test_one<polygon_type, polygon_type>("tipped_aitch3", tipped_aitch, join_round, end_flat, 55.2053, 0.3);
    test_one<polygon_type, polygon_type>("tipped_aitch9", tipped_aitch, join_round, end_flat, 76.6457, 0.9);
    test_one<polygon_type, polygon_type>("tipped_aitch13", tipped_aitch, join_round, end_flat, 90.641, 1.3);

    test_one<polygon_type, polygon_type>("snake4", snake, join_miter, end_flat, 64.44, 0.4);
    test_one<polygon_type, polygon_type>("snake5", snake, join_miter, end_flat, 72, 0.5);
    test_one<polygon_type, polygon_type>("snake6", snake, join_miter, end_flat, 75.44, 0.6);
    test_one<polygon_type, polygon_type>("snake16", snake, join_miter, end_flat, 114.24, 1.6);

    test_one<polygon_type, polygon_type>("funnelgate2", funnelgate, join_miter, end_flat, 120.982, 2);
    test_one<polygon_type, polygon_type>("funnelgate3", funnelgate, join_miter, end_flat, 13*13, 3);
    test_one<polygon_type, polygon_type>("funnelgate4", funnelgate, join_miter, end_flat, 15*15, 4);
    test_one<polygon_type, polygon_type>("gammagate1", gammagate, join_miter, end_flat, 88, 1);
    test_one<polygon_type, polygon_type>("fork_a1", fork_a, join_miter, end_flat, 88, 1);
    test_one<polygon_type, polygon_type>("fork_b1", fork_b, join_miter, end_flat, 154, 1);
    test_one<polygon_type, polygon_type>("fork_c1", fork_c, join_miter, end_flat, 152, 1);
    test_one<polygon_type, polygon_type>("triangle", triangle, join_miter, end_flat, 14.6569, 1.0);

    test_one<polygon_type, polygon_type>("gammagate2", gammagate, join_miter, end_flat, 130, 2);

    test_one<polygon_type, polygon_type>("flower1", flower, join_miter, end_flat, 67.614, 0.1);
    test_one<polygon_type, polygon_type>("flower20", flower, join_miter, end_flat, 74.894, 0.20);
    test_one<polygon_type, polygon_type>("flower25", flower, join_miter, end_flat, 78.226, 0.25);
    test_one<polygon_type, polygon_type>("flower30", flower, join_miter, end_flat, 81.492494146177947, 0.30);
    test_one<polygon_type, polygon_type>("flower35", flower, join_miter, end_flat, 84.694183819917185, 0.35);
    test_one<polygon_type, polygon_type>("flower40", flower, join_miter, end_flat, 87.8306529577, 0.40);
    test_one<polygon_type, polygon_type>("flower45", flower, join_miter, end_flat, 90.901901559536029, 0.45);
    test_one<polygon_type, polygon_type>("flower50", flower, join_miter, end_flat, 93.907929625415662, 0.50);
    test_one<polygon_type, polygon_type>("flower55", flower, join_miter, end_flat, 96.848737155342079, 0.55);
    test_one<polygon_type, polygon_type>("flower60", flower, join_miter, end_flat, 99.724324149315279, 0.60);

    test_one<polygon_type, polygon_type>("flower10", flower, join_round, end_flat, 67.486, 0.10);
    test_one<polygon_type, polygon_type>("flower20", flower, join_round, end_flat, 74.702, 0.20);
    test_one<polygon_type, polygon_type>("flower25", flower, join_round, end_flat, 78.071, 0.25);
    test_one<polygon_type, polygon_type>("flower30", flower, join_round, end_flat, 81.352, 0.30);
    test_one<polygon_type, polygon_type>("flower35", flower, join_round, end_flat, 84.547, 0.35);
    test_one<polygon_type, polygon_type>("flower40", flower, join_round, end_flat, 87.665, 0.40);
    test_one<polygon_type, polygon_type>("flower45", flower, join_round, end_flat, 90.709, 0.45);
    test_one<polygon_type, polygon_type>("flower50", flower, join_round, end_flat, 93.680, 0.50);
    test_one<polygon_type, polygon_type>("flower55", flower, join_round, end_flat, 96.580, 0.55);
    test_one<polygon_type, polygon_type>("flower60", flower, join_round, end_flat, 99.408, 0.60);

    // Saw
    {
        // SQL Server:
        // 68.6258859984014 90.2254986930165 112.799509089077 136.392823913949 161.224547934625 187.427508982734
        //215.063576036522 244.167935815974 274.764905445676 306.878264367143 340.530496138041 375.720107548269
        int const n = 12;
        double expected_round[n] =
            {
                68.6252,  90.222, 112.792, 136.397, 161.230, 187.435,
                215.073, 244.179, 274.779, 306.894, 340.543, 375.734
            };
        double expected_miter[n] =
            {
                70.7706,  98.804, 132.101, 170.661, 214.484, 263.57,
                317.92,  377.532, 442.408, 512.546, 587.948, 668.613
            };

        for (int i = 1; i <= n; i++)
        {
            std::ostringstream out;
            out << "saw_" << i;
            test_one<polygon_type, polygon_type>(out.str(), saw, join_round, end_flat, expected_round[i - 1], double(i) / 2.0, -999, true, 0.1);
            test_one<polygon_type, polygon_type>(out.str(), saw, join_miter, end_flat, expected_miter[i - 1], double(i) / 2.0);
        }
    }

    // Bowl
    {
        // SQL Server values - see query below.
        //1 43.2425133175081 60.0257800296593 78.3497997564532 98.2145746255142 119.620102487345 142.482792724034
        //2 166.499856911107 191.763334982583 218.446279387336 246.615018368511 276.300134755606 307.518458532186

        int const n = 12;
        double expected_round[n] =
            {
                43.2423,  60.025,  78.3477,  98.2109, 119.614, 142.487,
                166.505, 191.77, 218.455, 246.625, 276.312, 307.532
            };
        double expected_miter[n] =
            {
                43.4895,  61.014,  80.5726,  102.166, 125.794, 151.374,
                178.599, 207.443, 237.904, 270.000, 304.0, 340.000
            };

        for (int i = 1; i <= n; i++)
        {
            std::ostringstream out;
            out << "bowl_" << i;
            test_one<polygon_type, polygon_type>(out.str(), bowl, join_round, end_flat, expected_round[i - 1], double(i) / 2.0, -999, true, 0.1);
            test_one<polygon_type, polygon_type>(out.str(), bowl, join_miter, end_flat, expected_miter[i - 1], double(i) / 2.0);
        }
    }
    test_one<polygon_type, polygon_type>("county1", county1, join_round, end_flat, 0.00114092, 0.01);
    test_one<polygon_type, polygon_type>("county1", county1, join_miter, end_flat, 0.00132859, 0.01);

    test_one<polygon_type, polygon_type>("parcel1_10", parcel1, join_round, end_flat, 7571.39121246337891, 10.0);
    test_one<polygon_type, polygon_type>("parcel1_10", parcel1, join_miter, end_flat, 8207.45314788818359, 10.0);
    test_one<polygon_type, polygon_type>("parcel1_20", parcel1, join_round, end_flat, 11648.0537185668945, 20.0);
    test_one<polygon_type, polygon_type>("parcel1_20", parcel1, join_miter, end_flat, 14184.0223083496094, 20.0);
    test_one<polygon_type, polygon_type>("parcel1_30", parcel1, join_round, end_flat, 16350.3611068725586, 30.0);
    test_one<polygon_type, polygon_type>("parcel1_30", parcel1, join_miter, end_flat, 22417.8007659912109, 30.0);

    test_one<polygon_type, polygon_type>("parcel2_10", parcel2, join_round, end_flat, 5000.85063171386719, 10.0);
    test_one<polygon_type, polygon_type>("parcel2_10", parcel2, join_miter, end_flat, 5091.12226867675781, 10.0);
    test_one<polygon_type, polygon_type>("parcel2_20", parcel2, join_round, end_flat, 9049.60844421386719, 20.0);
    test_one<polygon_type, polygon_type>("parcel2_20", parcel2, join_miter, end_flat, 9410.69154357910156, 20.0);
    test_one<polygon_type, polygon_type>("parcel2_30", parcel2, join_round, end_flat, 13726.3790588378906, 30.0);
    test_one<polygon_type, polygon_type>("parcel2_30", parcel2, join_miter, end_flat, 14535.2319564819336, 30.0);

    test_one<polygon_type, polygon_type>("parcel3_10", parcel3, join_round, end_flat, 19992.6824035644531, 10.0);
    test_one<polygon_type, polygon_type>("parcel3_10", parcel3, join_miter, end_flat, 20024.5579376220703, 10.0);
    test_one<polygon_type, polygon_type>("parcel3_20", parcel3, join_round, end_flat, 34505.0746192932129, 20.0);
    test_one<polygon_type, polygon_type>("parcel3_20", parcel3, join_miter, end_flat, 34633.2606201171875, 20.0);
    test_one<polygon_type, polygon_type>("parcel3_30", parcel3, join_round, end_flat, 45261.4196014404297, 30.0);
    test_one<polygon_type, polygon_type>("parcel3_30", parcel3, join_miter, end_flat, 45567.3875694274902, 30.0);

    test_one<polygon_type, polygon_type>("parcel3_bend_10", parcel3_bend, join_round, end_flat, 155.6188, 5.0);
    test_one<polygon_type, polygon_type>("parcel3_bend_10", parcel3_bend, join_round, end_flat, 458.4187, 10.0);
    test_one<polygon_type, polygon_type>("parcel3_bend_10", parcel3_bend, join_round, end_flat, 917.9747, 15.0);
    test_one<polygon_type, polygon_type>("parcel3_bend_10", parcel3_bend, join_round, end_flat, 1534.4795, 20.0);


    // Negative buffers making polygons smaller
    test_one<polygon_type, polygon_type>("simplex", simplex, join_round, end_flat, 7.04043, -0.5);
    test_one<polygon_type, polygon_type>("simplex", simplex, join_miter, end_flat, 7.04043, -0.5);
    test_one<polygon_type, polygon_type>("concave_simplex", concave_simplex, join_round, end_flat, 0.777987, -0.5);
    test_one<polygon_type, polygon_type>("concave_simplex", concave_simplex, join_miter, end_flat, 0.724208, -0.5);

    test_one<polygon_type, polygon_type>("donut_simplex3", donut_simplex, join_miter, end_flat, 19.7636, -0.3);
    test_one<polygon_type, polygon_type>("donut_simplex3", donut_simplex, join_round, end_flat, 19.8861, -0.3);
    test_one<polygon_type, polygon_type>("donut_simplex6", donut_simplex, join_miter, end_flat, 12.8920, -0.6);
    test_one<polygon_type, polygon_type>("donut_simplex6", donut_simplex, join_round, end_flat, 12.9157, -0.6);
}


#ifdef HAVE_TTMATH
#include <ttmath_stub.hpp>
#endif

int test_main(int, char* [])
{
    test_all<bg::model::point<double, 2, bg::cs::cartesian> >();
    //test_all<bg::model::point<tt, 2, bg::cs::cartesian> >();
    
    return 0;
}
