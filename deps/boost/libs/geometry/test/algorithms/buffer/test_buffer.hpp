// Boost.Geometry (aka GGL, Generic Geometry Library)
// Unit Test

// Copyright (c) 2010-2014 Barend Gehrels, Amsterdam, the Netherlands.
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef BOOST_GEOMETRY_TEST_BUFFER_HPP
#define BOOST_GEOMETRY_TEST_BUFFER_HPP

#include <iostream>
#include <fstream>
#include <iomanip>

#if defined(TEST_WITH_SVG)
#define BOOST_GEOMETRY_BUFFER_USE_HELPER_POINTS

// Uncomment next lines if you want to have a zoomed view
// #define BOOST_GEOMETRY_BUFFER_TEST_SVG_USE_ALTERNATE_BOX

// If possible define box before including this unit with the right view
#ifdef BOOST_GEOMETRY_BUFFER_TEST_SVG_USE_ALTERNATE_BOX
#  ifndef BOOST_GEOMETRY_BUFFER_TEST_SVG_ALTERNATE_BOX
#    define BOOST_GEOMETRY_BUFFER_TEST_SVG_ALTERNATE_BOX "BOX(0 0,100 100)"
#  endif
#endif

#endif // TEST_WITH_SVG

#include <boost/foreach.hpp>
#include <geometry_test_common.hpp>


#include <boost/geometry/algorithms/envelope.hpp>
#include <boost/geometry/algorithms/area.hpp>
#include <boost/geometry/algorithms/buffer.hpp>
#include <boost/geometry/algorithms/correct.hpp>
#include <boost/geometry/algorithms/disjoint.hpp>
#include <boost/geometry/algorithms/intersects.hpp>
#include <boost/geometry/algorithms/is_valid.hpp>
#include <boost/geometry/algorithms/union.hpp>

#include <boost/geometry/algorithms/detail/overlay/debug_turn_info.hpp>
#include <boost/geometry/algorithms/detail/overlay/self_turn_points.hpp>

#include <boost/geometry/geometries/geometries.hpp>

#include <boost/geometry/strategies/strategies.hpp>

#include <boost/geometry/strategies/buffer.hpp>

#include <boost/geometry/io/wkt/wkt.hpp>

#include <boost/geometry/util/condition.hpp>


#if defined(TEST_WITH_SVG)

#include <boost/geometry/io/svg/svg_mapper.hpp>


inline char piece_type_char(bg::strategy::buffer::piece_type const& type)
{
    using namespace bg::strategy::buffer;
    switch(type)
    {
        case buffered_segment : return 's';
        case buffered_join : return 'j';
        case buffered_round_end : return 'r';
        case buffered_flat_end : return 'f';
        case buffered_point : return 'p';
        case buffered_concave : return 'c';
        default : return '?';
    }
}

template <typename Geometry, typename Mapper, typename RescalePolicy>
void post_map(Geometry const& geometry, Mapper& mapper, RescalePolicy const& rescale_policy)
{
    typedef typename bg::point_type<Geometry>::type point_type;
    typedef bg::detail::overlay::turn_info
    <
        point_type,
        typename bg::segment_ratio_type<point_type, RescalePolicy>::type
    > turn_info;

    std::vector<turn_info> turns;

    bg::detail::self_get_turn_points::no_interrupt_policy policy;
    bg::self_turns
        <
            bg::detail::overlay::assign_null_policy
        >(geometry, rescale_policy, turns, policy);

    BOOST_FOREACH(turn_info const& turn, turns)
    {
        mapper.map(turn.point, "fill:rgb(255,128,0);stroke:rgb(0,0,100);stroke-width:1", 3);
    }
}

template <typename SvgMapper, typename Box, typename Tag>
struct svg_visitor
{
#ifdef BOOST_GEOMETRY_BUFFER_TEST_SVG_USE_ALTERNATE_BOX
    Box m_alternate_box;
#endif

    class si
    {
    private :
        bg::segment_identifier m_id;

    public :
        inline si(bg::segment_identifier const& id)
            : m_id(id)
        {}

        template <typename Char, typename Traits>
        inline friend std::basic_ostream<Char, Traits>& operator<<(
                std::basic_ostream<Char, Traits>& os,
                si const& s)
        {
            os << s.m_id.multi_index << "." << s.m_id.segment_index;
            return os;
        }
    };


    SvgMapper& m_mapper;

