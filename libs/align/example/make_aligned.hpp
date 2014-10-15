/*
 Copyright (c) 2014 Glen Joseph Fernandes
 glenfe at live dot com

 Distributed under the Boost Software License,
 Version 1.0. (See accompanying file LICENSE_1_0.txt
 or copy at http://boost.org/LICENSE_1_0.txt)
*/
#ifndef MAKE_ALIGNED_HPP
#define MAKE_ALIGNED_HPP

#include <boost/align/aligned_alloc.hpp>
#include "aligned_ptr.hpp"

//[make_aligned_hpp
/*`
 The `make_aligned` function template creates
 an `aligned_ptr` for an object allocated with
 `aligned_alloc` and constructed with
 placement `new`. If allocation fails, it
 throws an object of `std::bad_alloc`. If an
 exception is thrown by the constructor, it
 uses `aligned_free` to free allocated memory
 and will rethrow the exception.
*/
template<class T, class... Args>
inline aligned_ptr<T> make_aligned(Args&&... args)
{
    auto p = boost::alignment::aligned_alloc(alignof(T),
        sizeof(T));
    if (!p) {
        throw std::bad_alloc();
    }
    try {
        auto q = ::new(p) T(std::forward<Args>(args)...);
        return aligned_ptr<T>(q);
    } catch (...) {
        boost::alignment::aligned_free(p);
        throw;
    }
}
//]

#endif
