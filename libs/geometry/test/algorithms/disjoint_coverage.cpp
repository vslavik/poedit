// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2014, Oracle and/or its affiliates.

// Licensed under the Boost Software License version 1.0.
// http://www.boost.org/users/license.html

// Contributed and/or modified by Menelaos Karavelas, on behalf of Oracle

#ifndef BOOST_TEST_MODULE
#define BOOST_TEST_MODULE test_disjoint_coverage
#endif

// unit test to test disjoint for all geometry combinations

#include <iostream>

#include <boost/test/included/unit_test.hpp>

#include <boost/geometry/core/tag.hpp>
#include <boost/geometry/core/tags.hpp>
#include <boost/geometry/multi/core/tags.hpp>

#include <boost/geometry/strategies/strategies.hpp>

#include <boost/geometry/io/wkt/read.hpp>
#include <boost/geometry/io/wkt/write.hpp>
#include <boost/geometry/multi/io/wkt/read.hpp>
#include <boost/geometry/multi/io/wkt/write.hpp>
#include <boost/geometry/io/dsv/write.hpp>

#include <boost/geometry/geometries/point.hpp>
#include <boost/geometry/geometries/segment.hpp>
#include <boost/geometry/geometries/linestring.hpp>
#include <boost/geometry/geometries/ring.hpp>
#include <boost/geometry/geometries/polygon.hpp>

#include <boost/geometry/multi/geometries/multi_point.hpp>
#include <boost/geometry/multi/geometries/multi_linestring.hpp>
#include <boost/geometry/multi/geometries/multi_polygon.hpp>

#include <boost/geometry/algorithms/disjoint.hpp>

#include "from_wkt.hpp"


#ifdef HAVE_TTMATH
#include <boost/geometry/extensions/contrib/ttmath_stub.hpp>
#endif

namespace bg = ::boost::geometry;

//============================================================================

template <typename Geometry, typename Tag = typename bg::tag<Geometry>::type>
struct pretty_print_geometry
{
    static inline std::ostream& apply(Geometry const& geometry)
    {
        std::cout << bg::wkt(geometry);
        return std::cout;
    }
};

template <typename Segment>
struct pretty_print_geometry<Segment, bg::segment_tag>
{
    static inline std::ostream& apply(Segment const& segment)
    {
        std::cout << "SEGMENT" << bg::dsv(segment);
        return std::cout;
    }
};

template <typename Ring>
struct pretty_print_geometry<Ring, bg::ring_tag>
{
    static inline std::ostream& apply(Ring const& ring)
    {
        std::cout << "RING" << bg::dsv(ring);
        return std::cout;
    }
};

template <typename Box>
struct pretty_print_geometry<Box, bg::box_tag>
{
    static inline std::ostream& apply(Box const& box)
    {
        std::cout << "BOX" << bg::dsv(box);
        return std::cout;
    }
};

//============================================================================

struct test_disjoint
{
    template <typename Geometry1, typename Geometry2>
    static inline void apply(Geometry1 const& geometry1,
                             Geometry2 const& geometry2,
                             bool expected_result)
    {
        bool result = bg::disjoint(geometry1, geometry2);
        BOOST_CHECK( result == expected_result );

        result = bg::disjoint(geometry2, geometry1);
        BOOST_CHECK( result == expected_result );

#ifdef BOOST_GEOMETRY_TEST_DEBUG
        std::cout << "G1 - G2: ";
        pretty_print_geometry<Geometry1>::apply(geometry1) << " - ";
        pretty_print_geometry<Geometry2>::apply(geometry2) << std::endl;
        std::cout << std::boolalpha;
        std::cout << "expected/computed result: "
                  << expected_result << " / " << result << std::endl;
        std::cout << std::endl;
        std::cout << std::noboolalpha;
#endif
    }
};

//============================================================================

// pointlike-pointlike geometries
template <typename P>
inline void test_point_point()
{
    typedef test_disjoint tester;

    tester::apply(from_wkt<P>("POINT(0 0)"),
                  from_wkt<P>("POINT(0 0)"),
                  false);

    tester::apply(from_wkt<P>("POINT(0 0)"),
                  from_wkt<P>("POINT(1 1)"),
                  true);
}

template <typename P>
inline void test_point_multipoint()
{
    typedef bg::model::multi_point<P> MP;
    
    typedef test_disjoint tester;

    tester::apply(from_wkt<P>("POINT(0 0)"),
                  from_wkt<MP>("MULTIPOINT(0 0,1 1)"),
                  false);

    tester::apply(from_wkt<P>("POINT(0 0)"),
                  from_wkt<MP>("MULTIPOINT(1 1,2 2)"),
                  true);
}

template <typename P>
inline void test_multipoint_multipoint()
{
    typedef bg::model::multi_point<P> MP;
    
    typedef test_disjoint tester;

    tester::apply(from_wkt<MP>("MULTIPOINT(0 0,1 0)"),
                  from_wkt<MP>("MULTIPOINT(0 0,1 1)"),
                  false);

    tester::apply(from_wkt<MP>("MULTIPOINT(0 0,1 0)"),
                  from_wkt<MP>("MULTIPOINT(1 1,2 2)"),
                  true);
}

//============================================================================

// pointlike-linear geometries
template <typename P>
inline void test_point_segment()
{
    typedef test_disjoint tester;
    typedef bg::model::segment<P> S;

    tester::apply(from_wkt<P>("POINT(0 0)"),
                  from_wkt<S>("SEGMENT(0 0,2 0)"),
                  false);

    tester::apply(from_wkt<P>("POINT(1 0)"),
                  from_wkt<S>("SEGMENT(0 0,2 0)"),
                  false);

    tester::apply(from_wkt<P>("POINT(1 1)"),
                  from_wkt<S>("SEGMENT(0 0,2 0)"),
                  true);
}

template <typename P>
inline void test_point_linestring()
{
    typedef bg::model::linestring<P> L;
    
    typedef test_disjoint tester;

    tester::apply(from_wkt<P>("POINT(0 0)"),
                  from_wkt<L>("LINESTRING(0 0,2 2,4 4)"),
                  false);

    tester::apply(from_wkt<P>("POINT(1 1)"),
                  from_wkt<L>("LINESTRING(0 0,2 2,4 4)"),
                  false);

    tester::apply(from_wkt<P>("POINT(3 3)"),
                  from_wkt<L>("LINESTRING(0 0,2 2,4 4)"),
                  false);

    tester::apply(from_wkt<P>("POINT(1 0)"),
                  from_wkt<L>("LINESTRING(0 0,2 2,4 4)"),
                  true);
}

