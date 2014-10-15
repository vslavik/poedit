// Boost.Geometry (aka GGL, Generic Geometry Library)
// Unit Test

// Copyright (c) 2014, Oracle and/or its affiliates.

// Contributed and/or modified by Menelaos Karavelas, on behalf of Oracle

// Licensed under the Boost Software License version 1.0.
// http://www.boost.org/users/license.html

#ifndef BOOST_GEOMETRY_TEST_IS_VALID_HPP
#define BOOST_GEOMETRY_TEST_IS_VALID_HPP

#include <iostream>
#include <sstream>
#include <string>

#include <boost/core/ignore_unused.hpp>
#include <boost/range.hpp>
#include <boost/variant/variant.hpp>

#include <boost/geometry/core/closure.hpp>
#include <boost/geometry/core/exterior_ring.hpp>
#include <boost/geometry/core/interior_rings.hpp>
#include <boost/geometry/core/point_order.hpp>
#include <boost/geometry/core/ring_type.hpp>
#include <boost/geometry/core/tag.hpp>
#include <boost/geometry/core/tags.hpp>

#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/geometries/segment.hpp>
#include <boost/geometry/geometries/box.hpp>
#include <boost/geometry/geometries/linestring.hpp>
#include <boost/geometry/geometries/ring.hpp>
#include <boost/geometry/geometries/polygon.hpp>
#include <boost/geometry/geometries/multi_point.hpp>
#include <boost/geometry/geometries/multi_linestring.hpp>
#include <boost/geometry/geometries/multi_polygon.hpp>

#include <boost/geometry/strategies/strategies.hpp>

#include <boost/geometry/io/wkt/wkt.hpp>

#include <boost/geometry/algorithms/convert.hpp>
#include <boost/geometry/algorithms/num_points.hpp>
#include <boost/geometry/algorithms/is_valid.hpp>

#include <boost/geometry/algorithms/detail/check_iterator_range.hpp>

#ifdef BOOST_GEOMETRY_TEST_DEBUG
#include "pretty_print_geometry.hpp"
#endif


namespace bg = ::boost::geometry;

typedef bg::model::point<double, 2, bg::cs::cartesian> point_type;
typedef bg::model::segment<point_type>                 segment_type;
typedef bg::model::box<point_type>                     box_type;
typedef bg::model::linestring<point_type>              linestring_type;
typedef bg::model::multi_linestring<linestring_type>   multi_linestring_type;
typedef bg::model::multi_point<point_type>             multi_point_type;


//----------------------------------------------------------------------------


// returns true if a geometry can be converted to closed
template
<
    typename Geometry,
    typename Tag = typename bg::tag<Geometry>::type,
    bg::closure_selector Closure = bg::closure<Geometry>::value
>
struct is_convertible_to_closed
{
    static inline bool apply(Geometry const&)
    {
        return false;
    }
};

template <typename Ring>
struct is_convertible_to_closed<Ring, bg::ring_tag, bg::open>
{
    static inline bool apply(Ring const& ring)
    {
        return boost::size(ring) > 0;
    }
};

template <typename Polygon>
struct is_convertible_to_closed<Polygon, bg::polygon_tag, bg::open>
{
    typedef typename bg::ring_type<Polygon>::type ring_type;

    template <typename InteriorRings>
    static inline
    bool apply_to_interior_rings(InteriorRings const& interior_rings)
    {
        return bg::detail::check_iterator_range
            <
                is_convertible_to_closed<ring_type>
            >::apply(boost::begin(interior_rings),
                     boost::end(interior_rings));
    }

    static inline bool apply(Polygon const& polygon)
    {
        return boost::size(bg::exterior_ring(polygon)) > 0
            && apply_to_interior_rings(bg::interior_rings(polygon));
    }
};

template <typename MultiPolygon>
struct is_convertible_to_closed<MultiPolygon, bg::multi_polygon_tag, bg::open>
{
    typedef typename boost::range_value<MultiPolygon>::type polygon;

    static inline bool apply(MultiPolygon const& multi_polygon)
    {
        return bg::detail::check_iterator_range
            <
                is_convertible_to_closed<polygon>,
                false // do not allow empty multi-polygon
            >::apply(boost::begin(multi_polygon),
                     boost::end(multi_polygon));
    }
};


//----------------------------------------------------------------------------


// returns true if a geometry can be converted to cw
template
<
    typename Geometry,
    typename Tag = typename bg::tag<Geometry>::type,
    bg::order_selector Order = bg::point_order<Geometry>::value
>
struct is_convertible_to_cw
{
    static inline bool apply(Geometry const&)
    {
        return bg::point_order<Geometry>::value == bg::counterclockwise;
    }
};


//----------------------------------------------------------------------------


// returns true if geometry can be converted to polygon
template
<
    typename Geometry,
    typename Tag = typename bg::tag<Geometry>::type
>
struct is_convertible_to_polygon
{
    typedef Geometry type;
    static bool const value = false;
};

template <typename Ring>
struct is_convertible_to_polygon<Ring, bg::ring_tag>
{
    typedef bg::model::polygon
        <
            typename bg::point_type<Ring>::type,
            bg::point_order<Ring>::value == bg::clockwise,
            bg::closure<Ring>::value == bg::closed
        > type;

    static bool const value = true;
};


//----------------------------------------------------------------------------


// returns true if geometry can be converted to multi-polygon
template
<
    typename Geometry,
    typename Tag = typename bg::tag<Geometry>::type
>
struct is_convertible_to_multipolygon
{
    typedef Geometry type;
    static bool const value = false;
};

