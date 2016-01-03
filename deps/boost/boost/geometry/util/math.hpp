// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2015 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2015 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2015 Mateusz Loskot, London, UK.

// This file was modified by Oracle on 2014, 2015.
// Modifications copyright (c) 2014-2015, Oracle and/or its affiliates.

// Contributed and/or modified by Menelaos Karavelas, on behalf of Oracle
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_UTIL_MATH_HPP
#define BOOST_GEOMETRY_UTIL_MATH_HPP

#include <cmath>
#include <limits>

#include <boost/core/ignore_unused.hpp>

#include <boost/math/constants/constants.hpp>
#ifdef BOOST_GEOMETRY_SQRT_CHECK_FINITENESS
#include <boost/math/special_functions/fpclassify.hpp>
#endif // BOOST_GEOMETRY_SQRT_CHECK_FINITENESS
#include <boost/math/special_functions/round.hpp>
#include <boost/numeric/conversion/cast.hpp>
#include <boost/type_traits/is_fundamental.hpp>

#include <boost/geometry/util/select_most_precise.hpp>

namespace boost { namespace geometry
{

namespace math
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail
{

template <typename T>
inline T const& greatest(T const& v1, T const& v2)
{
    return (std::max)(v1, v2);
}

template <typename T>
inline T const& greatest(T const& v1, T const& v2, T const& v3)
{
    return (std::max)(greatest(v1, v2), v3);
}

template <typename T>
inline T const& greatest(T const& v1, T const& v2, T const& v3, T const& v4)
{
    return (std::max)(greatest(v1, v2, v3), v4);
}

template <typename T>
inline T const& greatest(T const& v1, T const& v2, T const& v3, T const& v4, T const& v5)
{
    return (std::max)(greatest(v1, v2, v3, v4), v5);
}


template <typename T,
          bool IsFloatingPoint = boost::is_floating_point<T>::value>
struct abs
{
    static inline T apply(T const& value)
    {
        T const zero = T();
        return value < zero ? -value : value;
    }
};

template <typename T>
struct abs<T, true>
{
    static inline T apply(T const& value)
    {
        return fabs(value);
    }
};


struct equals_default_policy
{
    template <typename T>
    static inline T apply(T const& a, T const& b)
    {
        // See http://www.parashift.com/c++-faq-lite/newbie.html#faq-29.17
        return greatest(abs<T>::apply(a), abs<T>::apply(b), T(1));
    }
};

template <typename T,
          bool IsFloatingPoint = boost::is_floating_point<T>::value>
struct equals_factor_policy
{
    equals_factor_policy()
        : factor(1) {}
    explicit equals_factor_policy(T const& v)
        : factor(greatest(abs<T>::apply(v), T(1)))
    {}
    equals_factor_policy(T const& v0, T const& v1, T const& v2, T const& v3)
        : factor(greatest(abs<T>::apply(v0), abs<T>::apply(v1),
                          abs<T>::apply(v2), abs<T>::apply(v3),
                          T(1)))
    {}

    T const& apply(T const&, T const&) const
    {
        return factor;
    }

    T factor;
};

template <typename T>
struct equals_factor_policy<T, false>
{
    equals_factor_policy() {}
    explicit equals_factor_policy(T const&) {}
    equals_factor_policy(T const& , T const& , T const& , T const& ) {}

    static inline T apply(T const&, T const&)
    {
        return T(1);
    }
};

template <typename Type,
          bool IsFloatingPoint = boost::is_floating_point<Type>::value>
struct equals
{
    template <typename Policy>
    static inline bool apply(Type const& a, Type const& b, Policy const&)
    {
        return a == b;
    }
};

template <typename Type>
struct equals<Type, true>
{
    template <typename Policy>
    static inline bool apply(Type const& a, Type const& b, Policy const& policy)
    {
        boost::ignore_unused(policy);

        if (a == b)
        {
            return true;
        }

        return abs<Type>::apply(a - b) <= std::numeric_limits<Type>::epsilon() * policy.apply(a, b);
    }
};

template <typename T1, typename T2, typename Policy>
inline bool equals_by_policy(T1 const& a, T2 const& b, Policy const& policy)
{
    return detail::equals
        <
            typename select_most_precise<T1, T2>::type
        >::apply(a, b, policy);
}

template <typename Type,
          bool IsFloatingPoint = boost::is_floating_point<Type>::value>
struct smaller
{
    static inline bool apply(Type const& a, Type const& b)
    {
        return a < b;
    }
};

template <typename Type>
struct smaller<Type, true>
{
    static inline bool apply(Type const& a, Type const& b)
    {
        if (equals<Type, true>::apply(a, b, equals_default_policy()))
        {
            return false;
        }
        return a < b;
    }
};


template <typename Type,
          bool IsFloatingPoint = boost::is_floating_point<Type>::value>
struct equals_with_epsilon
    : public equals<Type, IsFloatingPoint>
{};

template
<
    typename T,
    bool IsFundemantal = boost::is_fundamental<T>::value /* false */
>
struct square_root
{
    typedef T return_type;

    static inline T apply(T const& value)
    {
        // for non-fundamental number types assume that sqrt is
        // defined either:
        // 1) at T's scope, or
        // 2) at global scope, or
        // 3) in namespace std
        using ::sqrt;
        using std::sqrt;

        return sqrt(value);
    }
};

template <typename FundamentalFP>
struct square_root_for_fundamental_fp
{
    typedef FundamentalFP return_type;

    static inline FundamentalFP apply(FundamentalFP const& value)
    {
#ifdef BOOST_GEOMETRY_SQRT_CHECK_FINITENESS
        // This is a workaround for some 32-bit platforms.
        // For some of those platforms it has been reported that
        // std::sqrt(nan) and/or std::sqrt(-nan) returns a finite value.
        // For those platforms we need to define the macro
        // BOOST_GEOMETRY_SQRT_CHECK_FINITENESS so that the argument
        // to std::sqrt is checked appropriately before passed to std::sqrt
        if (boost::math::isfinite(value))
        {
            return std::sqrt(value);
        }
        else if (boost::math::isinf(value) && value < 0)
        {
            return -std::numeric_limits<FundamentalFP>::quiet_NaN();
        }
        return value;
#else
        // for fundamental floating point numbers use std::sqrt
        return std::sqrt(value);
#endif // BOOST_GEOMETRY_SQRT_CHECK_FINITENESS
    }
};

template <>
struct square_root<float, true>
    : square_root_for_fundamental_fp<float>
{
};

template <>
struct square_root<double, true>
    : square_root_for_fundamental_fp<double>
{
};

template <>
struct square_root<long double, true>
    : square_root_for_fundamental_fp<long double>
{
};

template <typename T>
struct square_root<T, true>
{
    typedef double return_type;

    static inline double apply(T const& value)
    {
        // for all other fundamental number types use also std::sqrt
        //
        // Note: in C++98 the only other possibility is double;
        //       in C++11 there are also overloads for integral types;
        //       this specialization works for those as well.
        return square_root_for_fundamental_fp
            <
                double
            >::apply(boost::numeric_cast<double>(value));
    }
};


/*!
\brief Short construct to enable partial specialization for PI, currently not possible in Math.
*/
template <typename T>
struct define_pi
{
    static inline T apply()
    {
        // Default calls Boost.Math
        return boost::math::constants::pi<T>();
    }
};

template <typename T>
struct relaxed_epsilon
{
    static inline T apply(const T& factor)
    {
        return factor * std::numeric_limits<T>::epsilon();
    }
};

// ItoF ItoI FtoF
template <typename Result, typename Source,
          bool ResultIsInteger = std::numeric_limits<Result>::is_integer,
          bool SourceIsInteger = std::numeric_limits<Source>::is_integer>
struct round
{
    static inline Result apply(Source const& v)
    {
        return boost::numeric_cast<Result>(v);
    }
};

// FtoI
template <typename Result, typename Source>
struct round<Result, Source, true, false>
{
    static inline Result apply(Source const& v)
    {
        namespace bmp = boost::math::policies;
        // ignore rounding errors for backward compatibility
        typedef bmp::policy< bmp::rounding_error<bmp::ignore_error> > policy;
        return boost::numeric_cast<Result>(boost::math::round(v, policy()));
    }
};

} // namespace detail
#endif


template <typename T>
inline T pi() { return detail::define_pi<T>::apply(); }

template <typename T>
inline T relaxed_epsilon(T const& factor)
{
    return detail::relaxed_epsilon<T>::apply(factor);
}


// Maybe replace this by boost equals or boost ublas numeric equals or so

/*!
    \brief returns true if both arguments are equal.
    \ingroup utility
    \param a first argument
    \param b second argument
    \return true if a == b
    \note If both a and b are of an integral type, comparison is done by ==.
    If one of the types is floating point, comparison is done by abs and
    comparing with epsilon. If one of the types is non-fundamental, it might
    be a high-precision number and comparison is done using the == operator
    of that class.
*/

template <typename T1, typename T2>
inline bool equals(T1 const& a, T2 const& b)
{
    return detail::equals
        <
            typename select_most_precise<T1, T2>::type
        >::apply(a, b, detail::equals_default_policy());
}

template <typename T1, typename T2>
inline bool equals_with_epsilon(T1 const& a, T2 const& b)
{
    return detail::equals_with_epsilon
        <
            typename select_most_precise<T1, T2>::type
        >::apply(a, b, detail::equals_default_policy());
}

template <typename T1, typename T2>
inline bool smaller(T1 const& a, T2 const& b)
{
    return detail::smaller
        <
            typename select_most_precise<T1, T2>::type
        >::apply(a, b);
}

template <typename T1, typename T2>
inline bool larger(T1 const& a, T2 const& b)
{
    return detail::smaller
        <
            typename select_most_precise<T1, T2>::type
        >::apply(b, a);
}



double const d2r = geometry::math::pi<double>() / 180.0;
double const r2d = 1.0 / d2r;

/*!
    \brief Calculates the haversine of an angle
    \ingroup utility
    \note See http://en.wikipedia.org/wiki/Haversine_formula
    haversin(alpha) = sin2(alpha/2)
*/
template <typename T>
inline T hav(T const& theta)
{
    T const half = T(0.5);
    T const sn = sin(half * theta);
    return sn * sn;
}

/*!
\brief Short utility to return the square
\ingroup utility
\param value Value to calculate the square from
\return The squared value
*/
template <typename T>
inline T sqr(T const& value)
{
    return value * value;
}

/*!
\brief Short utility to return the square root
\ingroup utility
\param value Value to calculate the square root from
\return The square root value
*/
template <typename T>
inline typename detail::square_root<T>::return_type
sqrt(T const& value)
{
    return detail::square_root
        <
            T, boost::is_fundamental<T>::value
        >::apply(value);
}

/*!
\brief Short utility to workaround gcc/clang problem that abs is converting to integer
       and that older versions of MSVC does not support abs of long long...
\ingroup utility
*/
template<typename T>
inline T abs(T const& value)
{
    return detail::abs<T>::apply(value);
}

/*!
\brief Short utility to calculate the sign of a number: -1 (negative), 0 (zero), 1 (positive)
\ingroup utility
*/
template <typename T>
static inline int sign(T const& value)
{
    T const zero = T();
    return value > zero ? 1 : value < zero ? -1 : 0;
}

/*!
\brief Short utility to calculate the rounded value of a number.
\ingroup utility
\note If the source T is NOT an integral type and Result is an integral type
      the value is rounded towards the closest integral value. Otherwise it's
      casted.
*/
template <typename Result, typename T>
inline Result round(T const& v)
{
    return detail::round<Result, T>::apply(v);
}

} // namespace math


}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_UTIL_MATH_HPP
