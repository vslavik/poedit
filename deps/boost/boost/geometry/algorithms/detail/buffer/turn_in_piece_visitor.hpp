// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2012-2014 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_BUFFER_TURN_IN_PIECE_VISITOR
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_BUFFER_TURN_IN_PIECE_VISITOR


#include <boost/core/ignore_unused.hpp>

#include <boost/range.hpp>

#include <boost/geometry/arithmetic/dot_product.hpp>
#include <boost/geometry/algorithms/assign.hpp>
#include <boost/geometry/algorithms/comparable_distance.hpp>
#include <boost/geometry/algorithms/equals.hpp>
#include <boost/geometry/algorithms/expand.hpp>
#include <boost/geometry/algorithms/detail/disjoint/point_box.hpp>
#include <boost/geometry/algorithms/detail/disjoint/box_box.hpp>
#include <boost/geometry/algorithms/detail/overlay/segment_identifier.hpp>
#include <boost/geometry/algorithms/detail/overlay/get_turn_info.hpp>
#include <boost/geometry/policies/compare.hpp>
#include <boost/geometry/strategies/buffer.hpp>


namespace boost { namespace geometry
{


#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace buffer
{

struct piece_get_box
{
    template <typename Box, typename Piece>
    static inline void apply(Box& total, Piece const& piece)
    {
        geometry::expand(total, piece.robust_envelope);
    }
};

struct piece_ovelaps_box
{
    template <typename Box, typename Piece>
    static inline bool apply(Box const& box, Piece const& piece)
    {
        if (piece.type == strategy::buffer::buffered_flat_end
            || piece.type == strategy::buffer::buffered_concave)
        {
            // Turns cannot be inside a flat end (though they can be on border)
            // Neither we need to check if they are inside concave helper pieces

            // Skip all pieces not used as soon as possible
            return false;
        }

        return ! geometry::detail::disjoint::disjoint_box_box(box, piece.robust_envelope);
    }
};

struct turn_get_box
{
    template <typename Box, typename Turn>
    static inline void apply(Box& total, Turn const& turn)
    {
        geometry::expand(total, turn.robust_point);
    }
};

struct turn_ovelaps_box
{
    template <typename Box, typename Turn>
    static inline bool apply(Box const& box, Turn const& turn)
    {
        return ! geometry::detail::disjoint::disjoint_point_box(turn.robust_point, box);
    }
};


enum analyse_result
{
    analyse_unknown,
    analyse_continue,
    analyse_disjoint,
    analyse_within,
    analyse_on_original_boundary,
    analyse_on_offsetted,
    analyse_near_offsetted
};

template <typename Point>
inline bool in_box(Point const& previous,
        Point const& current, Point const& point)
{
    // Get its box (TODO: this can be prepared-on-demand later)
    typedef geometry::model::box<Point> box_type;
    box_type box;
    geometry::assign_inverse(box);
    geometry::expand(box, previous);
    geometry::expand(box, current);

    return geometry::covered_by(point, box);
}

template <typename Point, typename Turn>
inline analyse_result check_segment(Point const& previous,
        Point const& current, Turn const& turn,
        bool from_monotonic)
{
    typedef typename strategy::side::services::default_strategy
        <
            typename cs_tag<Point>::type
        >::type side_strategy;
    typedef typename geometry::coordinate_type<Point>::type coordinate_type;

    coordinate_type const twice_area
        = side_strategy::template side_value
            <
                coordinate_type,
                coordinate_type
            >(previous, current, turn.robust_point);

    if (twice_area == 0)
    {
        // Collinear, only on segment if it is covered by its bbox
        if (in_box(previous, current, turn.robust_point))
        {
            return analyse_on_offsetted;
        }
    }
    else if (twice_area < 0)
    {
        // It is in the triangle right-of the segment where the
        // segment is the hypothenusa. Check if it is close
        // (within rounding-area)
        if (twice_area * twice_area < geometry::comparable_distance(previous, current)
            && in_box(previous, current, turn.robust_point))
        {
            return analyse_near_offsetted;
        }
        else if (from_monotonic)
        {
            return analyse_within;
        }
    }
    else if (twice_area > 0 && from_monotonic)
    {
        // Left of segment
        return analyse_disjoint;
    }

    // Not monotonic, on left or right side: continue analysing
    return analyse_continue;
}


class analyse_turn_wrt_point_piece
{
public :
    template <typename Turn, typename Piece>
    static inline analyse_result apply(Turn const& turn, Piece const& piece)
    {
        typedef typename Piece::section_type section_type;
        typedef typename Turn::robust_point_type point_type;
        typedef typename geometry::coordinate_type<point_type>::type coordinate_type;

        coordinate_type const point_y = geometry::get<1>(turn.robust_point);

        typedef strategy::within::winding<point_type> strategy_type;

        typename strategy_type::state_type state;
        strategy_type strategy;
        boost::ignore_unused(strategy);
        
        for (std::size_t s = 0; s < piece.sections.size(); s++)
        {
            section_type const& section = piece.sections[s];
            // If point within vertical range of monotonic section:
            if (! section.duplicate
                && section.begin_index < section.end_index
                && point_y >= geometry::get<min_corner, 1>(section.bounding_box) - 1
                && point_y <= geometry::get<max_corner, 1>(section.bounding_box) + 1)
            {
                for (int i = section.begin_index + 1; i <= section.end_index; i++)
                {
                    point_type const& previous = piece.robust_ring[i - 1];
                    point_type const& current = piece.robust_ring[i];

                    analyse_result code = check_segment(previous, current, turn, false);
                    if (code != analyse_continue)
                    {
                        return code;
                    }

                    // Get the state (to determine it is within), we don't have
                    // to cover the on-segment case (covered above)
                    strategy.apply(turn.robust_point, previous, current, state);
                }
            }
        }

        int const code = strategy.result(state);
        if (code == 1)
        {
            return analyse_within;
        }
        else if (code == -1)
        {
            return analyse_disjoint;
        }

        // Should normally not occur - on-segment is covered
        return analyse_unknown;
    }

};

class analyse_turn_wrt_piece
{
    template <typename Point, typename Turn>
    static inline analyse_result check_helper_segment(Point const& s1,
                Point const& s2, Turn const& turn,
                bool is_original,
                Point const& offsetted)
    {
        typedef typename strategy::side::services::default_strategy
            <
                typename cs_tag<Point>::type
            >::type side_strategy;

        switch(side_strategy::apply(s1, s2, turn.robust_point))
        {
            case 1 :
                return analyse_disjoint; // left of segment
            case 0 :
                {
                    // If is collinear, either on segment or before/after
                    typedef geometry::model::box<Point> box_type;

                    box_type box;
                    geometry::assign_inverse(box);
                    geometry::expand(box, s1);
                    geometry::expand(box, s2);

                    if (geometry::covered_by(turn.robust_point, box))
                    {
                        // It is on the segment
                        if (! is_original
                            && geometry::comparable_distance(turn.robust_point, offsetted) <= 1)
                        {
                            // It is close to the offsetted-boundary, take
                            // any rounding-issues into account
                            return analyse_near_offsetted;
                        }

                        // Points on helper-segments are considered as within
                        // Points on original boundary are processed differently
                        return is_original
                            ? analyse_on_original_boundary
                            : analyse_within;
                    }

                    // It is collinear but not on the segment. Because these
                    // segments are convex, it is outside
                    // Unless the offsetted ring is collinear or concave w.r.t.
                    // helper-segment but that scenario is not yet supported
                    return analyse_disjoint;
                }
                break;
        }

        // right of segment
        return analyse_continue;
    }

    template <typename Turn, typename Piece>
    static inline analyse_result check_helper_segments(Turn const& turn, Piece const& piece)
    {
        typedef typename Turn::robust_point_type point_type;
        geometry::equal_to<point_type> comparator;

        point_type points[4];

        int helper_count = piece.robust_ring.size() - piece.offsetted_count;
        if (helper_count == 4)
        {
            for (int i = 0; i < 4; i++)
            {
                points[i] = piece.robust_ring[piece.offsetted_count + i];
            }
        }
        else if (helper_count == 3)
        {
            // Triangular piece, assign points but assign second twice
            for (int i = 0; i < 4; i++)
            {
                int index = i < 2 ? i : i - 1;
                points[i] = piece.robust_ring[piece.offsetted_count + index];
            }
        }
        else
        {
            // Some pieces (e.g. around points) do not have helper segments.
            // Others should have 3 (join) or 4 (side)
            return analyse_continue;
        }

        // First check point-equality
        point_type const& point = turn.robust_point;
        if (comparator(point, points[0]) || comparator(point, points[3]))
        {
            return analyse_on_offsetted;
        }
        if (comparator(point, points[1]) || comparator(point, points[2]))
        {
            return analyse_on_original_boundary;
        }

        // Right side of the piece
        analyse_result result
            = check_helper_segment(points[0], points[1], turn,
                    false, points[0]);
        if (result != analyse_continue)
        {
            return result;
        }

        // Left side of the piece
        result = check_helper_segment(points[2], points[3], turn,
                    false, points[3]);
        if (result != analyse_continue)
        {
            return result;
        }

        if (! comparator(points[1], points[2]))
        {
            // Side of the piece at side of original geometry
            result = check_helper_segment(points[1], points[2], turn,
                        true, point);
            if (result != analyse_continue)
            {
                return result;
            }
        }

        // We are within the \/ or |_| shaped piece, where the top is the
        // offsetted ring.
        if (! geometry::covered_by(point, piece.robust_offsetted_envelope))
        {
            // Not in offsetted-area. This makes a cheap check possible
            typedef typename strategy::side::services::default_strategy
                <
                    typename cs_tag<point_type>::type
                >::type side_strategy;

            switch(side_strategy::apply(points[3], points[0], point))
            {
                case 1 : return analyse_disjoint;
                case -1 : return analyse_within;
                case 0 : return analyse_disjoint;
            }
        }

        return analyse_continue;
    }

    template <typename Turn, typename Piece, typename Compare>
    static inline analyse_result check_monotonic(Turn const& turn, Piece const& piece, Compare const& compare)
    {
        typedef typename Piece::piece_robust_ring_type ring_type;
        typedef typename ring_type::const_iterator it_type;
        it_type end = piece.robust_ring.begin() + piece.offsetted_count;
        it_type it = std::lower_bound(piece.robust_ring.begin(),
                    end,
                    turn.robust_point,
                    compare);

        if (it != end
            && it != piece.robust_ring.begin())
        {
            // iterator points to point larger than point
            // w.r.t. specified direction, and prev points to a point smaller
            // We now know if it is inside/outside
            it_type prev = it - 1;
            return check_segment(*prev, *it, turn, true);
        }
        return analyse_continue;
    }

public :
    template <typename Turn, typename Piece>
    static inline analyse_result apply(Turn const& turn, Piece const& piece)
    {
        typedef typename Turn::robust_point_type point_type;
        analyse_result code = check_helper_segments(turn, piece);
        if (code != analyse_continue)
        {
            return code;
        }

        geometry::equal_to<point_type> comparator;

        if (piece.offsetted_count > 8)
        {
            // If the offset contains some points and is monotonic, we try
            // to avoid walking all points linearly.
            // We try it only once.
            if (piece.is_monotonic_increasing[0])
            {
                code = check_monotonic(turn, piece, geometry::less<point_type, 0>());
                if (code != analyse_continue) return code;
            }
            else if (piece.is_monotonic_increasing[1])
            {
                code = check_monotonic(turn, piece, geometry::less<point_type, 1>());
                if (code != analyse_continue) return code;
            }
            else if (piece.is_monotonic_decreasing[0])
            {
                code = check_monotonic(turn, piece, geometry::greater<point_type, 0>());
                if (code != analyse_continue) return code;
            }
            else if (piece.is_monotonic_decreasing[1])
            {
                code = check_monotonic(turn, piece, geometry::greater<point_type, 1>());
                if (code != analyse_continue) return code;
            }
        }

        // It is small or not monotonic, walk linearly through offset
        // TODO: this will be combined with winding strategy

        for (int i = 1; i < piece.offsetted_count; i++)
        {
            point_type const& previous = piece.robust_ring[i - 1];
            point_type const& current = piece.robust_ring[i];

            // The robust ring can contain duplicates
            // (on which any side or side-value would return 0)
            if (! comparator(previous, current))
            {
                code = check_segment(previous, current, turn, false);
                if (code != analyse_continue)
                {
                    return code;
                }
            }
        }

        return analyse_unknown;
    }

};


template <typename Turns, typename Pieces>
class turn_in_piece_visitor
{
    Turns& m_turns; // because partition is currently operating on const input only
    Pieces const& m_pieces; // to check for piece-type

    template <typename Operation, typename Piece>
    inline bool skip(Operation const& op, Piece const& piece) const
    {
        if (op.piece_index == piece.index)
        {
            return true;
        }
        Piece const& pc = m_pieces[op.piece_index];
        if (pc.left_index == piece.index || pc.right_index == piece.index)
        {
            if (pc.type == strategy::buffer::buffered_flat_end)
            {
                // If it is a flat end, don't compare against its neighbor:
                // it will always be located on one of the helper segments
                return true;
            }
            if (pc.type == strategy::buffer::buffered_concave)
            {
                // If it is concave, the same applies: the IP will be
                // located on one of the helper segments
                return true;
            }
        }

        return false;
    }


public:

    inline turn_in_piece_visitor(Turns& turns, Pieces const& pieces)
        : m_turns(turns)
        , m_pieces(pieces)
    {}

    template <typename Turn, typename Piece>
    inline void apply(Turn const& turn, Piece const& piece, bool first = true)
    {
        boost::ignore_unused_variable_warning(first);

        if (turn.count_within > 0)
        {
            // Already inside - no need to check again
            return;
        }

        if (piece.type == strategy::buffer::buffered_flat_end
            || piece.type == strategy::buffer::buffered_concave)
        {
            // Turns cannot be located within flat-end or concave pieces
            return;
        }

        if (! geometry::covered_by(turn.robust_point, piece.robust_envelope))
        {
            // Easy check: if the turn is not in the envelope, we can safely return
            return;
        }

        if (skip(turn.operations[0], piece) || skip(turn.operations[1], piece))
        {
            return;
        }

        // TODO: mutable_piece to make some on-demand preparations in analyse
        analyse_result analyse_code =
            piece.type == geometry::strategy::buffer::buffered_point
                ? analyse_turn_wrt_point_piece::apply(turn, piece)
                : analyse_turn_wrt_piece::apply(turn, piece);

        Turn& mutable_turn = m_turns[turn.turn_index];
        switch(analyse_code)
        {
            case analyse_disjoint :
                return;
            case analyse_on_offsetted :
                mutable_turn.count_on_offsetted++; // value is not used anymore
                return;
            case analyse_on_original_boundary :
                mutable_turn.count_on_original_boundary++;
                return;
            case analyse_within :
                mutable_turn.count_within++;
                return;
            case analyse_near_offsetted :
                mutable_turn.count_within_near_offsetted++;
                return;
            default :
                break;
        }

        // TODO: this point_in_geometry is a performance-bottleneck here and
        // will be replaced completely by extending analyse_piece functionality
        int geometry_code = detail::within::point_in_geometry(turn.robust_point, piece.robust_ring);

        if (geometry_code == 1)
        {
            mutable_turn.count_within++;
        }
    }
};


}} // namespace detail::buffer
#endif // DOXYGEN_NO_DETAIL


}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_BUFFER_TURN_IN_PIECE_VISITOR
