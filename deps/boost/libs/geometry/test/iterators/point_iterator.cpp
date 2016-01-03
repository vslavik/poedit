// Boost.Geometry (aka GGL, Generic Geometry Library)
// Unit Test

// Copyright (c) 2014, Oracle and/or its affiliates.

// Contributed and/or modified by Menelaos Karavelas, on behalf of Oracle

// Licensed under the Boost Software License version 1.0.
// http://www.boost.org/users/license.html


#ifndef BOOST_TEST_MODULE
#define BOOST_TEST_MODULE test_point_iterator
#endif

#include <cstddef>
#include <iostream>
#include <string>
#include <iterator>
#include <algorithm>

#include <boost/test/included/unit_test.hpp>

#include <boost/assign/list_of.hpp>
#include <boost/concept_check.hpp>
#include <boost/core/ignore_unused.hpp>
#include <boost/iterator/iterator_concepts.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/type_traits/is_const.hpp>

#include <boost/geometry/core/point_type.hpp>
#include <boost/geometry/multi/core/point_type.hpp>

#include <boost/geometry/geometries/geometries.hpp>
#include <boost/geometry/geometries/adapted/boost_tuple.hpp>
#include <boost/geometry/multi/geometries/multi_geometries.hpp>
#include <boost/geometry/multi/geometries/register/multi_point.hpp>

#include <boost/geometry/algorithms/equals.hpp>
#include <boost/geometry/algorithms/num_points.hpp>
#include <boost/geometry/multi/algorithms/num_points.hpp>

#include <boost/geometry/policies/compare.hpp>

#include <boost/geometry/io/wkt/wkt.hpp>
#include <boost/geometry/io/dsv/write.hpp>

#include <boost/geometry/iterators/point_iterator.hpp>
#include <boost/geometry/iterators/point_reverse_iterator.hpp>

namespace bg = ::boost::geometry;
namespace ba = ::boost::assign;

typedef bg::model::point<double, 2, bg::cs::cartesian> point_type;
typedef bg::model::point<double, 3, bg::cs::cartesian> point_type_3d;
typedef bg::model::linestring<point_type> linestring_type;
typedef bg::model::polygon<point_type, false, false> polygon_type; //ccw, open

// multi geometries
typedef bg::model::multi_point<point_type> multi_point_type;
typedef bg::model::multi_point<point_type_3d> multi_point_type_3d;
typedef bg::model::multi_linestring<linestring_type> multi_linestring_type;
typedef bg::model::multi_polygon<polygon_type> multi_polygon_type;

typedef boost::tuple<double, double> tuple_point_type;
typedef boost::tuple<double, double, double> tuple_point_type_3d;
typedef std::vector<tuple_point_type> tuple_multi_point_type;
typedef std::vector<tuple_point_type_3d> tuple_multi_point_type_3d;

BOOST_GEOMETRY_REGISTER_BOOST_TUPLE_CS(cs::cartesian)
BOOST_GEOMETRY_REGISTER_MULTI_POINT(tuple_multi_point_type)
BOOST_GEOMETRY_REGISTER_MULTI_POINT(tuple_multi_point_type_3d)


template <typename Geometry>
inline Geometry from_wkt(std::string const& wkt)
{
    Geometry geometry;
    boost::geometry::read_wkt(wkt, geometry);
    return geometry;
}


template <typename Iterator>
inline std::ostream& print_point_range(std::ostream& os,
                                       Iterator first,
                                       Iterator beyond,
                                       std::string const& header)
{
    os << header << "(";
    for (Iterator it = first; it != beyond; ++it)
    {
        os << " " << bg::dsv(*it);
    }
    os << " )";
    return os;
}


template
<
    typename Geometry,
    bool IsConst = boost::is_const<Geometry>::value
>
struct test_iterator_concepts
{
    typedef bg::point_iterator<Geometry> iterator;
    BOOST_CONCEPT_ASSERT(( boost::BidirectionalIteratorConcept<iterator> ));
    BOOST_CONCEPT_ASSERT(( boost_concepts::ReadableIteratorConcept<iterator> ));
    BOOST_CONCEPT_ASSERT(( boost_concepts::LvalueIteratorConcept<iterator> ));
    BOOST_CONCEPT_ASSERT
        (( boost_concepts::BidirectionalTraversalConcept<iterator> ));
};

