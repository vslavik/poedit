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

#include <boost/foreach.hpp>
#include <geometry_test_common.hpp>


#include <boost/geometry/algorithms/envelope.hpp>
#include <boost/geometry/algorithms/area.hpp>
#include <boost/geometry/algorithms/buffer.hpp>
#include <boost/geometry/algorithms/correct.hpp>
#include <boost/geometry/algorithms/union.hpp>

#include <boost/geometry/algorithms/detail/overlay/debug_turn_info.hpp>

#include <boost/geometry/geometries/geometries.hpp>

#include <boost/geometry/strategies/strategies.hpp>

#include <boost/geometry/algorithms/disjoint.hpp>
#include <boost/geometry/algorithms/intersects.hpp>
#include <boost/geometry/algorithms/detail/overlay/self_turn_points.hpp>

#include <boost/geometry/algorithms/detail/buffer/buffer_inserter.hpp>

#include <boost/geometry/strategies/buffer.hpp>



#include <boost/geometry/io/wkt/wkt.hpp>


#if defined(TEST_WITH_SVG)

#include <boost/geometry/io/svg/svg_mapper.hpp>

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

template <typename SvgMapper, typename Tag>
struct svg_visitor
{
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
    inline void map_turns(Turns const& turns)
    {
        namespace bgdb = boost::geometry::detail::buffer;
        typedef typename boost::range_value<Turns const>::type turn_type;
        typedef typename turn_type::point_type point_type;
        typedef typename turn_type::robust_point_type robust_point_type;

        std::map<robust_point_type, int, bg::less<robust_point_type> > offsets;

        for (typename boost::range_iterator<Turns const>::type it =
            boost::begin(turns); it != boost::end(turns); ++it)
        {
            char color = 'g';
            std::string fill = "fill:rgb(0,255,0);";
            switch(it->location)
            {
                case bgdb::inside_buffer : fill = "fill:rgb(255,0,0);"; color = 'r'; break;
                case bgdb::inside_original : fill = "fill:rgb(0,0,255);"; color = 'b'; break;
            }
            if (it->blocked())
            {
                fill = "fill:rgb(128,128,128);"; color = '-';
            }

            fill += "fill-opacity:0.7;";
            std::ostringstream out;
            out << it->operations[0].piece_index << "/" << it->operations[1].piece_index
                << " " << si(it->operations[0].seg_id) << "/" << si(it->operations[1].seg_id)
                << std::endl;
            out << " " << bg::method_char(it->method)
                << ":" << bg::operation_char(it->operations[0].operation)
                << "/" << bg::operation_char(it->operations[1].operation);
            out << " " << (it->count_within > 0 ? "w" : "")
                << (it->count_on_multi > 0 ? "m" : "")
                << (it->count_on_occupied > 0 ? "o" : "")
                << (it->count_on_offsetted > 0 ? "b" : "") // b: offsetted border
                << (it->count_on_helper > 0 ? "h" : "")
                ;

            offsets[it->get_robust_point()] += 10;
            int offset = offsets[it->get_robust_point()];

            m_mapper.map(it->point, fill, 6);
            m_mapper.text(it->point, out.str(), "fill:rgb(0,0,0);font-family='Arial';font-size:9px;", 5, offset);

            offsets[it->get_robust_point()] += 25;
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
            std::copy(boost::begin(piece.helper_segments),
                    boost::end(piece.helper_segments),
                    std::back_inserter(corner));

            if (corner.empty())
            {
                continue;
            }

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
                out << piece.index << "/" << int(piece.type) << "/" << piece.first_seg_id.segment_index << ".." << piece.last_segment_index - 1;
                point_type label_point = corner.front();
                if (corner.size() >= 2)
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
        if(phase == 0)
        {
            map_pieces(collection.m_pieces, collection.offsetted_rings, true, true);
            map_turns(collection.m_turns);
        }
        if (phase == 1)
        {
//        map_traversed_rings(collection.traversed_rings);
//        map_offsetted_rings(collection.offsetted_rings);
        }
    }
};

#endif

//-----------------------------------------------------------------------------
template <typename JoinStrategy>
struct JoinTestProperties { };

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
    typename Geometry
>
void test_buffer(std::string const& caseid, Geometry const& geometry,
            JoinStrategy const& join_strategy, EndStrategy const& end_strategy,
            bool check_self_intersections, double expected_area,
            double distance_left, double distance_right,
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

    if (distance_right < -998)
    {
        distance_right = distance_left;
    }

    bg::model::box<point_type> envelope;
    bg::envelope(geometry, envelope);

    std::string join_name = JoinTestProperties<JoinStrategy>::name();
    std::string end_name = EndTestProperties<EndStrategy>::name();

    if (boost::is_same<tag, bg::point_tag>::value 
        || boost::is_same<tag, bg::multi_point_tag>::value)
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
        << (distance_left < 0 && distance_right < 0 ? "_deflate" : "")
         // << "_" << point_buffer_count
        ;

    //std::cout << complete.str() << std::endl;

    std::ostringstream filename;
    filename << "buffer_" << complete.str() << ".svg";

#if defined(TEST_WITH_SVG)
    std::ofstream svg(filename.str().c_str());
    typedef bg::svg_mapper<point_type> mapper_type;
    mapper_type mapper(svg, 1000, 1000);

    {
        double d = std::abs(distance_left) + std::abs(distance_right);

        bg::model::box<point_type> box = envelope;
        bg::buffer(box, box, d * (join_name == "miter" ? 2.0 : 1.1));
        mapper.add(box);
    }

    svg_visitor<mapper_type, tag> visitor(mapper);
#else
    bg::detail::buffer::visit_pieces_default_policy visitor;
#endif

    bg::strategy::buffer::distance_asymmetric
        <
            coordinate_type
        > 
    distance_strategy(distance_left, distance_right);

    bg::strategy::buffer::side_straight side_strategy;

    // For (multi)points a buffer with 88 points is used for testing.
    // More points will give a more precise result - expected area should be
    // adapted then
    bg::strategy::buffer::point_circle circle_strategy(88);

    typedef typename bg::point_type<Geometry>::type point_type;
    typedef typename bg::rescale_policy_type<point_type>::type
        rescale_policy_type;

    // Enlarge the box to get a proper rescale policy
    bg::buffer(envelope, envelope, distance_strategy.max_distance(join_strategy, end_strategy));

    rescale_policy_type rescale_policy
            = bg::get_rescale_policy<rescale_policy_type>(envelope);

    std::vector<GeometryOut> buffered;

    bg::detail::buffer::buffer_inserter<GeometryOut>(geometry,
                        std::back_inserter(buffered),
                        distance_strategy,
                        side_strategy,
                        join_strategy,
                        end_strategy,
                        circle_strategy,
                        rescale_policy,
                        visitor);

    typename bg::default_area_result<GeometryOut>::type area = 0;
    BOOST_FOREACH(GeometryOut const& polygon, buffered)
    {
        area += bg::area(polygon);
    }

    //std::cout << caseid << " " << distance_left << std::endl;
    //std::cout << "INPUT: " << bg::wkt(geometry) << std::endl;
    //std::cout << "OUTPUT: " << area << std::endl;
    //BOOST_FOREACH(GeometryOut const& polygon, buffered)
    //{
    //    std::cout << bg::wkt(polygon) << std::endl;
    //}


    if (expected_area > -0.1)
    {
        BOOST_CHECK_MESSAGE
            (
                bg::math::abs(area - expected_area) < tolerance,
                complete.str() << " not as expected. " 
                << std::setprecision(18)
                << " Expected: "  << expected_area
                << " Detected: "  << area
            );

        if (check_self_intersections)
        {
            // Be sure resulting polygon does not contain
            // self-intersections
            BOOST_FOREACH(GeometryOut const& polygon, buffered)
            {
                BOOST_CHECK_MESSAGE
                    (
                        ! bg::detail::overlay::has_self_intersections(polygon,
                                rescale_policy, false),
                        complete.str() << " output is self-intersecting. "
                    );
            }
        }
    }

#if defined(TEST_WITH_SVG)
    // Map input geometry in green
    mapper.map(geometry, "opacity:0.5;fill:rgb(0,128,0);stroke:rgb(0,128,0);stroke-width:10");

    BOOST_FOREACH(GeometryOut const& polygon, buffered)
    {
        mapper.map(polygon, "opacity:0.4;fill:rgb(255,255,128);stroke:rgb(0,0,0);stroke-width:3");
        post_map(polygon, mapper, rescale_policy);
    }
#endif

    if (self_ip_count != NULL)
    {
        std::size_t count = 0;
        BOOST_FOREACH(GeometryOut const& polygon, buffered)
        {
            if (bg::detail::overlay::has_self_intersections(polygon,
                    rescale_policy, false))
            {
                count += count_self_ips(polygon, rescale_policy);
            }
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

    test_buffer<GeometryOut>
            (caseid, g, join_strategy, end_strategy,
            check_self_intersections, expected_area,
            distance_left, distance_right, tolerance, NULL);
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

    test_buffer<GeometryOut>(caseid, g, join_strategy, end_strategy,
            false, expected_area,
            distance_left, distance_right, tolerance, &self_ip_count);
}


#endif
