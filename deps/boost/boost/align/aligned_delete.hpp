/*
 (c) 2014 Glen Joseph Fernandes
 glenjofe at gmail dot com

 Distributed under the Boost Software
 License, Version 1.0.
 http://boost.org/LICENSE_1_0.txt
*/
#ifndef BOOST_ALIGN_ALIGNED_DELETE_HPP
#define BOOST_ALIGN_ALIGNED_DELETE_HPP

#include <boost/config.hpp>
#include <boost/align/aligned_alloc.hpp>
#include <boost/align/aligned_delete_forward.hpp>

namespace boost {
    namespace alignment {
        class aligned_delete {
        public:
            template<class T>
            void operator()(T* ptr) const
                BOOST_NOEXCEPT_IF(BOOST_NOEXCEPT_EXPR(ptr->~T())) {
                if (ptr) {
                    ptr->~T();
                    alignment::aligned_free(ptr);
                }
            }
        };
    }
}

#endif
