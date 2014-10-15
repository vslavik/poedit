/*
 Copyright (c) 2014 Glen Joseph Fernandes
 glenfe at live dot com

 Distributed under the Boost Software License,
 Version 1.0. (See accompanying file LICENSE_1_0.txt
 or copy at http://boost.org/LICENSE_1_0.txt)
*/
#ifndef ALIGNED_PTR_HPP
#define ALIGNED_PTR_HPP

//[aligned_ptr_hpp
/*`
 The `aligned_ptr` alias template is a
 `unique_ptr` that uses `aligned_delete`
 as the deleter, for destruction and
 deallocation. This smart pointer type is
 suitable for managing objects that are
 allocated with `aligned_alloc`.
*/

#include <boost/align/aligned_delete.hpp>
#include <memory>

template<class T>
using aligned_ptr = std::unique_ptr<T,
    boost::alignment::aligned_delete>;
//]

#endif
