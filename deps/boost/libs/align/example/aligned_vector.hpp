/*
 Copyright (c) 2014 Glen Joseph Fernandes
 glenfe at live dot com

 Distributed under the Boost Software License,
 Version 1.0. (See accompanying file LICENSE_1_0.txt
 or copy at http://boost.org/LICENSE_1_0.txt)
*/
#ifndef ALIGNED_VECTOR_HPP
#define ALIGNED_VECTOR_HPP

//[aligned_vector_hpp
/*`
 The `aligned_vector` alias template is a
 `vector` that uses `aligned_allocator` as
 the allocator type, with a configurable
 minimum alignment. It can be used with
 types that have an extended alignment or
 to specify an minimum extended alignment
 when used with any type.
*/
#include <boost/align/aligned_allocator.hpp>
#include <vector>

template<class T, std::size_t Alignment = 1>
using aligned_vector = std::vector<T,
    boost::alignment::aligned_allocator<T, Alignment> >;
//]

#endif
