/*
 Copyright (c) 2014 Glen Joseph Fernandes
 glenfe at live dot com

 Distributed under the Boost Software License,
 Version 1.0. (See accompanying file LICENSE_1_0.txt
 or copy at http://boost.org/LICENSE_1_0.txt)
*/
#include "aligned_vector.hpp"

//[aligned_vector_cpp
/*`
 Here `aligned_vector` is used to create
 a `vector` of integers where each integer
 object has extended cache alignment.
*/
enum {
    cache_line = 64
};

int main()
{
    aligned_vector<int, cache_line> v(32);
    v[0] = 1;
}
//]