template <typename Ring>
struct is_convertible_to_multipolygon<Ring, bg::ring_tag>
{
    typedef bg::model::multi_polygon
        <
            typename is_convertible_to_polygon<Ring>::type
        > type;

    static bool const value = true;
};

template <typename Polygon>
struct is_convertible_to_multipolygon<Polygon, bg::polygon_tag>
{
    typedef bg::model::multi_polygon<Polygon> type;
    static bool const value = true;
};


//----------------------------------------------------------------------------


template <typename ValidityTester>
struct validity_checker
{
    template <typename Geometry>
    static inline bool apply(Geometry const& geometry,
                             bool expected_result)
    {
        bool valid = ValidityTester::apply(geometry);
        BOOST_CHECK_MESSAGE( valid == expected_result,
            "Expected: " << expected_result
            << " detected: " << valid
            << " wkt: " << bg::wkt(geometry) );

        return valid;
    }
};


//----------------------------------------------------------------------------


struct default_validity_tester
{
    template <typename Geometry>    
    static inline bool apply(Geometry const& geometry)
    {
        return bg::is_valid(geometry);
    }
};


template <bool AllowSpikes>
struct validity_tester_linear
{
    template <typename Geometry>
    static inline bool apply(Geometry const& geometry)
    {
        return bg::dispatch::is_valid
            <
                Geometry,
                typename bg::tag<Geometry>::type,
                AllowSpikes
            >::apply(geometry);
    }
};


template <bool AllowDuplicates>
struct validity_tester_areal
{
    template <typename Geometry>
    static inline bool apply(Geometry const& geometry)
    {
        bool const irrelevant = true;

        return bg::dispatch::is_valid
            <
                Geometry,
                typename bg::tag<Geometry>::type,
                irrelevant,
                AllowDuplicates
            >::apply(geometry);
    }
};


//----------------------------------------------------------------------------


template
<
    typename ValidityTester,
    typename Geometry,
    typename ClosedGeometry = Geometry,
    typename CWGeometry = Geometry,
    typename CWClosedGeometry = Geometry,
    typename Tag = typename bg::tag<Geometry>::type
>
struct test_valid
{
    template <typename G>
    static inline void base_test(G const& g, bool expected_result)
    {
#ifdef BOOST_GEOMETRY_TEST_DEBUG
        std::cout << "=======" << std::endl;
#endif

        bool valid = validity_checker
            <
                ValidityTester
            >::apply(g, expected_result);
        boost::ignore_unused(valid);

#ifdef BOOST_GEOMETRY_TEST_DEBUG
        std::cout << "Geometry: ";
        pretty_print_geometry<G>::apply(std::cout, g);
        std::cout << std::endl;
        std::cout << "wkt: " << bg::wkt(g) << std::endl;
        std::cout << std::boolalpha;
        std::cout << "is valid? " << valid << std::endl;
        std::cout << "expected result: " << expected_result << std::endl;
        std::cout << "=======" << std::endl;
        std::cout << std::noboolalpha;
#endif
    }

    static inline void apply(Geometry const& geometry, bool expected_result)
    {
        base_test(geometry, expected_result);

        if ( is_convertible_to_closed<Geometry>::apply(geometry) )
        {
#ifdef BOOST_GEOMETRY_TEST_DEBUG
            std::cout << "...checking closed geometry..."
                      << std::endl;
#endif
            ClosedGeometry closed_geometry;
            bg::convert(geometry, closed_geometry);
            base_test(closed_geometry, expected_result);
        }
        if ( is_convertible_to_cw<Geometry>::apply(geometry) )
        {
#ifdef BOOST_GEOMETRY_TEST_DEBUG
            std::cout << "...checking cw open geometry..."
                      << std::endl;
#endif            
            CWGeometry cw_geometry;
            bg::convert(geometry, cw_geometry);
            base_test(cw_geometry, expected_result);
            if ( is_convertible_to_closed<CWGeometry>::apply(cw_geometry) )
            {
#ifdef BOOST_GEOMETRY_TEST_DEBUG
                std::cout << "...checking cw closed geometry..."
                          << std::endl;
#endif            
                CWClosedGeometry cw_closed_geometry;
                bg::convert(cw_geometry, cw_closed_geometry);
                base_test(cw_closed_geometry, expected_result);
            }
        }

        if ( is_convertible_to_polygon<Geometry>::value )
        {
#ifdef BOOST_GEOMETRY_TEST_DEBUG
            std::cout << "...checking geometry converted to polygon..."
                      << std::endl;
#endif            
            typename is_convertible_to_polygon<Geometry>::type polygon;
            bg::convert(geometry, polygon);
            base_test(polygon, expected_result);
        }

        if ( is_convertible_to_multipolygon<Geometry>::value )
        {
#ifdef BOOST_GEOMETRY_TEST_DEBUG
            std::cout << "...checking geometry converted to multi-polygon..."
                      << std::endl;
#endif            
            typename is_convertible_to_multipolygon
                <
                    Geometry
                >::type multipolygon;

            bg::convert(geometry, multipolygon);
            base_test(multipolygon, expected_result);
        }

#ifdef BOOST_GEOMETRY_TEST_DEBUG
        std::cout << std::endl << std::endl << std::endl;
#endif
    }
};


//----------------------------------------------------------------------------


template <typename VariantGeometry>
struct test_valid_variant
{
    static inline void apply(VariantGeometry const& vg, bool expected_result)
    {
        test_valid
            <
                default_validity_tester, VariantGeometry
            >::base_test(vg, expected_result);
    }
};


#endif // BOOST_GEOMETRY_TEST_IS_VALID_HPP
