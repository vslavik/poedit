// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2014-2015, Oracle and/or its affiliates.

// Contributed and/or modified by Menelaos Karavelas, on behalf of Oracle

// Licensed under the Boost Software License version 1.0.
// http://www.boost.org/users/license.html

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_IS_VALID_POINTLIKE_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_IS_VALID_POINTLIKE_HPP

#include <boost/range.hpp>

#include <boost/geometry/core/tags.hpp>

#include <boost/geometry/algorithms/validity_failure_type.hpp>
#include <boost/geometry/algorithms/dispatch/is_valid.hpp>


namespace boost { namespace geometry
{



#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{

// A point is always simple
template <typename Point>
struct is_valid<Point, point_tag>
{
    template <typename VisitPolicy>
    static inline bool apply(Point const&, VisitPolicy& visitor)
    {
        return visitor.template apply<no_failure>();
    }
};



// A MultiPoint is simple if no two Points in the MultiPoint are equal
// (have identical coordinate values in X and Y)
//
// Reference: OGC 06-103r4 (6.1.5)
template <typename MultiPoint, bool AllowEmptyMultiGeometries>
struct is_valid<MultiPoint, multi_point_tag, AllowEmptyMultiGeometries>
{
    template <typename VisitPolicy>
    static inline bool apply(MultiPoint const& multipoint,
                             VisitPolicy& visitor)
    {
        if (AllowEmptyMultiGeometries || boost::size(multipoint) > 0)
        {
            // we allow empty multi-geometries, so an empty multipoint
            // is considered valid
            return visitor.template apply<no_failure>();
        }
        else
        {
            // we do not allow an empty multipoint
            return visitor.template apply<failure_few_points>();
        }
    }
};


} // namespace dispatch
#endif // DOXYGEN_NO_DISPATCH


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_IS_VALID_POINTLIKE_HPP
