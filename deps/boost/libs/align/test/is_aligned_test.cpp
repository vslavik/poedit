/*
 Copyright (c) 2014 Glen Joseph Fernandes
 glenfe at live dot com

 Distributed under the Boost Software License,
 Version 1.0. (See accompanying file LICENSE_1_0.txt
 or copy at http://boost.org/LICENSE_1_0.txt)
*/
#include <boost/align/is_aligned.hpp>
#include <boost/core/lightweight_test.hpp>
#include <cstddef>

template<std::size_t Alignment>
void test()
{
    char s[Alignment + Alignment];
    char* b = s;
    while (!boost::alignment::is_aligned(Alignment, b)) {
        b++;
    }
    {
        void* p = &b[1];
        BOOST_TEST(!boost::alignment::is_aligned(Alignment, p));
    }
    {
        void* p = &b[Alignment];
        BOOST_TEST(boost::alignment::is_aligned(Alignment, p));
    }
    {
        void* p = b;
        BOOST_TEST(boost::alignment::is_aligned(1, p));
    }
}

int main()
{
    test<2>();
    test<4>();
    test<8>();
    test<16>();
    test<32>();
    test<64>();
    test<128>();

    return boost::report_errors();
}