    svg_visitor(SvgMapper& mapper)
        : m_mapper(mapper)
    {}

    template <typename Turns>
    inline void map_turns(Turns const& turns, bool label_good_turns, bool label_wrong_turns)
    {
        namespace bgdb = boost::geometry::detail::buffer;
        typedef typename boost::range_value<Turns const>::type turn_type;
        typedef typename turn_type::point_type point_type;
        typedef typename turn_type::robust_point_type robust_point_type;

        std::map<robust_point_type, int, bg::less<robust_point_type> > offsets;

        for (typename boost::range_iterator<Turns const>::type it =
            boost::begin(turns); it != boost::end(turns); ++it)
        {
#ifdef BOOST_GEOMETRY_BUFFER_TEST_SVG_USE_ALTERNATE_BOX
            if (bg::disjoint(it->point, m_alternate_box))
            {
                continue;
            }
#endif

            bool is_good = true;
            char color = 'g';
            std::string fill = "fill:rgb(0,255,0);";
            switch(it->location)
            {
                case bgdb::inside_buffer :
                    fill = "fill:rgb(255,0,0);";
                    color = 'r';
                    is_good = false;
                    break;
                case bgdb::location_discard :
                    fill = "fill:rgb(0,0,255);";
                    color = 'b';
                    is_good = false;
                    break;
            }
            if (!it->selectable_start)
            {
                fill = "fill:rgb(255,192,0);";
                color = 'o'; // orange
            }
            if (it->blocked())
            {
                fill = "fill:rgb(128,128,128);";
                color = '-';
                is_good = false;
            }

            fill += "fill-opacity:0.7;";

            m_mapper.map(it->point, fill, 4);

            if ((label_good_turns && is_good) || (label_wrong_turns && ! is_good))
            {
                std::ostringstream out;
                out << it->turn_index
                    << " " << it->operations[0].piece_index << "/" << it->operations[1].piece_index
                    << " " << si(it->operations[0].seg_id) << "/" << si(it->operations[1].seg_id)

    //              If you want to see travel information
                    << std::endl
                    << " nxt " << it->operations[0].enriched.travels_to_ip_index
                    << "/" << it->operations[1].enriched.travels_to_ip_index
                    << " or " << it->operations[0].enriched.next_ip_index
                    << "/" << it->operations[1].enriched.next_ip_index
                    //<< " frac " << it->operations[0].fraction

    //                If you want to see robust-point coordinates (e.g. to find duplicates)
    //                << std::endl
    //                << " " << bg::get<0>(it->robust_point) << " , " << bg::get<1>(it->robust_point)

                    << std::endl;
                out << " " << bg::method_char(it->method)
                    << ":" << bg::operation_char(it->operations[0].operation)
                    << "/" << bg::operation_char(it->operations[1].operation);
                out << " "
                    << (it->count_on_offsetted > 0 ? "b" : "") // b: offsetted border
                    << (it->count_within_near_offsetted > 0 ? "n" : "")
                    << (it->count_within > 0 ? "w" : "")
                    << (it->count_on_helper > 0 ? "h" : "")
                    << (it->count_on_multi > 0 ? "m" : "")
                    ;

                offsets[it->get_robust_point()] += 10;
                int offset = offsets[it->get_robust_point()];

                m_mapper.text(it->point, out.str(), "fill:rgb(0,0,0);font-family='Arial';font-size:9px;", 5, offset);

                offsets[it->get_robust_point()] += 25;
            }
        }
    }

