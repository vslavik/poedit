/*=============================================================================
    Copyright (c) 2001-2007 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#include <boost/detail/lightweight_test.hpp>
#include <boost/fusion/adapted/struct/adapt_struct.hpp>
#include <boost/fusion/sequence/intrinsic/at.hpp>
#include <boost/fusion/sequence/intrinsic/size.hpp>
#include <boost/fusion/sequence/intrinsic/empty.hpp>
#include <boost/fusion/sequence/intrinsic/front.hpp>
#include <boost/fusion/sequence/intrinsic/back.hpp>
#include <boost/fusion/sequence/intrinsic/value_at.hpp>
#include <boost/fusion/sequence/io/out.hpp>
#include <boost/fusion/container/vector/vector.hpp>
#include <boost/fusion/container/list/list.hpp>
#include <boost/fusion/container/generation/make_vector.hpp>
#include <boost/fusion/container/vector/convert.hpp>
#include <boost/fusion/sequence/comparison/equal_to.hpp>
#include <boost/fusion/sequence/comparison/not_equal_to.hpp>
#include <boost/fusion/sequence/comparison/less.hpp>
#include <boost/fusion/sequence/comparison/less_equal.hpp>
#include <boost/fusion/sequence/comparison/greater.hpp>
#include <boost/fusion/sequence/comparison/greater_equal.hpp>
#include <boost/fusion/mpl.hpp>
#include <boost/fusion/support/is_view.hpp>
#include <boost/mpl/front.hpp>
#include <boost/mpl/is_sequence.hpp>
#include <boost/mpl/assert.hpp>
#include <boost/static_assert.hpp>
#include <iostream>
#include <string>

namespace ns
{
    struct point
    {
        int x;
        int y;
        int z;
    };

#if !BOOST_WORKAROUND(__GNUC__,<4)
    struct point_with_private_attributes
    {
        friend struct boost::fusion::extension::access;

    private:
        int x;
        int y;
        int z;

    public:
        point_with_private_attributes(int x, int y, int z):x(x),y(y),z(z)
        {}
    };
#endif

    struct foo
    {
        int x;
    };
    
    struct bar
    {
        foo foo_;
        int y;
    };
}

#if BOOST_PP_VARIADICS

    BOOST_FUSION_ADAPT_STRUCT(
        ns::point,
        x,
        y,
        z
    )

#   if !BOOST_WORKAROUND(__GNUC__,<4)
    BOOST_FUSION_ADAPT_STRUCT(
        ns::point_with_private_attributes,
        x,
        y,
        z
    )
#   endif

    struct s { int m; };
    BOOST_FUSION_ADAPT_STRUCT(s, m)

    BOOST_FUSION_ADAPT_STRUCT(
        ns::bar,
        foo_.x, // test that adapted members can actually be expressions
        y
    )

#else // BOOST_PP_VARIADICS

    BOOST_FUSION_ADAPT_STRUCT(
        ns::point,
        (int, x)
        (int, y)
        (BOOST_FUSION_ADAPT_AUTO, z)
    )

#   if !BOOST_WORKAROUND(__GNUC__,<4)
    BOOST_FUSION_ADAPT_STRUCT(
        ns::point_with_private_attributes,
        (int, x)
        (int, y)
        (BOOST_FUSION_ADAPT_AUTO, z)
    )
#   endif

    struct s { int m; };
    BOOST_FUSION_ADAPT_STRUCT(s, (BOOST_FUSION_ADAPT_AUTO, m))

    BOOST_FUSION_ADAPT_STRUCT(
        ns::bar,
        (BOOST_FUSION_ADAPT_AUTO, foo_.x) // test that adapted members can actually be expressions
        (BOOST_FUSION_ADAPT_AUTO, y)
    )

#endif

int
main()
{
    using namespace boost::fusion;
    using namespace boost;
    using ns::point;

    std::cout << tuple_open('[');
    std::cout << tuple_close(']');
    std::cout << tuple_delimiter(", ");

    {
        BOOST_MPL_ASSERT_NOT((traits::is_view<point>));
        point p = {123, 456, 789};

        std::cout << at_c<0>(p) << std::endl;
        std::cout << at_c<1>(p) << std::endl;
        std::cout << at_c<2>(p) << std::endl;
        std::cout << p << std::endl;
        BOOST_TEST(p == make_vector(123, 456, 789));

        at_c<0>(p) = 6;
        at_c<1>(p) = 9;
        at_c<2>(p) = 12;
        BOOST_TEST(p == make_vector(6, 9, 12));

        BOOST_STATIC_ASSERT(boost::fusion::result_of::size<point>::value == 3);
        BOOST_STATIC_ASSERT(!boost::fusion::result_of::empty<point>::value);

        BOOST_TEST(front(p) == 6);
        BOOST_TEST(back(p) == 12);
    }

    {
        vector<int, float, int> v1(4, 2, 2);
        point v2 = {5, 3, 3};
        vector<long, double, int> v3(5, 4, 4);
        BOOST_TEST(v1 < v2);
        BOOST_TEST(v1 <= v2);
        BOOST_TEST(v2 > v1);
        BOOST_TEST(v2 >= v1);
        BOOST_TEST(v2 < v3);
        BOOST_TEST(v2 <= v3);
        BOOST_TEST(v3 > v2);
        BOOST_TEST(v3 >= v2);
    }

    {
        // conversion from point to vector
        point p = {5, 3, 3};
        vector<int, long, int> v(p);
        v = p;
    }

    {
        // conversion from point to list
        point p = {5, 3, 3};
        list<int, long, int> l(p);
        l = p;
    }

    { // begin/end
        using namespace boost::fusion;
        using boost::is_same;

        typedef boost::fusion::result_of::begin<s>::type b;
        typedef boost::fusion::result_of::end<s>::type e;
        // this fails
        BOOST_MPL_ASSERT((is_same<boost::fusion::result_of::next<b>::type, e>));
    }

    {
        BOOST_MPL_ASSERT((mpl::is_sequence<ns::point>));
        BOOST_MPL_ASSERT((boost::is_same<
            boost::fusion::result_of::value_at_c<ns::point,0>::type
          , mpl::front<ns::point>::type>));
    }

#if !BOOST_WORKAROUND(__GNUC__,<4)
    {
        ns::point_with_private_attributes p(123, 456, 789);

        std::cout << at_c<0>(p) << std::endl;
        std::cout << at_c<1>(p) << std::endl;
        std::cout << at_c<2>(p) << std::endl;
        std::cout << p << std::endl;
        BOOST_TEST(p == make_vector(123, 456, 789));
    }
#endif

    {
        fusion::vector<int, float> v1(4, 2);
        ns::bar v2 = {{5}, 3};
        BOOST_TEST(v1 < v2);
        BOOST_TEST(v1 <= v2);
        BOOST_TEST(v2 > v1);
        BOOST_TEST(v2 >= v1);
    }

    return boost::report_errors();
}

