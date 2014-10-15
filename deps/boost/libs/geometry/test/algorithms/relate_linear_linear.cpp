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
void test_linestring_linestring()
{
    typedef bg::model::linestring<P> ls;

    test_geometry<ls, ls>("LINESTRING(0 0, 2 2, 3 2)", "LINESTRING(0 0, 2 2, 3 2)", "1FFF0FFF2");
    test_geometry<ls, ls>("LINESTRING(0 0,3 2)", "LINESTRING(0 0, 2 2, 3 2)", "FF1F0F1F2");
    test_geometry<ls, ls>("LINESTRING(1 0,2 2,2 3)", "LINESTRING(0 0, 2 2, 3 2)", "0F1FF0102");

    test_geometry<ls, ls>("LINESTRING(0 0,5 0)", "LINESTRING(0 0,1 1,2 0,2 -1)", "0F1F00102");
    
    test_geometry<ls, ls>("LINESTRING(0 0, 1 1, 2 2, 3 2)", "LINESTRING(0 0, 2 2, 3 2)", "1FFF0FFF2");
    test_geometry<ls, ls>("LINESTRING(3 2, 2 2, 1 1, 0 0)", "LINESTRING(0 0, 2 2, 3 2)", "1FFF0FFF2");
    test_geometry<ls, ls>("LINESTRING(0 0, 1 1, 2 2, 3 2)", "LINESTRING(3 2, 2 2, 0 0)", "1FFF0FFF2");
    test_geometry<ls, ls>("LINESTRING(3 2, 2 2, 1 1, 0 0)", "LINESTRING(3 2, 2 2, 0 0)", "1FFF0FFF2");

    test_geometry<ls, ls>("LINESTRING(3 1, 2 2, 1 1, 0 0)", "LINESTRING(0 0, 2 2, 3 2)", "1F1F00102");
    test_geometry<ls, ls>("LINESTRING(3 3, 2 2, 1 1, 0 0)", "LINESTRING(0 0, 2 2, 3 2)", "1F1F00102");

    test_geometry<ls, ls>("LINESTRING(0 0, 1 1, 2 2, 2 3)", "LINESTRING(0 0, 2 2, 2 3)", "1FFF0FFF2");
    test_geometry<ls, ls>("LINESTRING(2 3, 2 2, 1 1, 0 0)", "LINESTRING(0 0, 2 2, 2 3)", "1FFF0FFF2");
    test_geometry<ls, ls>("LINESTRING(0 0, 1 1, 2 2, 2 3)", "LINESTRING(2 3, 2 2, 0 0)", "1FFF0FFF2");
    test_geometry<ls, ls>("LINESTRING(2 3, 2 2, 1 1, 0 0)", "LINESTRING(2 3, 2 2, 0 0)", "1FFF0FFF2");

    test_geometry<ls, ls>("LINESTRING(1 1, 2 2, 3 2)", "LINESTRING(0 0, 2 2, 4 2)", "1FF0FF102");

    test_geometry<ls, ls>("LINESTRING(3 2, 2 2, 1 1)", "LINESTRING(0 0, 2 2, 4 2)", "1FF0FF102");
    test_geometry<ls, ls>("LINESTRING(1 1, 2 2, 3 2)", "LINESTRING(4 2, 2 2, 0 0)", "1FF0FF102");
    test_geometry<ls, ls>("LINESTRING(3 2, 2 2, 1 1)", "LINESTRING(4 2, 2 2, 0 0)", "1FF0FF102");

//    test_geometry<ls, ls>("LINESTRING(1 1, 2 2, 2 2)", "LINESTRING(0 0, 2 2, 4 2)", true);

//    test_geometry<ls, ls>("LINESTRING(1 1, 2 2, 3 3)", "LINESTRING(0 0, 2 2, 4 2)", false);
//    test_geometry<ls, ls>("LINESTRING(1 1, 2 2, 3 2, 3 3)", "LINESTRING(0 0, 2 2, 4 2)", false);
//    test_geometry<ls, ls>("LINESTRING(1 1, 2 2, 3 1)", "LINESTRING(0 0, 2 2, 4 2)", false);
//    test_geometry<ls, ls>("LINESTRING(1 1, 2 2, 3 2, 3 1)", "LINESTRING(0 0, 2 2, 4 2)", false);

//    test_geometry<ls, ls>("LINESTRING(0 1, 1 1, 2 2, 3 2)", "LINESTRING(0 0, 2 2, 4 2)", false);
//    test_geometry<ls, ls>("LINESTRING(0 1, 0 0, 2 2, 3 2)", "LINESTRING(0 0, 2 2, 4 2)", false);
//    test_geometry<ls, ls>("LINESTRING(1 0, 1 1, 2 2, 3 2)", "LINESTRING(0 0, 2 2, 4 2)", false);
//    test_geometry<ls, ls>("LINESTRING(1 0, 0 0, 2 2, 3 2)", "LINESTRING(0 0, 2 2, 4 2)", false);

//    test_geometry<ls, ls>("LINESTRING(0 0)", "LINESTRING(0 0)", false);
//    test_geometry<ls, ls>("LINESTRING(1 1)", "LINESTRING(0 0, 2 2)", true);
//    test_geometry<ls, ls>("LINESTRING(0 0)", "LINESTRING(0 0, 2 2)", false);
//    test_geometry<ls, ls>("LINESTRING(0 0, 1 1)", "LINESTRING(0 0)", false);

//    test_geometry<ls, ls>("LINESTRING(0 0,5 0,3 0,6 0)", "LINESTRING(0 0,6 0)", true);
//    test_geometry<ls, ls>("LINESTRING(0 0,2 2,3 3,1 1)", "LINESTRING(0 0,3 3,6 3)", true);

    // SPIKES!
    test_geometry<ls, ls>("LINESTRING(0 0,2 2,3 3,1 1,5 3)", "LINESTRING(0 0,3 3,6 3)", "1F100F102");
    test_geometry<ls, ls>("LINESTRING(5 3,1 1,3 3,2 2,0 0)", "LINESTRING(0 0,3 3,6 3)", "1F100F102");
    test_geometry<ls, ls>("LINESTRING(0 0,2 2,3 3,1 1,5 3)", "LINESTRING(6 3,3 3,0 0)", "1F100F102");
    test_geometry<ls, ls>("LINESTRING(5 3,1 1,3 3,2 2,0 0)", "LINESTRING(6 3,3 3,0 0)", "1F100F102");

    test_geometry<ls, ls>("LINESTRING(6 3,3 3,0 0)", "LINESTRING(0 0,2 2,3 3,1 1,5 3)", "101F001F2");

    test_geometry<ls, ls>("LINESTRING(0 0,10 0)", "LINESTRING(1 0,9 0,2 0)", "101FF0FF2");
    test_geometry<ls, ls>("LINESTRING(0 0,2 2,3 3,1 1)", "LINESTRING(0 0,3 3,6 3)", "1FF00F102");
    // TODO: REWRITE MATRICES
    // BEGIN
    /*test_geometry<ls, ls>("LINESTRING(0 0,2 2,3 3,1 1)", "LINESTRING(0 0,4 4,6 3)", "1FF00F102");

    test_geometry<ls, ls>("LINESTRING(0 0,2 0,1 0)", "LINESTRING(0 1,0 0,2 0)", "1FF00F102");
    test_geometry<ls, ls>("LINESTRING(2 0,0 0,1 0)", "LINESTRING(0 1,0 0,2 0)", "1FF00F102");

    test_geometry<ls, ls>("LINESTRING(0 0,3 3,1 1)", "LINESTRING(3 0,3 3,3 1)", "0F1FF0102");
    test_geometry<ls, ls>("LINESTRING(0 0,3 3,1 1)", "LINESTRING(2 0,2 3,2 1)", "0F1FF0102");
    test_geometry<ls, ls>("LINESTRING(0 0,3 3,1 1)", "LINESTRING(2 0,2 2,2 1)", "0F1FF0102");
 
     test_geometry<ls, ls>("LINESTRING(0 0,2 2,3 3,4 4)", "LINESTRING(0 0,1 1,4 4)", "1FFF0FFF2");*/
    // END

    test_geometry<ls, ls>("LINESTRING(0 0,2 2,3 3,4 4)", "LINESTRING(0 0,1 1,4 4)", "1FFF0FFF2");

    // loop i/i i/i u/u u/u
    test_geometry<ls, ls>("LINESTRING(0 0,10 0)",
                          "LINESTRING(1 1,1 0,6 0,6 1,4 1,4 0,9 0,9 1)", "1F1FF0102");

    // self-intersecting and self-touching equal
    test_geometry<ls, ls>("LINESTRING(0 5,5 5,10 5,10 10,5 10,5 5,5 0)",
                          "LINESTRING(0 5,5 5,5 10,10 10,10 5,5 5,5 0)", "1FFF0FFF2");
    // self-intersecting loop and self-touching equal
    test_geometry<ls, ls>("LINESTRING(0 5,5 5,10 5,10 10,5 10,5 5,10 5,10 10,5 10,5 5,5 0)",
                          "LINESTRING(0 5,5 5,5 10,10 10,10 5,5 5,5 0)", "1FFF0FFF2");

    test_geometry<ls, ls>("LINESTRING(0 0,1 1)", "LINESTRING(0 1,1 0)", "0F1FF0102");
    test_geometry<ls, ls>("LINESTRING(0 0,1 1)", "LINESTRING(1 1,2 0)", "FF1F00102");
    test_geometry<ls, ls>("LINESTRING(0 0,1 1)", "LINESTRING(2 0,1 1)", "FF1F00102");

    test_geometry<ls, ls>("LINESTRING(0 0,1 0,2 1,3 5,4 0)", "LINESTRING(1 0,2 1,3 5)", "101FF0FF2");
    test_geometry<ls, ls>("LINESTRING(0 0,1 0,2 1,3 5,4 0)", "LINESTRING(3 5,2 1,1 0)", "101FF0FF2");
    test_geometry<ls, ls>("LINESTRING(1 0,2 1,3 5)", "LINESTRING(4 0,3 5,2 1,1 0,0 0)", "1FF0FF102");
    test_geometry<ls, ls>("LINESTRING(3 5,2 1,1 0)", "LINESTRING(4 0,3 5,2 1,1 0,0 0)", "1FF0FF102");
    
    test_geometry<ls, ls>("LINESTRING(0 0,10 0)", "LINESTRING(-1 -1,1 0,10 0,20 -1)", "1F10F0102");
    test_geometry<ls, ls>("LINESTRING(0 0,10 0)", "LINESTRING(20 -1,10 0,1 0,-1 -1)", "1F10F0102");

    test_geometry<ls, ls>("LINESTRING(-1 1,0 0,1 0,5 0,5 5,10 5,15 0,31 0)",
                          "LINESTRING(-1 -1,0 0,1 0,2 0,3 1,4 0,30 0)",
                          "101FF0102");
    test_geometry<ls, ls>("LINESTRING(-1 1,0 0,1 0,5 0,5 5,10 5,15 0,31 0)",
                          "LINESTRING(30 0,4 0,3 1,2 0,1 0,0 0,-1 -1)",
                          "101FF0102");
    test_geometry<ls, ls>("LINESTRING(31 0,15 0,10 5,5 5,5 0,1 0,0 0,-1 1)",
                          "LINESTRING(-1 -1,0 0,1 0,2 0,3 1,4 0,30 0)",
                          "101FF0102");
    test_geometry<ls, ls>("LINESTRING(31 0,15 0,10 5,5 5,5 0,1 0,0 0,-1 1)",
                          "LINESTRING(30 0,4 0,3 1,2 0,1 0,0 0,-1 -1)",
                          "101FF0102");

    // self-IP
    test_geometry<ls, ls>("LINESTRING(1 0,9 0)",
                          "LINESTRING(0 0,10 0,10 10,5 0,0 10)",
                          "1FF0FF102");
    test_geometry<ls, ls>("LINESTRING(1 0,5 0,9 0)",
                          "LINESTRING(0 0,10 0,10 10,5 0,0 10)",
                          "1FF0FF102");
    test_geometry<ls, ls>("LINESTRING(1 0,9 0)",
                          "LINESTRING(0 0,10 0,10 10,5 10,5 -1)",
                          "1FF0FF102");
    test_geometry<ls, ls>("LINESTRING(1 0,9 0)",
                          "LINESTRING(0 0,10 0,5 0,5 5)",
                          "1FF0FF102");
    test_geometry<ls, ls>("LINESTRING(1 0,7 0)", "LINESTRING(0 0,10 0,10 10,4 -1)",
                          "1FF0FF102");
    test_geometry<ls, ls>("LINESTRING(1 0,5 0,7 0)", "LINESTRING(0 0,10 0,10 10,4 -1)",
                          "1FF0FF102");
    test_geometry<ls, ls>("LINESTRING(1 0,7 0,8 1)", "LINESTRING(0 0,10 0,10 10,4 -1)",
                          "1F10F0102");
    test_geometry<ls, ls>("LINESTRING(1 0,5 0,7 0,8 1)", "LINESTRING(0 0,10 0,10 10,4 -1)",
                          "1F10F0102");

    // self-IP going out and in on the same point
    test_geometry<ls, ls>("LINESTRING(2 0,5 0,5 5,6 5,5 0,8 0)", "LINESTRING(1 0,9 0)",
                          "1F10FF102");

    // duplicated points
    test_geometry<ls, ls>("LINESTRING(1 1, 2 2, 2 2)", "LINESTRING(0 0, 2 2, 4 2)", "1FF0FF102");
    test_geometry<ls, ls>("LINESTRING(1 1, 1 1, 2 2)", "LINESTRING(0 0, 2 2, 4 2)", "1FF0FF102");

    // linear ring
    test_geometry<ls, ls>("LINESTRING(0 0,10 0)", "LINESTRING(5 0,9 0,5 5,1 0,5 0)", "1F1FF01F2");
    test_geometry<ls, ls>("LINESTRING(0 0,5 0,10 0)", "LINESTRING(5 0,9 0,5 5,1 0,5 0)", "1F1FF01F2");
    test_geometry<ls, ls>("LINESTRING(0 0,5 0,10 0)", "LINESTRING(5 0,10 0,5 5,1 0,5 0)", "1F10F01F2");

    test_geometry<ls, ls>("LINESTRING(0 0,5 0)", "LINESTRING(5 0,10 0,5 5,0 0,5 0)", "1FF0FF1F2");
    test_geometry<ls, ls>("LINESTRING(0 0,5 0)", "LINESTRING(5 0,10 0,5 5,5 0)", "FF10F01F2");

    test_geometry<ls, ls>("LINESTRING(1 0,1 6)", "LINESTRING(0 0,5 0,5 5,0 5)", "0F10F0102");

    // point-size Linestring
    test_geometry<ls, ls>("LINESTRING(1 0,1 0)", "LINESTRING(0 0,5 0)", "0FFFFF102");
    test_geometry<ls, ls>("LINESTRING(1 0,1 0)", "LINESTRING(1 0,5 0)", "F0FFFF102");
    test_geometry<ls, ls>("LINESTRING(1 0,1 0)", "LINESTRING(0 0,1 0)", "F0FFFF102");
    test_geometry<ls, ls>("LINESTRING(1 0,1 0)", "LINESTRING(1 0,1 0)", "0FFFFFFF2");
    test_geometry<ls, ls>("LINESTRING(1 0,1 0)", "LINESTRING(0 0,0 0)", "FF0FFF0F2");

    //to_svg<ls, ls>("LINESTRING(0 0,5 0)", "LINESTRING(5 0,10 0,5 5,5 0)", "test_relate_00.svg");

    // INVALID LINESTRINGS
    // 1-point LS (a Point) NOT disjoint
    //test_geometry<ls, ls>("LINESTRING(1 0)", "LINESTRING(0 0,5 0)", "0FFFFF102");
    //test_geometry<ls, ls>("LINESTRING(0 0,5 0)", "LINESTRING(1 0)", "0F1FF0FF2");
    //test_geometry<ls, ls>("LINESTRING(0 0,5 0)", "LINESTRING(1 0,1 0,1 0)", "0F1FF0FF2");
    // Point/Point
    //test_geometry<ls, ls>("LINESTRING(0 0)", "LINESTRING(0 0)", "0FFFFFFF2");

    // OTHER MASKS
    {
        namespace bgdr = bg::detail::relate;
        ls ls1, ls2, ls3, ls4;
        bg::read_wkt("LINESTRING(0 0,2 0)", ls1);
        bg::read_wkt("LINESTRING(2 0,4 0)", ls2);
        bg::read_wkt("LINESTRING(1 0,1 1)", ls3);
        bg::read_wkt("LINESTRING(1 0,4 0)", ls4);
        BOOST_CHECK(bgdr::relate(ls1, ls2, bgdr::mask9("FT*******")
                                        || bgdr::mask9("F**T*****")
                                        || bgdr::mask9("F***T****")));
        BOOST_CHECK(bgdr::relate(ls1, ls3, bgdr::mask9("FT*******")
                                        || bgdr::mask9("F**T*****")
                                        || bgdr::mask9("F***T****")));
        BOOST_CHECK(bgdr::relate(ls3, ls1, bgdr::mask9("FT*******")
                                        || bgdr::mask9("F**T*****")
                                        || bgdr::mask9("F***T****")));
        BOOST_CHECK(bgdr::relate(ls2, ls4, bgdr::mask9("T*F**F***"))); // within
    }
}