template <typename P>
inline void test_point_multilinestring()
{
    typedef bg::model::linestring<P> L;
    typedef bg::model::multi_linestring<L> ML;
    
    typedef test_disjoint tester;

    tester::apply(from_wkt<P>("POINT(0 1)"),
                  from_wkt<ML>("MULTILINESTRING((0 0,2 2,4 4),(0 0,2 0,4 0))"),
                  true);

    tester::apply(from_wkt<P>("POINT(0 0)"),
                  from_wkt<ML>("MULTILINESTRING((0 0,2 2,4 4),(0 0,2 0,4 0))"),
                  false);

    tester::apply(from_wkt<P>("POINT(1 1)"),
                  from_wkt<ML>("MULTILINESTRING((0 0,2 2,4 4),(0 0,2 0,4 0))"),
                  false);

    tester::apply(from_wkt<P>("POINT(1 0)"),
                  from_wkt<ML>("MULTILINESTRING((0 0,2 2,4 4),(0 0,2 0,4 0))"),
                  false);
}

template <typename P>
inline void test_multipoint_segment()
{
    typedef test_disjoint tester;
    typedef bg::model::multi_point<P> MP;
    typedef bg::model::segment<P> S;

    tester::apply(from_wkt<MP>("MULTIPOINT(0 0,1 1)"),
                  from_wkt<S>("SEGMENT(0 0,2 0)"),
                  false);

    tester::apply(from_wkt<MP>("MULTIPOINT(1 0,1 1)"),
                  from_wkt<S>("SEGMENT(0 0,2 0)"),
                  false);

    tester::apply(from_wkt<MP>("MULTIPOINT(1 1,2 2)"),
                  from_wkt<S>("SEGMENT(0 0,2 0)"),
                  true);
}

template <typename P>
inline void test_multipoint_linestring()
{
    typedef bg::model::multi_point<P> MP;
    typedef bg::model::linestring<P> L;
    
    typedef test_disjoint tester;

    tester::apply(from_wkt<MP>("MULTIPOINT(0 0,1 0)"),
                  from_wkt<L>("LINESTRING(0 0,2 2,4 4)"),
                  false);

    tester::apply(from_wkt<MP>("MULTIPOINT(1 0,1 1)"),
                  from_wkt<L>("LINESTRING(0 0,2 2,4 4)"),
                  false);

    tester::apply(from_wkt<MP>("MULTIPOINT(1 0,3 3)"),
                  from_wkt<L>("LINESTRING(0 0,2 2,4 4)"),
                  false);

    tester::apply(from_wkt<MP>("MULTIPOINT(1 0,2 0)"),
                  from_wkt<L>("LINESTRING(0 0,2 2,4 4)"),
                  true);
}

template <typename P>
inline void test_multipoint_multilinestring()
{
    typedef bg::model::multi_point<P> MP;
    typedef bg::model::linestring<P> L;
    typedef bg::model::multi_linestring<L> ML;
    
    typedef test_disjoint tester;

    tester::apply(from_wkt<MP>("MULTIPOINT(0 1,0 2)"),
                  from_wkt<ML>("MULTILINESTRING((0 0,2 2,4 4),(0 0,2 0,4 0))"),
                  true);

    tester::apply(from_wkt<MP>("POINT(0 0,1 0)"),
                  from_wkt<ML>("MULTILINESTRING((0 0,2 2,4 4),(0 0,2 0,4 0))"),
                  false);

    tester::apply(from_wkt<MP>("POINT(0 1,1 1)"),
                  from_wkt<ML>("MULTILINESTRING((0 0,2 2,4 4),(0 0,2 0,4 0))"),
                  false);

    tester::apply(from_wkt<MP>("POINT(0 1,1 0)"),
                  from_wkt<ML>("MULTILINESTRING((0 0,2 2,4 4),(0 0,2 0,4 0))"),
                  false);
}

//============================================================================

// pointlike-areal geometries
template <typename P>
inline void test_point_box()
{
    typedef test_disjoint tester;
    typedef bg::model::box<P> B;

    tester::apply(from_wkt<P>("POINT(0 0)"),
                  from_wkt<B>("BOX(0 0,1 1)"),
                  false);

    tester::apply(from_wkt<P>("POINT(2 2)"),
                  from_wkt<B>("BOX(0 0,1 0)"),
                  true);
}

template <typename P>
inline void test_point_ring()
{
    typedef bg::model::ring<P, false, false> R; // ccw, open
    
    typedef test_disjoint tester;

    tester::apply(from_wkt<P>("POINT(0 0)"),
                  from_wkt<R>("POLYGON((0 0,1 0,0 1))"),
                  false);

    tester::apply(from_wkt<P>("POINT(1 1)"),
                  from_wkt<R>("POLYGON((0 0,1 0,0 1))"),
                  true);
}

template <typename P>
inline void test_point_polygon()
{
    typedef bg::model::polygon<P, false, false> PL; // ccw, open
    
    typedef test_disjoint tester;

    tester::apply(from_wkt<P>("POINT(0 0)"),
                  from_wkt<PL>("POLYGON((0 0,1 0,0 1))"),
                  false);

    tester::apply(from_wkt<P>("POINT(1 1)"),
                  from_wkt<PL>("POLYGON((0 0,1 0,0 1))"),
                  true);
}

template <typename P>
inline void test_point_multipolygon()
{
    typedef bg::model::polygon<P, false, false> PL; // ccw, open
    typedef bg::model::multi_polygon<PL> MPL;
    
    typedef test_disjoint tester;

    tester::apply(from_wkt<P>("POINT(0 0)"),
                  from_wkt<MPL>("MULTIPOLYGON(((0 0,1 0,0 1)),((2 0,3 0,2 1)))"),
                  false);

    tester::apply(from_wkt<P>("POINT(1 1)"),
                  from_wkt<MPL>("MULTIPOLYGON(((0 0,1 0,0 1)),((2 0,3 0,2 1)))"),
                  true);
}