template <typename Geometry>
struct test_iterator_concepts<Geometry, false>
    : test_iterator_concepts<Geometry, true>
{
    typedef bg::point_iterator<Geometry> iterator;
    BOOST_CONCEPT_ASSERT
        (( boost::Mutable_BidirectionalIteratorConcept<iterator> ));
    BOOST_CONCEPT_ASSERT
        (( boost_concepts::WritableIteratorConcept<iterator> ));
    BOOST_CONCEPT_ASSERT
        (( boost_concepts::SwappableIteratorConcept<iterator> ));
};


struct equals
{
    template <typename Iterator>
    static inline std::size_t number_of_elements(Iterator begin,
                                                 Iterator end)
    {
        std::size_t size = std::distance(begin, end);

        std::size_t num_elems(0);
        for (Iterator it = begin; it != end; ++it)
        {
            ++num_elems;
        }
        BOOST_CHECK( size == num_elems );

        num_elems = 0;
        for (Iterator it = end; it != begin; --it)
        {
            ++num_elems;
        }
        BOOST_CHECK( size == num_elems );

        return num_elems;
    }

    template <typename Iterator1, typename Iterator2>
    static inline bool apply(Iterator1 begin1, Iterator1 end1,
                             Iterator2 begin2, Iterator2 end2)
    {
        std::size_t num_points1 = number_of_elements(begin1, end1);
        std::size_t num_points2 = number_of_elements(begin2, end2);

        if ( num_points1 != num_points2 )
        {
            return false;
        }

        Iterator1 it1 = begin1;
        Iterator2 it2 = begin2;
        for (; it1 != end1; ++it1, ++it2)
        {
            if ( !bg::equals(*it1, *it2) )
            {
                return false;
            }
        }
        return true;
    }
};


struct test_assignment
{
    template <typename Iterator, typename ConstIterator, typename Value>
    static inline void apply(Iterator it, ConstIterator cit,
                             Value const& value1, Value const& value2)
    {
#ifdef BOOST_GEOMETRY_TEST_DEBUG
        std::cout << "== before assignment ==" << std::endl;
        std::cout << "value1: " << bg::wkt(value1) << std::endl;
        std::cout << "value2: " << bg::wkt(value2) << std::endl;
        std::cout << "*it   : " << bg::wkt(*it) << std::endl;
        std::cout << "*cit  : " << bg::wkt(*cit) << std::endl;
#endif

        BOOST_CHECK( bg::equals(*it, value1) );
        BOOST_CHECK( !bg::equals(*it, value2) );
        BOOST_CHECK( bg::equals(*cit, value1) );
        BOOST_CHECK( !bg::equals(*cit, value2) );

        *it = value2;
        BOOST_CHECK( bg::equals(*it, value2) );
        BOOST_CHECK( !bg::equals(*it, value1) );
        BOOST_CHECK( bg::equals(*cit, value2) );
        BOOST_CHECK( !bg::equals(*cit, value1) );

#ifdef BOOST_GEOMETRY_TEST_DEBUG
        std::cout << "== after 1st assignment ==" << std::endl;
        std::cout << "value1: " << bg::wkt(value1) << std::endl;
        std::cout << "value2: " << bg::wkt(value2) << std::endl;
        std::cout << "*it   : " << bg::wkt(*it) << std::endl;
        std::cout << "*cit  : " << bg::wkt(*cit) << std::endl;
#endif

        *it = value1;
        BOOST_CHECK( bg::equals(*it, value1) );
        BOOST_CHECK( !bg::equals(*it, value2) );
        BOOST_CHECK( bg::equals(*cit, value1) );
        BOOST_CHECK( !bg::equals(*cit, value2) );

#ifdef BOOST_GEOMETRY_TEST_DEBUG
        std::cout << "== after 2nd assignment ==" << std::endl;
        std::cout << "value1: " << bg::wkt(value1) << std::endl;
        std::cout << "value2: " << bg::wkt(value2) << std::endl;
        std::cout << "*it   : " << bg::wkt(*it) << std::endl;
        std::cout << "*cit  : " << bg::wkt(*cit) << std::endl;
        std::cout << std::endl;
#endif
    }
};


