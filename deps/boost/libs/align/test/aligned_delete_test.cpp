/*
 Copyright (c) 2014 Glen Joseph Fernandes
 glenfe at live dot com

 Distributed under the Boost Software License,
 Version 1.0. (See accompanying file LICENSE_1_0.txt
 or copy at http://boost.org/LICENSE_1_0.txt)
*/
#include <boost/align/aligned_alloc.hpp>
#include <boost/align/aligned_delete.hpp>
#include <boost/align/alignment_of.hpp>
#include <boost/core/lightweight_test.hpp>
#include <new>
#include <cstddef>

class type {
public:
    static int value;

    type() {
        value++;
    }

    ~type() {
        value--;
    }
};

int type::value = 0;

int main()
{
    {
        void* p = boost::alignment::aligned_alloc(boost::
            alignment::alignment_of<type>::value, sizeof(type));
        if (!p) {
            throw std::bad_alloc();
        }
        type* q = ::new(p) type();
        boost::alignment::aligned_delete d;
        BOOST_TEST(type::value == 1);
        d(q);
        BOOST_TEST(type::value == 0);
    }
    {
        void* p = boost::alignment::aligned_alloc(boost::
            alignment::alignment_of<int>::value, sizeof(int));
        if (!p) {
            throw std::bad_alloc();
        }
        int* q = ::new(p) int();
        boost::alignment::aligned_delete d;
        d(q);
    }

    return boost::report_errors();
}