template <typename P>
inline void test_multipoint_box()
{
    typedef test_disjoint tester;
    typedef bg::model::multi_point<P> MP;
    typedef bg::model::box<P> B;

    tester::apply(from_wkt<MP>("MULTIPOINT(0 0,1 1)"),
                  from_wkt<B>("BOX(0 0,2 2)"),
                  false);

    tester::apply(from_wkt<MP>("MULTIPOINT(1 1,3 3)"),
                  from_wkt<B>("BOX(0 0,2 2)"),
                  false);

    tester::apply(from_wkt<MP>("MULTIPOINT(3 3,4 4)"),
                  from_wkt<B>("BOX(0 0,2 2)"),
                  true);
}

template <typename P>
inline void test_multipoint_ring()
{
    typedef bg::model::multi_point<P> MP;
    typedef bg::model::ring<P, false, false> R; // ccw, open
    
    typedef test_disjoint tester;

    tester::apply(from_wkt<MP>("MULTIPOINT(0 0,1 0)"),
                  from_wkt<R>("POLYGON((0 0,1 0,0 1))"),
                  false);

    tester::apply(from_wkt<MP>("MULTIPOINT(1 0,1 1)"),
                  from_wkt<R>("POLYGON((0 0,1 0,0 1))"),
                  false);

    tester::apply(from_wkt<MP>("MULTIPOINT(1 1,2 2)"),
                  from_wkt<R>("POLYGON((0 0,1 0,0 1))"),
                  true);
}

template <typename P>
inline void test_multipoint_polygon()
{
    typedef bg::model::multi_point<P> MP;
    typedef bg::model::polygon<P, false, false> PL; // ccw, open
    
    typedef test_disjoint tester;

    tester::apply(from_wkt<MP>("MULTIPOINT(0 0,1 0)"),
                  from_wkt<PL>("POLYGON(((0 0,1 0,0 1)))"),
                  false);

    tester::apply(from_wkt<MP>("MULTIPOINT(0 0,2 0)"),
                  from_wkt<PL>("POLYGON(((0 0,1 0,0 1)))"),
                  false);

    tester::apply(from_wkt<MP>("MULTIPOINT(1 1,2 0)"),
                  from_wkt<PL>("POLYGON(((0 0,1 0,0 1)))"),
                  true);

    tester::apply(from_wkt<MP>("MULTIPOINT(1 1,2 3)"),
                  from_wkt<PL>("POLYGON(((0 0,1 0,0 1)))"),
                  true);
}

template <typename P>
inline void test_multipoint_multipolygon()
{
    typedef bg::model::multi_point<P> MP;
    typedef bg::model::polygon<P, false, false> PL; // ccw, open
    typedef bg::model::multi_polygon<PL> MPL;
    
    typedef test_disjoint tester;

    tester::apply(from_wkt<MP>("MULTIPOINT(0 0,2 0)"),
                  from_wkt<MPL>("MULTIPOLYGON((0 0,1 0,0 1)),(2 0,3 0,2 1))"),
                  false);

    tester::apply(from_wkt<MP>("MULTIPOINT(0 0,1 0)"),
                  from_wkt<MPL>("MULTIPOLYGON((0 0,1 0,0 1)),(2 0,3 0,2 1))"),
                  false);

    tester::apply(from_wkt<MP>("MULTIPOINT(1 1,2 0)"),
                  from_wkt<MPL>("MULTIPOLYGON((0 0,1 0,0 1)),(2 0,3 0,2 1))"),
                  false);

    tester::apply(from_wkt<MP>("MULTIPOINT(1 1,2 3)"),
                  from_wkt<MPL>("MULTIPOLYGON((0 0,1 0,0 1)),(2 0,3 0,2 1))"),
                  true);
}

//============================================================================

// linear-linear geometries
template <typename P>
inline void test_segment_segment()
{
    typedef bg::model::segment<P> S;
    
    typedef test_disjoint tester;

    tester::apply(from_wkt<S>("SEGMENT(0 0,2 0)"),
                  from_wkt<S>("SEGMENT(0 0,0 2)"),
                  false);

    tester::apply(from_wkt<S>("SEGMENT(0 0,2 0)"),
                  from_wkt<S>("SEGMENT(2 0,3 0)"),
                  false);

    tester::apply(from_wkt<S>("SEGMENT(0 0,2 0)"),
                  from_wkt<S>("SEGMENT(1 0,3 0)"),
                  false);

    tester::apply(from_wkt<S>("SEGMENT(0 0,2 0)"),
                  from_wkt<S>("SEGMENT(1 0,1 1)"),
                  false);

    tester::apply(from_wkt<S>("SEGMENT(0 0,2 0)"),
                  from_wkt<S>("SEGMENT(1 1,2 2)"),
                  true);
}

template <typename P>
inline void test_linestring_segment()
{
    typedef bg::model::segment<P> S;
    typedef bg::model::linestring<P> L;
    
    typedef test_disjoint tester;

    tester::apply(from_wkt<S>("SEGMENT(0 0,2 0)"),
                  from_wkt<L>("LINESTRING(0 0,0 2)"),
                  false);

    tester::apply(from_wkt<S>("SEGMENT(0 0,2 0)"),
                  from_wkt<L>("LINESTRING(2 0,3 0)"),
                  false);

    tester::apply(from_wkt<S>("SEGMENT(0 0,2 0)"),
                  from_wkt<L>("LINESTRING(1 0,3 0)"),
                  false);

    tester::apply(from_wkt<S>("SEGMENT(0 0,2 0)"),
                  from_wkt<L>("LINESTRING(1 0,1 1)"),
                  false);

    tester::apply(from_wkt<S>("SEGMENT(0 0,2 0)"),
                  from_wkt<L>("LINESTRING(1 1,2 2)"),
                  true);
}