template <typename P>
void test_linestring_multi_linestring()
{
    typedef bg::model::linestring<P> ls;
    typedef bg::model::multi_linestring<ls> mls;

    // LS disjoint
    test_geometry<ls, mls>("LINESTRING(0 0,10 0)", "MULTILINESTRING((1 0,2 0),(1 1,2 1))", "101FF0102");
    // linear ring disjoint
    test_geometry<ls, mls>("LINESTRING(0 0,10 0)", "MULTILINESTRING((1 0,2 0),(1 1,2 1,2 2,1 1))", "101FF01F2");
    // 2xLS forming non-simple linear ring disjoint
    test_geometry<ls, mls>("LINESTRING(0 0,10 0)", "MULTILINESTRING((1 0,2 0),(1 1,2 1,2 2),(1 1,2 2))", "101FF01F2");

    test_geometry<ls, mls>("LINESTRING(0 0,10 0)",
                           "MULTILINESTRING((1 0,9 0),(9 0,2 0))",
                           "101FF0FF2");

    // rings
    test_geometry<ls, mls>("LINESTRING(0 0,5 0,5 5,0 5,0 0)",
                           "MULTILINESTRING((5 5,0 5,0 0),(0 0,5 0,5 5))",
                           "1FFFFFFF2");
    test_geometry<ls, mls>("LINESTRING(0 0,5 0,5 5,0 5,0 0)",
                           "MULTILINESTRING((5 5,5 0,0 0),(0 0,0 5,5 5))",
                           "1FFFFFFF2");
    // overlapping rings
    test_geometry<ls, mls>("LINESTRING(0 0,5 0,5 5,0 5,0 0)",
                           "MULTILINESTRING((5 5,0 5,0 0),(0 0,5 0,5 5,0 5))",
                           "10FFFFFF2");
    test_geometry<ls, mls>("LINESTRING(0 0,5 0,5 5,0 5,0 0)",
                           "MULTILINESTRING((5 5,5 0,0 0),(0 0,0 5,5 5,5 0))",
                           "10FFFFFF2");

    // INVALID LINESTRINGS
    // 1-point LS (a Point) disjoint
    //test_geometry<ls, mls>("LINESTRING(0 0,10 0)", "MULTILINESTRING((1 0,2 0),(1 1))", "101FF00F2");
    //test_geometry<ls, mls>("LINESTRING(0 0,10 0)", "MULTILINESTRING((1 0,2 0),(1 1,1 1))", "101FF00F2");
    //test_geometry<ls, mls>("LINESTRING(0 0,10 0)", "MULTILINESTRING((1 0,2 0),(1 1,1 1,1 1))", "101FF00F2");
    // 1-point LS (a Point) NOT disjoint
    //test_geometry<ls, mls>("LINESTRING(0 0,10 0)", "MULTILINESTRING((1 0,9 0),(2 0))", "101FF0FF2");
    //test_geometry<ls, mls>("LINESTRING(0 0,10 0)", "MULTILINESTRING((1 0,9 0),(2 0,2 0))", "101FF0FF2");
    //test_geometry<ls, mls>("LINESTRING(0 0,10 0)", "MULTILINESTRING((1 0,9 0),(2 0,2 0,2 0))", "101FF0FF2");

    // point-like
    test_geometry<ls, mls>("LINESTRING(0 0, 5 0)",                      // |--------------|
                           "MULTILINESTRING((0 0, 1 0),(2 0, 2 0))",    // |------|  *
                           "101F00FF2");
    test_geometry<ls, mls>("LINESTRING(0 0, 5 0)",                      // |--------------|
                           "MULTILINESTRING((0 0, 1 0),(1 0, 1 0))",    // |------*
                           "101F00FF2");
    test_geometry<ls, mls>("LINESTRING(0 0, 5 0)",                      // |--------------|
                           "MULTILINESTRING((5 0, 1 0),(1 0, 1 0))",    //        *-------|
                           "101F00FF2");
    test_geometry<ls, mls>("LINESTRING(0 0, 5 0)",                      // |--------------|
                           "MULTILINESTRING((0 0, 1 0),(5 0, 5 0))",    // |------|       *
                           "10100FFF2");
    test_geometry<ls, mls>("LINESTRING(0 0, 5 0)",                      // |--------------|
                           "MULTILINESTRING((0 0, 1 0),(0 0, 0 0))",    // *------|
                           "101000FF2");
    test_geometry<ls, mls>("LINESTRING(0 0, 5 0)",                      // |--------------|
                           "MULTILINESTRING((4 0, 5 0),(5 0, 5 0))",    //         |------*
                           "101000FF2");
    test_geometry<ls, mls>("LINESTRING(0 0, 5 0)",                      // |--------------|
                           "MULTILINESTRING((1 0, 2 0),(0 0, 0 0))",    // *  |------|
                           "1010F0FF2");

    test_geometry<ls, mls>("LINESTRING(0 0, 5 0)",                      //   |--------------|
                           "MULTILINESTRING((2 0, 2 0),(2 0, 2 2))",    //            *
                           "001FF0102");                                //            |

    // for consistency
    test_geometry<ls, mls>("LINESTRING(0 0, 5 0)",                      // |--------------|
                           "MULTILINESTRING((0 0, 5 0),(0 0, 2 0))",    // |--------------|
                           "10F00FFF2");                                // |------|

    test_geometry<ls, mls>("LINESTRING(0 0, 5 0)",                      // |--------------|
                           "MULTILINESTRING((0 0, 5 0),(3 0, 5 0))",    // |--------------|
                           "10F00FFF2");                                //         |------|

    test_geometry<ls, mls>("LINESTRING(0 0, 5 0)",                      // |--------------|
                           "MULTILINESTRING((0 0, 5 0),(0 0, 6 0))",    // |--------------|
                           "1FF00F102");                                // |----------------|

    test_geometry<ls, mls>("LINESTRING(0 0, 5 0)",                      //   |--------------|
                           "MULTILINESTRING((0 0, 5 0),(-1 0, 5 0))",   //   |--------------|
                           "1FF00F102");                                // |----------------|

    test_geometry<ls, mls>("LINESTRING(0 0, 5 0)",                      //   |--------------|
                           "MULTILINESTRING((0 0, 5 0),(-1 0, 6 0))",   //   |--------------|
                           "1FF00F102");                                // |------------------|

    test_geometry<ls, mls>("LINESTRING(0 0, 5 0)",                      //   |--------------|
                           "MULTILINESTRING((0 0, 5 0),(-1 0, 2 0))",   //   |--------------|
                           "10F00F102");                                // |-------|

    test_geometry<ls, mls>("LINESTRING(0 0, 5 0)",                      //   |--------------|
                           "MULTILINESTRING((0 0, 5 0),(2 0, 6 0))",    //   |--------------|
                           "10F00F102");                                //            |-------|

    test_geometry<ls, mls>("LINESTRING(0 0, 5 0)",                      //   |--------------|
                           "MULTILINESTRING((0 0, 5 0),(2 0, 2 2))",    //   |--------------|
                           "10FF0F102");                                //            |
                                                                        //            |
}