template <typename Geometry, typename PointRange>
struct test_point_iterator_of_geometry
{
    template <typename G>
    static inline void base_test(G& geometry,
                                 PointRange const& point_range,
                                 std::string const& header)
    {
        typedef bg::point_iterator<G> point_iterator;

        test_iterator_concepts<G>();

        point_iterator begin = bg::points_begin(geometry);
        point_iterator end = bg::points_end(geometry);

        BOOST_CHECK( std::size_t(std::distance(begin, end))
                     ==
                     bg::num_points(geometry) );

        BOOST_CHECK( equals::apply(begin, end,
                                   bg::points_begin(point_range),
                                   bg::points_end(point_range))
                     );

        boost::ignore_unused(header);

#ifdef BOOST_GEOMETRY_TEST_DEBUG
        std::cout << header << " geometry: " << bg::wkt(geometry) << std::endl;
        print_point_range(std::cout, begin, end, "point range: ");
        std::cout << std::endl;

        typedef bg::point_iterator<PointRange const> point_range_iterator;

        print_point_range(std::cout,
                          bg::points_begin(point_range),
                          bg::points_end(point_range),
                          "expected point range: ");
        std::cout << std::endl;
#endif
    }

    static inline void apply(Geometry geometry, PointRange const& point_range)
    {
        base_test<Geometry>(geometry, point_range, "non-const");

#ifdef BOOST_GEOMETRY_TEST_DEBUG
        std::cout << std::endl;
#endif

        base_test<Geometry const>(geometry, point_range, "const");

#ifdef BOOST_GEOMETRY_TEST_DEBUG
        std::cout << std::endl << std::endl;
#endif

        // testing construction of const and non-const iterator
        typedef bg::point_iterator<Geometry> point_iterator;
        typedef bg::point_iterator<Geometry const> const_point_iterator;

        point_iterator begin = bg::points_begin(geometry);
        point_iterator end = bg::points_end(geometry);

        const_point_iterator const_begin = bg::points_begin(geometry);
        const_point_iterator const_end = bg::points_end(geometry);

        // same for reverse iterator
        typedef bg::point_reverse_iterator<Geometry> point_reverse_iterator;
        typedef bg::point_reverse_iterator
            <
                Geometry const
            > const_point_reverse_iterator;

        point_reverse_iterator rbegin = bg::points_rbegin(geometry);
        point_reverse_iterator rend = bg::points_rend(geometry);

        const_point_reverse_iterator const_rbegin = bg::points_rbegin(geometry);
        const_point_reverse_iterator const_rend = bg::points_rend(geometry);

        // testing assignment of non-const to const iterator
        const_begin = begin;
        const_end = end;

        // testing assignment of non-const to const reverse_iterator
        const_rbegin = rbegin;
        const_rend = rend;

        // testing equality/inequality comparison
        BOOST_CHECK ( begin == const_begin );
        BOOST_CHECK ( end == const_end );
        if ( begin != end )
        {
            BOOST_CHECK ( begin != const_end );
            BOOST_CHECK ( const_begin != end );
        }

        // testing equality/inequality comparison for reverse_iterator
        BOOST_CHECK ( rbegin == const_rbegin );
        BOOST_CHECK ( rend == const_rend );
        if ( rbegin != rend )
        {
            BOOST_CHECK ( rbegin != const_rend );
            BOOST_CHECK ( const_rbegin != rend );
        }

        if ( begin != end )
        {
            BOOST_CHECK( rbegin != rend );

            point_reverse_iterator rlast(rend);
            --rlast;
            BOOST_CHECK( bg::equals(*begin, *rlast) );

            point_iterator last(end);
            --last;
            BOOST_CHECK( bg::equals(*rbegin, *last) );
        }
        
        // testing dereferencing/assignment
        if ( begin != end )
        {
            typedef typename bg::point_type<Geometry>::type point;

            point p = *begin;
            point q = bg::make_zero<point>();

            test_assignment::apply(begin, const_begin, p, q);

            *begin = q;
            test_assignment::apply(begin, const_begin, q, p);

            *begin = p;
        }

        // test with algorithms
#ifdef BOOST_GEOMETRY_TEST_DEBUG
        print_point_range(std::cout, begin, end, "original:\n") << std::endl;
        print_point_range(std::cout, rbegin, rend, "reverse traversal:\n")
            << std::endl;
        std::cout << bg::wkt(geometry) << std::endl;
        std::cout << std::endl;
#endif

        std::reverse(begin, end);
#ifdef BOOST_GEOMETRY_TEST_DEBUG
        print_point_range(std::cout, begin, end, "reversed:\n") << std::endl;
        std::cout << bg::wkt(geometry) << std::endl;
        std::cout << std::endl;
#endif

        std::reverse(begin, end);
#ifdef BOOST_GEOMETRY_TEST_DEBUG
        print_point_range(std::cout, begin, end, "re-reversed:\n") << std::endl;
        std::cout << bg::wkt(geometry) << std::endl;
        std::cout << std::endl;
        std::cout << std::endl;
#endif

        typedef typename std::iterator_traits
            <
                point_iterator
            >::value_type point;
        if ( const_begin != const_end )
        {
            const_point_iterator pit_max = std::max_element(const_begin,
                                                            const_end,
                                                            bg::less<point>());

            BOOST_CHECK( pit_max != const_end ); // to avoid warnings
#ifdef BOOST_GEOMETRY_TEST_DEBUG
            std::cout << "max point: " << bg::dsv(*pit_max) << std::endl;
#endif
        }
#ifdef BOOST_GEOMETRY_TEST_DEBUG
        std::cout << std::endl;
        std::cout << std::endl;
        std::cout << std::endl;
#endif
    }
};


