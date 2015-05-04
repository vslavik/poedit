// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.

// This file was modified by Oracle on 2013, 2014.
// Modifications copyright (c) 2013-2014 Oracle and/or its affiliates.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

#ifndef BOOST_GEOMETRY_TEST_RELATE_HPP
#define BOOST_GEOMETRY_TEST_RELATE_HPP

#include <geometry_test_common.hpp>

#include <boost/geometry/core/ring_type.hpp>
#include <boost/geometry/algorithms/detail/relate/relate.hpp>
#include <boost/geometry/strategies/strategies.hpp>
#include <boost/geometry/geometries/geometries.hpp>
#include <boost/geometry/geometries/point_xy.hpp>

#include <boost/geometry/io/wkt/read.hpp>

#include <boost/geometry/strategies/cartesian/point_in_box.hpp>
#include <boost/geometry/strategies/cartesian/box_in_box.hpp>
#include <boost/geometry/strategies/agnostic/point_in_box_by_side.hpp>

namespace bgdr = bg::detail::relate;

std::string transposed(std::string matrix)
{
    if ( !matrix.empty() )
    {
        std::swap(matrix[1], matrix[3]);
        std::swap(matrix[2], matrix[6]);
        std::swap(matrix[5], matrix[7]);
    }
    return matrix;
}

bool matrix_compare(std::string const& m1, std::string const& m2)
{
    BOOST_ASSERT(m1.size() == 9 && m2.size() == 9);
    for ( size_t i = 0 ; i < 9 ; ++i )
    {
        if ( m1[i] == '*' || m2[i] == '*' )
            continue;

        if ( m1[i] != m2[i] )
            return false;
    }
    return true;
}

bool matrix_compare(std::string const& m, std::string const& res1, std::string const& res2)
{
    return matrix_compare(m, res1)
        || ( !res2.empty() ? matrix_compare(m, res2) : false );
}

std::string matrix_format(std::string const& matrix1, std::string const& matrix2)
{
    return matrix1
         + ( !matrix2.empty() ? " || " : "" ) + matrix2;
}

template <typename Geometry1, typename Geometry2>
void check_geometry(Geometry1 const& geometry1,
                    Geometry2 const& geometry2,
                    std::string const& wkt1,
                    std::string const& wkt2,
                    std::string const& expected1,
                    std::string const& expected2 = std::string())
{
    {
        std::string res_str = bgdr::relate<bgdr::matrix9>(geometry1, geometry2);
        bool ok = matrix_compare(res_str, expected1, expected2);
        BOOST_CHECK_MESSAGE(ok,
            "relate: " << wkt1
            << " and " << wkt2
            << " -> Expected: " << matrix_format(expected1, expected2)
            << " detected: " << res_str);
    }

    // changed sequence of geometries - transposed result
    {
        std::string res_str = bgdr::relate(geometry2, geometry1, bgdr::matrix9());
        std::string expected1_tr = transposed(expected1);
        std::string expected2_tr = transposed(expected2);
        bool ok = matrix_compare(res_str, expected1_tr, expected2_tr);
        BOOST_CHECK_MESSAGE(ok,
            "relate: " << wkt2
            << " and " << wkt1
            << " -> Expected: " << matrix_format(expected1_tr, expected2_tr)
            << " detected: " << res_str);
    }

    if ( expected2.empty() )
    {
        {
            bool result = bgdr::relate(geometry1, geometry2, bgdr::mask9(expected1));
            // TODO: SHOULD BE !interrupted - CHECK THIS!
            BOOST_CHECK_MESSAGE(result, 
                "relate: " << wkt1
                << " and " << wkt2
                << " -> Expected: " << expected1);
        }

        if ( BOOST_GEOMETRY_CONDITION((
                bg::detail::relate::interruption_enabled<Geometry1, Geometry2>::value )) )
        {
            // brake the expected output
            std::string expected_interrupt = expected1;
            bool changed = false;
            BOOST_FOREACH(char & c, expected_interrupt)
            {
                if ( c >= '0' && c <= '9' )
                {
                    if ( c == '0' )
                        c = 'F';
                    else
                        --c;

                    changed = true;
                }
            }

            if ( changed )
            {
                bool result = bgdr::relate(geometry1, geometry2, bgdr::mask9(expected_interrupt));
                // TODO: SHOULD BE interrupted - CHECK THIS!
                BOOST_CHECK_MESSAGE(!result,
                    "relate: " << wkt1
                    << " and " << wkt2
                    << " -> Expected interrupt for:" << expected_interrupt);
            }
        }
    }
}

template <typename Geometry1, typename Geometry2>
void test_geometry(std::string const& wkt1,
                   std::string const& wkt2,
                   std::string const& expected1,
                   std::string const& expected2 = std::string())
{
    Geometry1 geometry1;
    Geometry2 geometry2;
    bg::read_wkt(wkt1, geometry1);
    bg::read_wkt(wkt2, geometry2);
    check_geometry(geometry1, geometry2, wkt1, wkt2, expected1, expected2);
}

#endif // BOOST_GEOMETRY_TEST_RELATE_HPP