template <typename P>
void test_multi_linestring_multi_linestring()
{
    typedef bg::model::linestring<P> ls;
    typedef bg::model::multi_linestring<ls> mls;

    test_geometry<mls, mls>("MULTILINESTRING((0 0,0 0,18 0,18 0,19 0,19 0,19 0,30 0,30 0))",
                            "MULTILINESTRING((0 10,5 0,20 0,20 0,30 0))",
                            "1F1F00102");
    test_geometry<mls, mls>("MULTILINESTRING((0 0,0 0,18 0,18 0,19 0,19 0,19 0,30 0,30 0))",
                            //"MULTILINESTRING((0 10,5 0,20 0,20 0,30 0),(1 10,1 10,1 0,1 0,1 -10),(2 0,2 0),(3 0,3 0,3 0),(0 0,0 0,0 10,0 10),(30 0,30 0,31 0,31 0))",
                            "MULTILINESTRING((0 10,5 0,20 0,20 0,30 0),(1 10,1 10,1 0,1 0,1 -10),(0 0,0 0,0 10,0 10),(30 0,30 0,31 0,31 0))",
                            "1F100F102");
    test_geometry<mls, mls>("MULTILINESTRING((0 0,0 0,18 0,18 0,19 0,19 0,19 0,30 0,30 0))",
                            "MULTILINESTRING((0 10,5 0,20 0,20 0,30 0),(0 0,0 0,0 10,0 10))",
                            "1F1F0F1F2");

    // point-like
    test_geometry<mls, mls>("MULTILINESTRING((0 0, 0 0),(1 1, 1 1))",
                            "MULTILINESTRING((0 0, 0 0))",
                            "0F0FFFFF2");
    test_geometry<mls, mls>("MULTILINESTRING((0 0, 0 0),(1 1, 1 1))",
                            "MULTILINESTRING((0 0, 0 0),(1 1, 1 1))",
                            "0FFFFFFF2");
    test_geometry<mls, mls>("MULTILINESTRING((0 0, 0 0),(1 1, 1 1))",
                            "MULTILINESTRING((2 2, 2 2),(3 3, 3 3))",
                            "FF0FFF0F2");

    test_geometry<mls, mls>("MULTILINESTRING((0 5,10 5,10 10,5 10),(5 10,5 0,5 2),(5 2,5 5,0 5))",
                            "MULTILINESTRING((5 5,0 5),(5 5,5 0),(10 10,10 5,5 5,5 10,10 10))",
                            "10FFFFFF2");
}

template <typename P>
void test_all()
{
    test_linestring_linestring<P>();
    test_linestring_multi_linestring<P>();
    test_multi_linestring_multi_linestring<P>();
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
