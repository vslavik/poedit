/*
 Copyright (c) 2014 Glen Joseph Fernandes
 glenfe at live dot com

 Distributed under the Boost Software License,
 Version 1.0. (See accompanying file LICENSE_1_0.txt
 or copy at http://boost.org/LICENSE_1_0.txt)
*/
#include "make_aligned.hpp"

//[make_aligned_cpp
/*`
 Here `make_aligned` is used to create an
 `aligned_ptr` object for a type which has
 extended alignment specified.
*/
struct alignas(16) type {
    float data[4];
};

int main()
{
    auto p = make_aligned<type>();
    p->data[0] = 1.0f;
}
//]
