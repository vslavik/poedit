// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_ENRICHMENT_INFO_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_ENRICHMENT_INFO_HPP


namespace boost { namespace geometry
{


#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace overlay
{


/*!
\brief Keeps info to enrich intersection info (per source)
\details Class to keep information necessary for traversal phase (a phase
    of the overlay process). The information is gathered during the
    enrichment phase
 */
template<typename P>
struct enrichment_info
{
    inline enrichment_info()
        : travels_to_vertex_index(-1)
        , travels_to_ip_index(-1)
        , next_ip_index(-1)
    {}

    // vertex to which is free travel after this IP,
    // so from "segment_index+1" to "travels_to_vertex_index", without IP-s,
    // can be -1
    signed_index_type travels_to_vertex_index;

    // same but now IP index, so "next IP index" but not on THIS segment
    int travels_to_ip_index;

    // index of next IP on this segment, -1 if there is no one
    int next_ip_index;
};


}} // namespace detail::overlay
#endif //DOXYGEN_NO_DETAIL



}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_ENRICHMENT_INFO_HPP