    template <typename Pieces, typename OffsettedRings>
    inline void map_pieces(Pieces const& pieces,
                OffsettedRings const& offsetted_rings,
                bool do_pieces, bool do_indices)
    {
        typedef typename boost::range_value<Pieces const>::type piece_type;
        typedef typename boost::range_value<OffsettedRings const>::type ring_type;

        for(typename boost::range_iterator<Pieces const>::type it = boost::begin(pieces);
            it != boost::end(pieces);
            ++it)
        {
            const piece_type& piece = *it;
            bg::segment_identifier seg_id = piece.first_seg_id;
            if (seg_id.segment_index < 0)
            {
                continue;
            }

            ring_type corner;


            ring_type const& ring = offsetted_rings[seg_id.multi_index];

            std::copy(boost::begin(ring) + seg_id.segment_index,
                    boost::begin(ring) + piece.last_segment_index,
                    std::back_inserter(corner));
            std::copy(boost::begin(piece.helper_points),
                    boost::end(piece.helper_points),
                    std::back_inserter(corner));

            if (corner.empty())
            {
                continue;
            }
#ifdef BOOST_GEOMETRY_BUFFER_TEST_SVG_USE_ALTERNATE_BOX
            if (bg::disjoint(corner, m_alternate_box))
            {
                continue;
            }
#endif

            if (do_pieces)
            {
                std::string style = "opacity:0.3;stroke:rgb(0,0,0);stroke-width:1;";
                m_mapper.map(corner,
                    piece.type == bg::strategy::buffer::buffered_segment
                    ? style + "fill:rgb(255,128,0);"
                    : style + "fill:rgb(255,0,0);");
            }

            if (do_indices)
            {
                // Label starting piece_index / segment_index
                typedef typename bg::point_type<ring_type>::type point_type;

                std::ostringstream out;
                out << piece.index << " (" << piece_type_char(piece.type) << ") " << piece.first_seg_id.segment_index << ".." << piece.last_segment_index - 1;
                point_type label_point = bg::return_centroid<point_type>(corner);

                if ((piece.type == bg::strategy::buffer::buffered_concave
                     || piece.type == bg::strategy::buffer::buffered_flat_end)
                    && corner.size() >= 2u)
                {
                    bg::set<0>(label_point, (bg::get<0>(corner[0]) + bg::get<0>(corner[1])) / 2.0);
                    bg::set<1>(label_point, (bg::get<1>(corner[0]) + bg::get<1>(corner[1])) / 2.0);
                }
                m_mapper.text(label_point, out.str(), "fill:rgb(255,0,0);font-family='Arial';font-size:10px;", 5, 5);
            }
        }
    }

    template <typename TraversedRings>
    inline void map_traversed_rings(TraversedRings const& traversed_rings)
    {
        for(typename boost::range_iterator<TraversedRings const>::type it
                = boost::begin(traversed_rings); it != boost::end(traversed_rings); ++it)
        {
            m_mapper.map(*it, "opacity:0.4;fill:none;stroke:rgb(0,255,0);stroke-width:2");
        }
    }

    template <typename OffsettedRings>
    inline void map_offsetted_rings(OffsettedRings const& offsetted_rings)
    {
        for(typename boost::range_iterator<OffsettedRings const>::type it
                = boost::begin(offsetted_rings); it != boost::end(offsetted_rings); ++it)
        {
            if (it->discarded())
            {
                m_mapper.map(*it, "opacity:0.4;fill:none;stroke:rgb(255,0,0);stroke-width:2");
            }
            else
            {
                m_mapper.map(*it, "opacity:0.4;fill:none;stroke:rgb(0,0,255);stroke-width:2");
            }
        }
    }

    template <typename PieceCollection>
    inline void apply(PieceCollection const& collection, int phase)
    {
        // Comment next return if you want to see pieces, turns, etc.
        return;

        if(phase == 0)
        {
            map_pieces(collection.m_pieces, collection.offsetted_rings, true, true);
            map_turns(collection.m_turns, true, false);
        }
#if !defined(BOOST_GEOMETRY_BUFFER_TEST_SVG_USE_ALTERNATE_BOX)
        if (phase == 1)
        {
//        map_traversed_rings(collection.traversed_rings);
//        map_offsetted_rings(collection.offsetted_rings);
        }
#endif
    }
};

#endif

//-----------------------------------------------------------------------------
template <typename JoinStrategy>
struct JoinTestProperties
{
    static std::string name() { return "joinunknown"; }
};

template<> struct JoinTestProperties<boost::geometry::strategy::buffer::join_round>
{ 
    static std::string name() { return "round"; }
};

template<> struct JoinTestProperties<boost::geometry::strategy::buffer::join_miter>
{ 
    static std::string name() { return "miter"; }
};

template<> struct JoinTestProperties<boost::geometry::strategy::buffer::join_round_by_divide>
{ 
    static std::string name() { return "divide"; }
};


//-----------------------------------------------------------------------------
template <typename EndStrategy>
struct EndTestProperties { };

template<> struct EndTestProperties<boost::geometry::strategy::buffer::end_round>
{ 
    static std::string name() { return "round"; }
};

template<> struct EndTestProperties<boost::geometry::strategy::buffer::end_flat>
{ 
    static std::string name() { return "flat"; }
};