template <typename P>
inline void test_multilinestring_segment()
{
    typedef bg::model::segment<P> S;
    typedef bg::model::linestring<P> L;
    typedef bg::model::multi_linestring<L> ML;
    
    typedef test_disjoint tester;

    tester::apply(from_wkt<S>("SEGMENT(0 0,2 0)"),
                  from_wkt<ML>("MULTILINESTRING((0 0,0 2))"),
                  false);

    tester::apply(from_wkt<S>("SEGMENT(0 0,2 0)"),
                  from_wkt<ML>("MULTILINESTRING((2 0,3 0))"),
                  false);

    tester::apply(from_wkt<S>("SEGMENT(0 0,2 0)"),
                  from_wkt<ML>("MULTILINESTRING((1 0,3 0))"),
                  false);

    tester::apply(from_wkt<S>("SEGMENT(0 0,2 0)"),
                  from_wkt<ML>("MULTILINESTRING((1 0,1 1))"),
                  false);

    tester::apply(from_wkt<S>("SEGMENT(0 0,2 0)"),
                  from_wkt<ML>("MULTILINESTRING((1 1,2 2))"),
                  true);
}

template <typename P>
inline void test_linestring_linestring()
{
    typedef bg::model::linestring<P> L;
    
    typedef test_disjoint tester;

    tester::apply(from_wkt<L>("LINESTRING(0 0,2 0)"),
                  from_wkt<L>("LINESTRING(0 0,0 2)"),
                  false);

    tester::apply(from_wkt<L>("LINESTRING(0 0,2 0)"),
                  from_wkt<L>("LINESTRING(2 0,3 0)"),
                  false);

    tester::apply(from_wkt<L>("LINESTRING(0 0,2 0)"),
                  from_wkt<L>("LINESTRING(1 0,3 0)"),
                  false);

    tester::apply(from_wkt<L>("LINESTRING(0 0,2 0)"),
                  from_wkt<L>("LINESTRING(1 0,1 1)"),
                  false);

    tester::apply(from_wkt<L>("LINESTRING(0 0,2 0)"),
                  from_wkt<L>("LINESTRING(1 1,2 2)"),
                  true);
}

template <typename P>
inline void test_linestring_multilinestring()
{
    typedef bg::model::linestring<P> L;
    typedef bg::model::multi_linestring<L> ML;
    
    typedef test_disjoint tester;

    tester::apply(from_wkt<L>("LINESTRING(0 0,2 0)"),
                  from_wkt<ML>("MULTILINESTRING((0 0,0 2))"),
                  false);

    tester::apply(from_wkt<L>("LINESTRING(0 0,2 0)"),
                  from_wkt<ML>("MULTILINESTRING((2 0,3 0))"),
                  false);

    tester::apply(from_wkt<L>("LINESTRING(0 0,2 0)"),
                  from_wkt<ML>("MULTILINESTRING((1 0,3 0))"),
                  false);

    tester::apply(from_wkt<L>("LINESTRING(0 0,2 0)"),
                  from_wkt<ML>("MULTILINESTRING((1 0,1 1))"),
                  false);

    tester::apply(from_wkt<L>("LINESTRING(0 0,2 0)"),
                  from_wkt<ML>("MULTILINESTRING((1 1,2 2))"),
                  true);
}

template <typename P>
inline void test_multilinestring_multilinestring()
{
    typedef bg::model::linestring<P> L;
    typedef bg::model::multi_linestring<L> ML;
    
    typedef test_disjoint tester;

    tester::apply(from_wkt<ML>("MULTILINESTRING((0 0,2 0))"),
                  from_wkt<ML>("MULTILINESTRING((0 0,0 2))"),
                  false);

    tester::apply(from_wkt<ML>("MULTILINESTRING((0 0,2 0))"),
                  from_wkt<ML>("MULTILINESTRING((2 0,3 0))"),
                  false);

    tester::apply(from_wkt<ML>("MULTILINESTRING((0 0,2 0))"),
                  from_wkt<ML>("MULTILINESTRING((1 0,3 0))"),
                  false);

    tester::apply(from_wkt<ML>("MULTILINESTRING((0 0,2 0))"),
                  from_wkt<ML>("MULTILINESTRING((1 0,1 1))"),
                  false);

    tester::apply(from_wkt<ML>("MULTILINESTRING((0 0,2 0))"),
                  from_wkt<ML>("MULTILINESTRING((1 1,2 2))"),
                  true);
}

//============================================================================

// linear-areal geometries
template <typename P>
inline void test_segment_box()
{
    typedef bg::model::segment<P> S;
    typedef bg::model::box<P> B;
    
    typedef test_disjoint tester;

    tester::apply(from_wkt<S>("SEGMENT(0 0,2 0)"),
                  from_wkt<B>("BOX(0 0,2 2)"),
                  false);

    tester::apply(from_wkt<S>("SEGMENT(1 1,3 3)"),
                  from_wkt<B>("BOX(0 0,2 2)"),
                  false);

    tester::apply(from_wkt<S>("SEGMENT(2 2,3 3)"),
                  from_wkt<B>("BOX(0 0,2 2)"),
                  false);

    tester::apply(from_wkt<S>("SEGMENT(4 4,3 3)"),
                  from_wkt<B>("BOX(0 0,2 2)"),
                  true);

    tester::apply(from_wkt<S>("SEGMENT(0 4,4 4)"),
                  from_wkt<B>("BOX(0 0,2 2)"),
                  true);

    tester::apply(from_wkt<S>("SEGMENT(4 0,4 4)"),
                  from_wkt<B>("BOX(0 0,2 2)"),
                  true);

    tester::apply(from_wkt<S>("SEGMENT(0 -2,0 -1)"),
                  from_wkt<B>("BOX(0 0,1 1)"),
                  true);

    tester::apply(from_wkt<S>("SEGMENT(-2 -2,-2 -1)"),
                  from_wkt<B>("BOX(0 0,1 1)"),
                  true);

    tester::apply(from_wkt<S>("SEGMENT(-2 -2,-2 -2)"),
                  from_wkt<B>("BOX(0 0,1 1)"),
                  true);

    tester::apply(from_wkt<S>("SEGMENT(-2 0,-2 0)"),
                  from_wkt<B>("BOX(0 0,1 1)"),
                  true);

    tester::apply(from_wkt<S>("SEGMENT(0 -2,0 -2)"),
                  from_wkt<B>("BOX(0 0,1 1)"),
                  true);

    tester::apply(from_wkt<S>("SEGMENT(-2 0,-1 0)"),
                  from_wkt<B>("BOX(0 0,1 1)"),
                  true);

    // segment degenerates to a point
    tester::apply(from_wkt<S>("SEGMENT(0 0,0 0)"),
                  from_wkt<B>("BOX(0 0,1 1)"),
                  false);

    tester::apply(from_wkt<S>("SEGMENT(1 1,1 1)"),
                  from_wkt<B>("BOX(0 0,2 2)"),
                  false);

    tester::apply(from_wkt<S>("SEGMENT(2 2,2 2)"),
                  from_wkt<B>("BOX(0 0,2 2)"),
                  false);

    tester::apply(from_wkt<S>("SEGMENT(2 0,2 0)"),
                  from_wkt<B>("BOX(0 0,2 2)"),
                  false);

    tester::apply(from_wkt<S>("SEGMENT(0 2,0 2)"),
                  from_wkt<B>("BOX(0 0,2 2)"),
                  false);

    tester::apply(from_wkt<S>("SEGMENT(2 2,2 2)"),
                  from_wkt<B>("BOX(0 0,1 1)"),
                  true);
}

