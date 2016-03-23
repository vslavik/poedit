/*
(c) 2015 Glen Joseph Fernandes
<glenjofe -at- gmail.com>

Distributed under the Boost Software
License, Version 1.0.
http://boost.org/LICENSE_1_0.txt
*/
#include <boost/align/assume_aligned.hpp>
#include <boost/core/lightweight_test.hpp>
#include <boost/align/is_aligned.hpp>

void test()
{
    char s[128];
    char* p = s;
    while (!boost::alignment::is_aligned(128, p)) {
        p++;
    }
    void* q = p;
    BOOST_ALIGN_ASSUME_ALIGNED(q, 1);
    BOOST_ALIGN_ASSUME_ALIGNED(q, 2);
    BOOST_ALIGN_ASSUME_ALIGNED(q, 4);
    BOOST_ALIGN_ASSUME_ALIGNED(q, 8);
    BOOST_ALIGN_ASSUME_ALIGNED(q, 16);
    BOOST_ALIGN_ASSUME_ALIGNED(q, 32);
    BOOST_ALIGN_ASSUME_ALIGNED(q, 64);
    BOOST_ALIGN_ASSUME_ALIGNED(q, 128);
    BOOST_TEST(q == p);
}

int main()
{
    test();
    return boost::report_errors();
}