template <typename Geometry, typename RescalePolicy>
std::size_t count_self_ips(Geometry const& geometry, RescalePolicy const& rescale_policy)
{
    typedef typename bg::point_type<Geometry>::type point_type;
    typedef bg::detail::overlay::turn_info
    <
        point_type,
        typename bg::segment_ratio_type<point_type, RescalePolicy>::type
    > turn_info;

    std::vector<turn_info> turns;

    bg::detail::self_get_turn_points::no_interrupt_policy policy;
    bg::self_turns
        <
            bg::detail::overlay::assign_null_policy
        >(geometry, rescale_policy, turns, policy);

    return turns.size();
}

template
<
    typename GeometryOut,
    typename JoinStrategy,
    typename EndStrategy,
    typename DistanceStrategy,
    typename SideStrategy,
    typename PointStrategy,
    typename Geometry
>
void test_buffer(std::string const& caseid, Geometry const& geometry,
            JoinStrategy const& join_strategy,
            EndStrategy const& end_strategy,
            DistanceStrategy const& distance_strategy,
            SideStrategy const& side_strategy,
            PointStrategy const& point_strategy,
            bool check_self_intersections, double expected_area,
            double tolerance,
            std::size_t* self_ip_count)
{
    namespace bg = boost::geometry;

    typedef typename bg::coordinate_type<Geometry>::type coordinate_type;
    typedef typename bg::point_type<Geometry>::type point_type;

    typedef typename bg::tag<Geometry>::type tag;
    // TODO use something different here:
    std::string type = boost::is_same<tag, bg::polygon_tag>::value ? "poly"
        : boost::is_same<tag, bg::linestring_tag>::value ? "line"
        : boost::is_same<tag, bg::point_tag>::value ? "point"
        : boost::is_same<tag, bg::multi_polygon_tag>::value ? "multipoly"
        : boost::is_same<tag, bg::multi_linestring_tag>::value ? "multiline"
        : boost::is_same<tag, bg::multi_point_tag>::value ? "multipoint"
        : ""
        ;

    bg::model::box<point_type> envelope;
    bg::envelope(geometry, envelope);

    std::string join_name = JoinTestProperties<JoinStrategy>::name();
    std::string end_name = EndTestProperties<EndStrategy>::name();

    if ( BOOST_GEOMETRY_CONDITION((
            boost::is_same<tag, bg::point_tag>::value 
         || boost::is_same<tag, bg::multi_point_tag>::value )) )
    {
        join_name.clear();
    }

    std::ostringstream complete;
    complete
        << type << "_"
        << caseid << "_"
        << string_from_type<coordinate_type>::name()
        << "_" << join_name
        << (end_name.empty() ? "" : "_") << end_name
        << (distance_strategy.negative() ? "_deflate" : "")
        << (bg::point_order<GeometryOut>::value == bg::counterclockwise ? "_ccw" : "")
         // << "_" << point_buffer_count
        ;

    //std::cout << complete.str() << std::endl;

    std::ostringstream filename;
    filename << "buffer_" << complete.str() << ".svg";

#if defined(TEST_WITH_SVG)
    std::ofstream svg(filename.str().c_str());
    typedef bg::svg_mapper<point_type> mapper_type;
    mapper_type mapper(svg, 1000, 1000);

    svg_visitor<mapper_type, bg::model::box<point_type>, tag> visitor(mapper);

#ifdef BOOST_GEOMETRY_BUFFER_TEST_SVG_USE_ALTERNATE_BOX
    // Create a zoomed-in view
    bg::model::box<point_type> alternate_box;
    bg::read_wkt(BOOST_GEOMETRY_BUFFER_TEST_SVG_ALTERNATE_BOX, alternate_box);
    mapper.add(alternate_box);

    // Take care non-visible elements are skipped
    visitor.m_alternate_box = alternate_box;
#else

    {
        bg::model::box<point_type> box = envelope;
        if (distance_strategy.negative())
        {
            bg::buffer(box, box, 1.0);
        }
        else
        {
            bg::buffer(box, box, 1.1 * distance_strategy.max_distance(join_strategy, end_strategy));
        }
        mapper.add(box);
    }
#endif

#else
    bg::detail::buffer::visit_pieces_default_policy visitor;
#endif

    typedef typename bg::point_type<Geometry>::type point_type;
    typedef typename bg::rescale_policy_type<point_type>::type
        rescale_policy_type;

    // Enlarge the box to get a proper rescale policy
    bg::buffer(envelope, envelope, distance_strategy.max_distance(join_strategy, end_strategy));

    rescale_policy_type rescale_policy
            = bg::get_rescale_policy<rescale_policy_type>(envelope);

    bg::model::multi_polygon<GeometryOut> buffered;

    bg::detail::buffer::buffer_inserter<GeometryOut>(geometry,
                        std::back_inserter(buffered),
                        distance_strategy,
                        side_strategy,
                        join_strategy,
                        end_strategy,
                        point_strategy,
                        rescale_policy,
                        visitor);

    typename bg::default_area_result<GeometryOut>::type area = bg::area(buffered);

    //std::cout << caseid << " " << distance_left << std::endl;
    //std::cout << "INPUT: " << bg::wkt(geometry) << std::endl;
    //std::cout << "OUTPUT: " << area << std::endl;
    //std::cout << bg::wkt(buffered) << std::endl;


    if (expected_area > -0.1)
    {
        double const difference = area - expected_area;
        BOOST_CHECK_MESSAGE
            (
                bg::math::abs(difference) < tolerance,
                complete.str() << " not as expected. " 
                << std::setprecision(18)
                << " Expected: " << expected_area
                << " Detected: " << area
                << " Diff: " << difference
                << std::setprecision(3)
                << " , " << 100.0 * (difference / expected_area) << "%"
            );

        if (check_self_intersections)
        {
            // Be sure resulting polygon does not contain
            // self-intersections
            BOOST_CHECK_MESSAGE
                (
                    ! bg::detail::overlay::has_self_intersections(buffered,
                            rescale_policy, false),
                    complete.str() << " output is self-intersecting. "
                );
        }
    }

#ifdef BOOST_GEOMETRY_BUFFER_TEST_IS_VALID
    if (! bg::is_valid(buffered))
    {
        std::cout
            << "NOT VALID: " << complete.str() << std::endl
            << std::fixed << std::setprecision(16) << bg::wkt(buffered) << std::endl;
    }
//    BOOST_CHECK_MESSAGE(bg::is_valid(buffered) == true, complete.str() <<  " is not valid");
//    BOOST_CHECK_MESSAGE(bg::intersects(buffered) == false, complete.str() <<  " intersects");
#endif

#if defined(TEST_WITH_SVG)
    bool const areal = boost::is_same
        <
            typename bg::tag_cast<tag, bg::areal_tag>::type, bg::areal_tag
        >::type::value;

    // Map input geometry in green
    if (areal)
    {
#ifdef BOOST_GEOMETRY_BUFFER_TEST_SVG_USE_ALTERNATE_BOX_FOR_INPUT
        // Assuming input is areal
        bg::model::multi_polygon<GeometryOut> clipped;
        bg::intersection(geometry, alternate_box, clipped);
        mapper.map(clipped, "opacity:0.5;fill:rgb(0,128,0);stroke:rgb(0,64,0);stroke-width:2");
#else
        mapper.map(geometry, "opacity:0.5;fill:rgb(0,128,0);stroke:rgb(0,64,0);stroke-width:2");
#endif
    }
    else
    {
        // TODO: clip input points/linestring
        mapper.map(geometry, "opacity:0.5;stroke:rgb(0,128,0);stroke-width:10");
    }

    {
        // Map buffer in yellow (inflate) and with orange-dots (deflate)
        std::string style = distance_strategy.negative()
            ? "opacity:0.4;fill:rgb(255,255,192);stroke:rgb(255,128,0);stroke-width:3"
            : "opacity:0.4;fill:rgb(255,255,128);stroke:rgb(0,0,0);stroke-width:3";

#ifdef BOOST_GEOMETRY_BUFFER_TEST_SVG_USE_ALTERNATE_BOX
        // Clip output multi-polygon with box
        bg::model::multi_polygon<GeometryOut> clipped;
        bg::intersection(buffered, alternate_box, clipped);
        mapper.map(clipped, style);
#else
        mapper.map(buffered, style);
#endif
    }
    post_map(buffered, mapper, rescale_policy);
#endif // TEST_WITH_SVG

    if (self_ip_count != NULL)
    {
        std::size_t count = 0;
        if (bg::detail::overlay::has_self_intersections(buffered,
                rescale_policy, false))
        {
            count = count_self_ips(buffered, rescale_policy);
        }

        *self_ip_count += count;
        if (count > 0)
        {
            std::cout << complete.str() << " " << count << std::endl;
        }
    }
}