template <typename P>
inline void test_segment_ring()
{
    typedef bg::model::segment<P> S;
    typedef bg::model::ring<P, false, false> R; // ccw, open
    
    typedef test_disjoint tester;

    tester::apply(from_wkt<S>("SEGMENT(0 0,2 0)"),
                  from_wkt<R>("POLYGON((0 0,2 0,0 2))"),
                  false);

    tester::apply(from_wkt<S>("SEGMENT(1 0,3 3)"),
                  from_wkt<R>("POLYGON((0 0,2 0,0 2))"),
                  false);

    tester::apply(from_wkt<S>("SEGMENT(1 1,3 3)"),
                  from_wkt<R>("POLYGON((0 0,2 0,0 2))"),
                  false);

    tester::apply(from_wkt<S>("SEGMENT(2 2,3 3)"),
                  from_wkt<R>("POLYGON((0 0,2 0,0 2))"),
                  true);
}

template <typename P>
inline void test_segment_polygon()
{
    typedef bg::model::segment<P> S;
    typedef bg::model::polygon<P, false, false> PL; // ccw, open
    
    typedef test_disjoint tester;

    tester::apply(from_wkt<S>("SEGMENT(0 0,2 0)"),
                  from_wkt<PL>("POLYGON((0 0,2 0,0 2))"),
                  false);

    tester::apply(from_wkt<S>("SEGMENT(1 0,3 3)"),
                  from_wkt<PL>("POLYGON((0 0,2 0,0 2))"),
                  false);

    tester::apply(from_wkt<S>("SEGMENT(1 1,3 3)"),
                  from_wkt<PL>("POLYGON((0 0,2 0,0 2))"),
                  false);

    tester::apply(from_wkt<S>("SEGMENT(2 2,3 3)"),
                  from_wkt<PL>("POLYGON((0 0,2 0,0 2))"),
                  true);
}

template <typename P>
inline void test_segment_multipolygon()
{
    typedef bg::model::segment<P> S;
    typedef bg::model::polygon<P, false, false> PL; // ccw, open
    typedef bg::model::multi_polygon<PL> MPL;
    
    typedef test_disjoint tester;

    tester::apply(from_wkt<S>("SEGMENT(0 0,2 0)"),
                  from_wkt<MPL>("MULTIPOLYGON(((0 0,2 0,0 2)))"),
                  false);

    tester::apply(from_wkt<S>("SEGMENT(1 0,3 3)"),
                  from_wkt<MPL>("MULTIPOLYGON(((0 0,2 0,0 2)))"),
                  false);

    tester::apply(from_wkt<S>("SEGMENT(1 1,3 3)"),
                  from_wkt<MPL>("MULTIPOLYGON(((0 0,2 0,0 2)))"),
                  false);

    tester::apply(from_wkt<S>("SEGMENT(2 2,3 3)"),
                  from_wkt<MPL>("MULTIPOLYGON(((0 0,2 0,0 2)))"),
                  true);
}

template <typename P>
inline void test_linestring_box()
{
    typedef bg::model::linestring<P> L;
    typedef bg::model::box<P> B;
    
    typedef test_disjoint tester;

    tester::apply(from_wkt<L>("LINESTRING(0 0,2 0)"),
                  from_wkt<B>("BOX(0 0,2 2)"),
                  false);

    tester::apply(from_wkt<L>("LINESTRING(1 1,3 3)"),
                  from_wkt<B>("BOX(0 0,2 2)"),
                  false);

    tester::apply(from_wkt<L>("LINESTRING(2 2,3 3)"),
                  from_wkt<B>("BOX(0 0,2 2)"),
                  false);

    tester::apply(from_wkt<L>("LINESTRING(4 4,3 3)"),
                  from_wkt<B>("BOX(0 0,2 2)"),
                  true);
}

template <typename P>
inline void test_linestring_ring()
{
    typedef bg::model::linestring<P> L;
    typedef bg::model::ring<P, false, false> R; // ccw, open
    
    typedef test_disjoint tester;

    tester::apply(from_wkt<L>("LINESTRING(0 0,2 0)"),
                  from_wkt<R>("POLYGON((0 0,2 0,0 2))"),
                  false);

    tester::apply(from_wkt<L>("LINESTRING(1 0,3 3)"),
                  from_wkt<R>("POLYGON((0 0,2 0,0 2))"),
                  false);

    tester::apply(from_wkt<L>("LINESTRING(1 1,3 3)"),
                  from_wkt<R>("POLYGON((0 0,2 0,0 2))"),
                  false);

    tester::apply(from_wkt<L>("LINESTRING(2 2,3 3)"),
                  from_wkt<R>("POLYGON((0 0,2 0,0 2))"),
                  true);
}