//======================================================================
//======================================================================


BOOST_AUTO_TEST_CASE( test_linestring_point_iterator )
{
#ifdef BOOST_GEOMETRY_TEST_DEBUG
    std::cout << "*** LINESTRING ***" << std::endl;
#endif

    typedef tuple_multi_point_type TMP;
    typedef linestring_type L;

    typedef test_point_iterator_of_geometry<L, TMP> tester;

    tester::apply(from_wkt<L>("LINESTRING()"),
                  TMP()
                  );

    tester::apply(from_wkt<L>("LINESTRING(3 3,4 4,5 5)"),
                  ba::tuple_list_of(3,3)(4,4)(5,5)
                  );

#ifdef BOOST_GEOMETRY_TEST_DEBUG
    std::cout << std::endl << std::endl << std::endl;
#endif
}


//======================================================================
//======================================================================


BOOST_AUTO_TEST_CASE( test_polygon_point_iterator )
{
#ifdef BOOST_GEOMETRY_TEST_DEBUG
    std::cout << "*** POLYGON ***" << std::endl;
#endif

    typedef tuple_multi_point_type TMP;
    typedef polygon_type P;

    typedef test_point_iterator_of_geometry<P, TMP> tester;

    tester::apply(from_wkt<P>("POLYGON()"),
                  TMP()
                  );

    tester::apply(from_wkt<P>("POLYGON(())"),
                  TMP()
                  );

    tester::apply(from_wkt<P>("POLYGON((1 1,9 1,9 9,1 9),(5 5,6 5,6 6,5 6))"),
                  ba::tuple_list_of(1,1)(9,1)(9,9)(1,9)(5,5)(6,5)(6,6)(5,6)
                  );

    tester::apply(from_wkt<P>("POLYGON((3 3,4 4,5 5),(),(),(),(6 6,7 7,8 8),(),(),(9 9),())"),
                  ba::tuple_list_of(3,3)(4,4)(5,5)(6,6)(7,7)(8,8)(9,9)
                  );

    tester::apply(from_wkt<P>("POLYGON((),(3 3,4 4,5 5),(),(),(6 6,7 7,8 8),(),(),(9 9),())"),
                  ba::tuple_list_of(3,3)(4,4)(5,5)(6,6)(7,7)(8,8)(9,9)
                  );

#ifdef BOOST_GEOMETRY_TEST_DEBUG
    std::cout << std::endl << std::endl;
#endif
}


//======================================================================
//======================================================================


BOOST_AUTO_TEST_CASE( test_multipoint_point_iterator )
{
#ifdef BOOST_GEOMETRY_TEST_DEBUG
    std::cout << "*** MULTIPOINT ***" << std::endl;
#endif

    typedef tuple_multi_point_type TMP;
    typedef multi_point_type MP;

    typedef test_point_iterator_of_geometry<MP, TMP> tester;

    tester::apply(from_wkt<MP>("MULTIPOINT()"),
                  TMP()
                  );

    tester::apply(from_wkt<MP>("MULTIPOINT(3 3,4 4,5 5)"),
                  ba::tuple_list_of(3,3)(4,4)(5,5)
                  );

#ifdef BOOST_GEOMETRY_TEST_DEBUG
    std::cout << std::endl << std::endl << std::endl;
#endif
}


//======================================================================
//======================================================================