#ifdef BOOST_GEOMETRY_CHECK_WITH_POSTGIS
static int counter = 0;
#endif

template
<
    typename Geometry,
    typename GeometryOut,
    typename JoinStrategy,
    typename EndStrategy
>
void test_one(std::string const& caseid, std::string const& wkt,
        JoinStrategy const& join_strategy, EndStrategy const& end_strategy,
        double expected_area,
        double distance_left, double distance_right = -999,
        bool check_self_intersections = true,
        double tolerance = 0.01)
{
    namespace bg = boost::geometry;
    Geometry g;
    bg::read_wkt(wkt, g);
    bg::correct(g);


#ifdef BOOST_GEOMETRY_CHECK_WITH_POSTGIS
    std::cout
        << (counter > 0 ? "union " : "")
        << "select " << counter++
        << ", '" << caseid << "' as caseid"
        << ", ST_Area(ST_Buffer(ST_GeomFromText('" << wkt << "'), "
        << distance_left
        << ", 'endcap=" << end_name << " join=" << join_name << "'))"
        << ", "  << expected_area
        << std::endl;
#endif


    bg::strategy::buffer::side_straight side_strategy;
    bg::strategy::buffer::point_circle circle_strategy(88);

    bg::strategy::buffer::distance_asymmetric
    <
        typename bg::coordinate_type<Geometry>::type
    > distance_strategy(distance_left,
                        distance_right > -998 ? distance_right : distance_left);

    test_buffer<GeometryOut>
            (caseid, g,
            join_strategy, end_strategy,
            distance_strategy, side_strategy, circle_strategy,
            check_self_intersections, expected_area,
            tolerance, NULL);

#if !defined(BOOST_GEOMETRY_COMPILER_MODE_DEBUG) && defined(BOOST_GEOMETRY_COMPILER_MODE_RELEASE)

    // Also test symmetric distance strategy if right-distance is not specified
    // (only in release mode)
    if (bg::math::equals(distance_right, -999))
    {
        bg::strategy::buffer::distance_symmetric
        <
            typename bg::coordinate_type<Geometry>::type
        > sym_distance_strategy(distance_left);

        test_buffer<GeometryOut>
                (caseid + "_sym", g,
                join_strategy, end_strategy,
                sym_distance_strategy, side_strategy, circle_strategy,
                check_self_intersections, expected_area,
                tolerance, NULL);

    }
#endif
}