template <typename P>
inline void test_linestring_polygon()
{
    typedef bg::model::linestring<P> L;
    typedef bg::model::polygon<P, false, false> PL; // ccw, open
    
    typedef test_disjoint tester;

    tester::apply(from_wkt<L>("LINESTRING(0 0,2 0)"),
                  from_wkt<PL>("POLYGON((0 0,2 0,0 2))"),
                  false);

    tester::apply(from_wkt<L>("LINESTRING(1 0,3 3)"),
                  from_wkt<PL>("POLYGON((0 0,2 0,0 2))"),
                  false);

    tester::apply(from_wkt<L>("LINESTRING(1 1,3 3)"),
                  from_wkt<PL>("POLYGON((0 0,2 0,0 2))"),
                  false);

    tester::apply(from_wkt<L>("LINESTRING(2 2,3 3)"),
                  from_wkt<PL>("POLYGON((0 0,2 0,0 2))"),
                  true);
}

template <typename P>
inline void test_linestring_multipolygon()
{
    typedef bg::model::linestring<P> L;
    typedef bg::model::polygon<P, false, false> PL; // ccw, open
    typedef bg::model::multi_polygon<PL> MPL;

    typedef test_disjoint tester;

    tester::apply(from_wkt<L>("LINESTRING(0 0,2 0)"),
                  from_wkt<MPL>("MULTIPOLYGON(((0 0,2 0,0 2)))"),
                  false);

    tester::apply(from_wkt<L>("LINESTRING(1 0,3 3)"),
                  from_wkt<MPL>("MULTIPOLYGON(((0 0,2 0,0 2)))"),
                  false);

    tester::apply(from_wkt<L>("LINESTRING(1 1,3 3)"),
                  from_wkt<MPL>("MULTIPOLYGON(((0 0,2 0,0 2)))"),
                  false);

    tester::apply(from_wkt<L>("LINESTRING(2 2,3 3)"),
                  from_wkt<MPL>("MULTIPOLYGON(((0 0,2 0,0 2)))"),
                  true);
}

template <typename P>
inline void test_multilinestring_box()
{
    typedef bg::model::linestring<P> L;
    typedef bg::model::multi_linestring<L> ML;
    typedef bg::model::box<P> B;

    typedef test_disjoint tester;

    tester::apply(from_wkt<ML>("MULTILINESTRING((0 0,2 0))"),
                  from_wkt<B>("BOX(0 0,2 2)"),
                  false);

    tester::apply(from_wkt<ML>("MULTILINESTRING((1 1,3 3))"),
                  from_wkt<B>("BOX(0 0,2 2)"),
                  false);

    tester::apply(from_wkt<ML>("MULTILINESTRING((2 2,3 3))"),
                  from_wkt<B>("BOX(0 0,2 2)"),
                  false);

    tester::apply(from_wkt<ML>("MULTILINESTRING((4 4,3 3))"),
                  from_wkt<B>("BOX(0 0,2 2)"),
                  true);
}

template <typename P>
inline void test_multilinestring_ring()
{
    typedef bg::model::linestring<P> L;
    typedef bg::model::multi_linestring<L> ML;
    typedef bg::model::ring<P, false, false> R; // ccw, open

    typedef test_disjoint tester;

    tester::apply(from_wkt<ML>("MULTILINESTRING((0 0,2 0))"),
                  from_wkt<R>("POLYGON((0 0,2 0,0 2))"),
                  false);

    tester::apply(from_wkt<ML>("MULTILINESTRING((1 0,3 3))"),
                  from_wkt<R>("POLYGON((0 0,2 0,0 2))"),
                  false);

    tester::apply(from_wkt<ML>("MULTILINESTRING((1 1,3 3))"),
                  from_wkt<R>("POLYGON((0 0,2 0,0 2))"),
                  false);

    tester::apply(from_wkt<ML>("MULTILINESTRING((2 2,3 3))"),
                  from_wkt<R>("POLYGON((0 0,2 0,0 2))"),
                  true);
}

template <typename P>
inline void test_multilinestring_polygon()
{
    typedef bg::model::linestring<P> L;
    typedef bg::model::multi_linestring<L> ML;
    typedef bg::model::polygon<P, false, false> PL; // ccw, open

    typedef test_disjoint tester;

    tester::apply(from_wkt<ML>("MULTILINESTRING((0 0,2 0))"),
                  from_wkt<PL>("POLYGON((0 0,2 0,0 2))"),
                  false);

    tester::apply(from_wkt<ML>("MULTILINESTRING((1 0,3 3))"),
                  from_wkt<PL>("POLYGON((0 0,2 0,0 2))"),
                  false);

    tester::apply(from_wkt<ML>("MULTILINESTRING((1 1,3 3))"),
                  from_wkt<PL>("POLYGON((0 0,2 0,0 2))"),
                  false);

    tester::apply(from_wkt<ML>("MULTILINESTRING((2 2,3 3))"),
                  from_wkt<PL>("POLYGON((0 0,2 0,0 2))"),
                  true);
}

template <typename P>
inline void test_multilinestring_multipolygon()
{
    typedef bg::model::linestring<P> L;
    typedef bg::model::multi_linestring<L> ML;
    typedef bg::model::polygon<P, false, false> PL; // ccw, open
    typedef bg::model::multi_polygon<PL> MPL;

    typedef test_disjoint tester;

    tester::apply(from_wkt<ML>("MULTILINESTRING((0 0,2 0))"),
                  from_wkt<MPL>("MULTIPOLYGON(((0 0,2 0,0 2)))"),
                  false);

    tester::apply(from_wkt<ML>("MULTILINESTRING((1 0,3 3))"),
                  from_wkt<MPL>("MULTIPOLYGON(((0 0,2 0,0 2)))"),
                  false);

    tester::apply(from_wkt<ML>("MULTILINESTRING((1 1,3 3))"),
                  from_wkt<MPL>("MULTIPOLYGON(((0 0,2 0,0 2)))"),
                  false);

    tester::apply(from_wkt<ML>("MULTILINESTRING((2 2,3 3))"),
                  from_wkt<MPL>("MULTIPOLYGON(((0 0,2 0,0 2)))"),
                  true);
}

//============================================================================

// areal-areal geometries
template <typename P>
inline void test_box_box()
{
    typedef bg::model::box<P> B;

    typedef test_disjoint tester;

    tester::apply(from_wkt<B>("BOX(2 2,3 3)"),
                  from_wkt<B>("BOX(0 0,2 2)"),
                  false);

    tester::apply(from_wkt<B>("BOX(1 1,3 3)"),
                  from_wkt<B>("BOX(0 0,2 2)"),
                  false);

    tester::apply(from_wkt<B>("BOX(3 3,4 4)"),
                  from_wkt<B>("BOX(0 0,2 2)"),
                  true);
}