BOOST_AUTO_TEST_CASE( test_multipoint_3d_point_iterator )
{
#ifdef BOOST_GEOMETRY_TEST_DEBUG
    std::cout << "*** MULTIPOINT 3D ***" << std::endl;
#endif

    typedef tuple_multi_point_type_3d TMP;
    typedef multi_point_type_3d MP;

    typedef test_point_iterator_of_geometry<MP, TMP> tester;

    tester::apply(from_wkt<MP>("MULTIPOINT()"),
                  TMP()
                  );

    tester::apply(from_wkt<MP>("MULTIPOINT(3 3 3,4 4 4,5 5 5)"),
                  ba::tuple_list_of(3,3,3)(4,4,4)(5,5,5)
                  );

#ifdef BOOST_GEOMETRY_TEST_DEBUG
    std::cout << std::endl << std::endl << std::endl;
#endif
}


//======================================================================
//======================================================================


BOOST_AUTO_TEST_CASE( test_multilinestring_point_iterator )
{
#ifdef BOOST_GEOMETRY_TEST_DEBUG
    std::cout << "*** MULTILINESTRING ***" << std::endl;
#endif

    typedef tuple_multi_point_type TMP;
    typedef multi_linestring_type ML;

    typedef test_point_iterator_of_geometry<ML, TMP> tester;

    tester::apply(from_wkt<ML>("MULTILINESTRING()"),
                  TMP()
                  );

    tester::apply(from_wkt<ML>("MULTILINESTRING(())"),
                  TMP()
                  );

    tester::apply(from_wkt<ML>("MULTILINESTRING((),(),())"),
                  TMP()
                  );

    tester::apply(from_wkt<ML>("MULTILINESTRING((1 1,2 2,3 3),(3 3,4 4,5 5),(6 6))"),
                  ba::tuple_list_of(1,1)(2,2)(3,3)(3,3)(4,4)(5,5)(6,6)
                  );

    tester::apply(from_wkt<ML>("MULTILINESTRING((),(),(1 1,2 2,3 3),(),(),(3 3,4 4,5 5),(),(6 6),(),(),())"),
                  ba::tuple_list_of(1,1)(2,2)(3,3)(3,3)(4,4)(5,5)(6,6)
                  );

#ifdef BOOST_GEOMETRY_TEST_DEBUG
    std::cout << std::endl << std::endl;
#endif
}


//======================================================================
//======================================================================


BOOST_AUTO_TEST_CASE( test_multipolygon_point_iterator )
{
#ifdef BOOST_GEOMETRY_TEST_DEBUG
    std::cout << "*** MULTIPOLYGON ***" << std::endl;
#endif

    typedef tuple_multi_point_type TMP;
    typedef multi_polygon_type MPL;

    typedef test_point_iterator_of_geometry<MPL, TMP> tester;

    tester::apply(from_wkt<MPL>("MULTIPOLYGON()"),
                  TMP()
                  );

    tester::apply(from_wkt<MPL>("MULTIPOLYGON( () )"),
                  TMP()
                  );

    tester::apply(from_wkt<MPL>("MULTIPOLYGON( (()) )"),
                  TMP()
                  );

    tester::apply(from_wkt<MPL>("MULTIPOLYGON( ((),()) )"),
                  TMP()
                  );

    tester::apply(from_wkt<MPL>("MULTIPOLYGON(((3 3,4 4,5 5),(6 6,7 7,8 8),(9 9)),((1 1,2 2,10 10),(11 11,12 12)))"),
                  ba::tuple_list_of(3,3)(4,4)(5,5)(6,6)(7,7)(8,8)(9,9)\
                  (1,1)(2,2)(10,10)(11,11)(12,12)
                  );

    tester::apply(from_wkt<MPL>("MULTIPOLYGON(((3 3,4 4,5 5),(),(),(),(6 6,7 7,8 8),(),(),(9 9),()),((),(1 1,2 2,10 10),(),(),(),(11 11,12 12),(),(),(13 13),()))"),
                  ba::tuple_list_of(3,3)(4,4)(5,5)(6,6)(7,7)(8,8)(9,9)\
                  (1,1)(2,2)(10,10)(11,11)(12,12)(13,13)
                  );

    tester::apply(from_wkt<MPL>("MULTIPOLYGON(((3 3,4 4,5 5),(),(),(),(6 6,7 7,8 8),(),(),(9 9),()),((),(1 1,2 2,10 10),(),(),(),(11 11,12 12),(),(),(13 13),()),((),(),()))"),
                  ba::tuple_list_of(3,3)(4,4)(5,5)(6,6)(7,7)(8,8)(9,9)\
                  (1,1)(2,2)(10,10)(11,11)(12,12)(13,13)
                  );

#ifdef BOOST_GEOMETRY_TEST_DEBUG
    std::cout << std::endl << std::endl;
#endif
}