// Version (currently for the Aimes test) counting self-ip's instead of checking
template
<
    typename Geometry,
    typename GeometryOut,
    typename JoinStrategy,
    typename EndStrategy
>
void test_one(std::string const& caseid, std::string const& wkt,
        JoinStrategy const& join_strategy, EndStrategy const& end_strategy,
        double expected_area,
        double distance_left, double distance_right,
        std::size_t& self_ip_count,
        double tolerance = 0.01)
{
    namespace bg = boost::geometry;
    Geometry g;
    bg::read_wkt(wkt, g);
    bg::correct(g);

    bg::strategy::buffer::distance_asymmetric
    <
        typename bg::coordinate_type<Geometry>::type
    > distance_strategy(distance_left,
                        distance_right > -998 ? distance_right : distance_left);

    bg::strategy::buffer::point_circle circle_strategy(88);
    bg::strategy::buffer::side_straight side_strategy;
    test_buffer<GeometryOut>(caseid, g,
            join_strategy, end_strategy,
            distance_strategy, side_strategy, circle_strategy,
            false, expected_area,
            tolerance, &self_ip_count);
}

template
<
    typename Geometry,
    typename GeometryOut,
    typename JoinStrategy,
    typename EndStrategy,
    typename DistanceStrategy,
    typename SideStrategy,
    typename PointStrategy
>
void test_with_custom_strategies(std::string const& caseid,
        std::string const& wkt,
        JoinStrategy const& join_strategy,
        EndStrategy const& end_strategy,
        DistanceStrategy const& distance_strategy,
        SideStrategy const& side_strategy,
        PointStrategy const& point_strategy,
        double expected_area,
        double tolerance = 0.01)
{
    namespace bg = boost::geometry;
    Geometry g;
    bg::read_wkt(wkt, g);
    bg::correct(g);

    test_buffer<GeometryOut>
            (caseid, g,
            join_strategy, end_strategy,
            distance_strategy, side_strategy, point_strategy,
            true, expected_area, tolerance, NULL);
}



#endif