template <typename P>
inline void test_ring_box()
{
    typedef bg::model::box<P> B;
    typedef bg::model::ring<P, false, false> R; // ccw, open

    typedef test_disjoint tester;

    tester::apply(from_wkt<B>("BOX(2 2,3 3)"),
                  from_wkt<R>("POLYGON((0 0,2 0,2 2,0 2))"),
                  false);

    tester::apply(from_wkt<B>("BOX(1 1,3 3)"),
                  from_wkt<R>("POLYGON((0 0,2 0,2 2,0 2))"),
                  false);

    tester::apply(from_wkt<B>("BOX(3 3,4 4)"),
                  from_wkt<R>("POLYGON((0 0,2 0,2 2,0 2))"),
                  true);
}

template <typename P>
inline void test_polygon_box()
{
    typedef bg::model::box<P> B;
    typedef bg::model::polygon<P, false, false> PL; // ccw, open

    typedef test_disjoint tester;

    tester::apply(from_wkt<B>("BOX(2 2,3 3)"),
                  from_wkt<PL>("POLYGON((0 0,2 0,2 2,0 2))"),
                  false);

    tester::apply(from_wkt<B>("BOX(1 1,3 3)"),
                  from_wkt<PL>("POLYGON((0 0,2 0,2 2,0 2))"),
                  false);

    tester::apply(from_wkt<B>("BOX(3 3,4 4)"),
                  from_wkt<PL>("POLYGON((0 0,2 0,2 2,0 2))"),
                  true);
}

template <typename P>
inline void test_multipolygon_box()
{
    typedef bg::model::box<P> B;
    typedef bg::model::polygon<P, false, false> PL; // ccw, open
    typedef bg::model::multi_polygon<PL> MPL;
    
    typedef test_disjoint tester;

    tester::apply(from_wkt<B>("BOX(2 2,3 3)"),
                  from_wkt<MPL>("MULTIPOLYGON(((0 0,2 0,2 2,0 2)))"),
                  false);

    tester::apply(from_wkt<B>("BOX(1 1,3 3)"),
                  from_wkt<MPL>("MULTIPOLYGON(((0 0,2 0,2 2,0 2)))"),
                  false);

    tester::apply(from_wkt<B>("BOX(3 3,4 4)"),
                  from_wkt<MPL>("MULTIPOLYGON(((0 0,2 0,2 2,0 2)))"),
                  true);
}

template <typename P>
inline void test_ring_ring()
{
    typedef bg::model::ring<P, false, false> R; // ccw, open

    typedef test_disjoint tester;

    tester::apply(from_wkt<R>("POLYGON((2 2,2 3,3 3,3 2))"),
                  from_wkt<R>("POLYGON((0 0,2 0,2 2,0 2))"),
                  false);

    tester::apply(from_wkt<R>("POLYGON((1 1,1 3,3 3,3 1))"),
                  from_wkt<R>("POLYGON((0 0,2 0,2 2,0 2))"),
                  false);

    tester::apply(from_wkt<R>("POLYGON((3 3,3 4,4 4,4 3))"),
                  from_wkt<R>("POLYGON((0 0,2 0,2 2,0 2))"),
                  true);
}

template <typename P>
inline void test_polygon_ring()
{
    typedef bg::model::ring<P, false, false> R; // ccw, open
    typedef bg::model::polygon<P, false, false> PL; // ccw, open

    typedef test_disjoint tester;

    tester::apply(from_wkt<R>("POLYGON((2 2,2 3,3 3,3 2))"),
                  from_wkt<PL>("POLYGON((0 0,2 0,2 2,0 2))"),
                  false);

    tester::apply(from_wkt<R>("POLYGON((1 1,1 3,3 3,3 1))"),
                  from_wkt<PL>("POLYGON((0 0,2 0,2 2,0 2))"),
                  false);

    tester::apply(from_wkt<R>("POLYGON((3 3,3 4,4 4,4 3))"),
                  from_wkt<PL>("POLYGON((0 0,2 0,2 2,0 2))"),
                  true);
}

template <typename P>
inline void test_multipolygon_ring()
{
    typedef bg::model::ring<P, false, false> R; // ccw, open
    typedef bg::model::polygon<P, false, false> PL; // ccw, open
    typedef bg::model::multi_polygon<PL> MPL;

    typedef test_disjoint tester;

    tester::apply(from_wkt<R>("POLYGON((2 2,2 3,3 3,3 2))"),
                  from_wkt<MPL>("MULTIPOLYGON(((0 0,2 0,2 2,0 2)))"),
                  false);

    tester::apply(from_wkt<R>("POLYGON((1 1,1 3,3 3,3 1))"),
                  from_wkt<MPL>("MULTIPOLYGON(((0 0,2 0,2 2,0 2)))"),
                  false);

    tester::apply(from_wkt<R>("POLYGON((3 3,3 4,4 4,4 3))"),
                  from_wkt<MPL>("MULTIPOLYGON(((0 0,2 0,2 2,0 2)))"),
                  true);
}

template <typename P>
inline void test_polygon_polygon()
{
    typedef bg::model::polygon<P, false, false> PL; // ccw, open

    typedef test_disjoint tester;

    tester::apply(from_wkt<PL>("POLYGON((2 2,2 3,3 3,3 2))"),
                  from_wkt<PL>("POLYGON((0 0,2 0,2 2,0 2))"),
                  false);

    tester::apply(from_wkt<PL>("POLYGON((1 1,1 3,3 3,3 1))"),
                  from_wkt<PL>("POLYGON((0 0,2 0,2 2,0 2))"),
                  false);

    tester::apply(from_wkt<PL>("POLYGON((3 3,3 4,4 4,4 3))"),
                  from_wkt<PL>("POLYGON((0 0,2 0,2 2,0 2))"),
                  true);
}

template <typename P>
inline void test_polygon_multipolygon()
{
    typedef bg::model::polygon<P, false, false> PL; // ccw, open
    typedef bg::model::multi_polygon<PL> MPL;

    typedef test_disjoint tester;

    tester::apply(from_wkt<PL>("POLYGON((2 2,2 3,3 3,3 2))"),
                  from_wkt<MPL>("MULTIPOLYGON(((0 0,2 0,2 2,0 2)))"),
                  false);

    tester::apply(from_wkt<PL>("POLYGON((1 1,1 3,3 3,3 1))"),
                  from_wkt<MPL>("MULTIPOLYGON(((0 0,2 0,2 2,0 2)))"),
                  false);

    tester::apply(from_wkt<PL>("POLYGON((3 3,3 4,4 4,4 3))"),
                  from_wkt<MPL>("MULTIPOLYGON(((0 0,2 0,2 2,0 2)))"),
                  true);
}

template <typename P>
inline void test_multipolygon_multipolygon()
{
    typedef bg::model::polygon<P, false, false> PL; // ccw, open
    typedef bg::model::multi_polygon<PL> MPL;

    typedef test_disjoint tester;

    tester::apply(from_wkt<MPL>("MULTIPOLYGON(((2 2,2 3,3 3,3 2)))"),
                  from_wkt<MPL>("MULTIPOLYGON(((0 0,2 0,2 2,0 2)))"),
                  false);

    tester::apply(from_wkt<MPL>("MULTIPOLYGON(((1 1,1 3,3 3,3 1)))"),
                  from_wkt<MPL>("MULTIPOLYGON(((0 0,2 0,2 2,0 2)))"),
                  false);

    tester::apply(from_wkt<MPL>("MULTIPOLYGON(((3 3,3 4,4 4,4 3)))"),
                  from_wkt<MPL>("MULTIPOLYGON(((0 0,2 0,2 2,0 2)))"),
                  true);
}

//============================================================================

template <typename CoordinateType>
inline void test_pointlike_pointlike()
{
    typedef bg::model::point<CoordinateType, 2, bg::cs::cartesian> point_type;

    test_point_point<point_type>();
    // not implemented yet
    //    test_point_multipoint<point_type>();

    //    test_multipoint_multipoint<point_type>();
}

template <typename CoordinateType>
inline void test_pointlike_linear()
{
    typedef bg::model::point<CoordinateType, 2, bg::cs::cartesian> point_type;

    test_point_linestring<point_type>();
    test_point_multilinestring<point_type>();
    test_point_segment<point_type>();

    // not implemented yet
    //    test_multipoint_linestring<point_type>();
    //    test_multipoint_multilinestring<point_type>();
    //    test_multipoint_segment<point_type>();
}

template <typename CoordinateType>
inline void test_pointlike_areal()
{
    typedef bg::model::point<CoordinateType, 2, bg::cs::cartesian> point_type;

    test_point_polygon<point_type>();
    test_point_multipolygon<point_type>();
    test_point_ring<point_type>();
    test_point_box<point_type>();

    // not implemented yet
    //    test_multipoint_polygon<point_type>();
    //    test_multipoint_multipolygon<point_type>();
    //    test_multipoint_ring<point_type>();
    //    test_multipoint_box<point_type>();
}

template <typename CoordinateType>
inline void test_linear_linear()
{
    typedef bg::model::point<CoordinateType, 2, bg::cs::cartesian> point_type;

    test_linestring_linestring<point_type>();
    test_linestring_multilinestring<point_type>();
    test_linestring_segment<point_type>();

    test_multilinestring_multilinestring<point_type>();
    test_multilinestring_segment<point_type>();

    test_segment_segment<point_type>();
}

template <typename CoordinateType>
inline void test_linear_areal()
{
    typedef bg::model::point<CoordinateType, 2, bg::cs::cartesian> point_type;

    test_segment_polygon<point_type>();
    test_segment_multipolygon<point_type>();
    test_segment_ring<point_type>();
    test_segment_box<point_type>();

    test_linestring_polygon<point_type>();
    test_linestring_multipolygon<point_type>();
    test_linestring_ring<point_type>();
    test_linestring_box<point_type>();

    test_multilinestring_polygon<point_type>();
    test_multilinestring_multipolygon<point_type>();
    test_multilinestring_ring<point_type>();
    test_multilinestring_box<point_type>();
}

template <typename CoordinateType>
inline void test_areal_areal()
{
    typedef bg::model::point<CoordinateType, 2, bg::cs::cartesian> point_type;

    test_polygon_polygon<point_type>();
    test_polygon_multipolygon<point_type>();
    test_polygon_ring<point_type>();
    test_polygon_box<point_type>();

    test_multipolygon_multipolygon<point_type>();
    test_multipolygon_ring<point_type>();
    test_multipolygon_box<point_type>();

    test_ring_ring<point_type>();
    test_ring_box<point_type>();

    test_box_box<point_type>();
}

//============================================================================

BOOST_AUTO_TEST_CASE( test_pointlike_pointlike_all )
{
    test_pointlike_pointlike<double>();
    test_pointlike_pointlike<int>();
#ifdef HAVE_TTMATH
    test_pointlike_pointlike<ttmath_big>();
#endif
}

BOOST_AUTO_TEST_CASE( test_pointlike_linear_all )
{
    test_pointlike_linear<double>();
    test_pointlike_linear<int>();
#ifdef HAVE_TTMATH
    test_pointlike_linear<ttmath_big>();
#endif
}

BOOST_AUTO_TEST_CASE( test_pointlike_areal_all )
{
    test_pointlike_areal<double>();
    test_pointlike_areal<int>();
#ifdef HAVE_TTMATH
    test_pointlike_areal<ttmath_big>();
#endif
}

BOOST_AUTO_TEST_CASE( test_linear_linear_all )
{
    test_linear_linear<double>();
    test_linear_linear<int>();
#ifdef HAVE_TTMATH
    test_linear_linear<ttmath_big>();
#endif
}

BOOST_AUTO_TEST_CASE( test_linear_areal_all )
{
    test_linear_areal<double>();
    test_linear_areal<int>();
#ifdef HAVE_TTMATH
    test_linear_areal<ttmath_big>();
#endif
}

BOOST_AUTO_TEST_CASE( test_areal_areal_all )
{
    test_areal_areal<double>();
    test_areal_areal<int>();
#ifdef HAVE_TTMATH
    test_areal_areal<ttmath_big>();
#endif
}
