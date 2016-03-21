
//          Copyright Oliver Kowalke 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_COROUTINES2_DETAIL_PULL_CONTROL_BLOCK_HPP
#define BOOST_COROUTINES2_DETAIL_PULL_CONTROL_BLOCK_HPP

#include <exception>
#include <type_traits>

#include <boost/config.hpp>
#include <boost/context/execution_context.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace coroutines2 {
namespace detail {

template< typename T >
struct pull_coroutine< T >::control_block {
    typename push_coroutine< T >::control_block                 *   other;
    boost::context::execution_context                               ctx;
    bool                                                            preserve_fpu;
    int                                                             state;
    std::exception_ptr                                              except;
    bool                                                            bvalid;
    typename std::aligned_storage< sizeof( T), alignof( T) >::type  storage[1];

    template< typename StackAllocator, typename Fn >
    control_block( context::preallocated, StackAllocator, Fn &&, bool);

    explicit control_block( typename push_coroutine< T >::control_block *, boost::context::execution_context const&);

    ~control_block();

    control_block( control_block &) = delete;
    control_block & operator=( control_block &) = delete;

    void resume();

    void set( T *);

    T & get();

    bool valid() const noexcept;
};

template< typename T >
struct pull_coroutine< T & >::control_block {
    typename push_coroutine< T & >::control_block   *   other;
    boost::context::execution_context                   ctx;
    bool                                                preserve_fpu;
    int                                                 state;
    std::exception_ptr                                  except;
    T                                               *   t;

    template< typename StackAllocator, typename Fn >
    control_block( context::preallocated, StackAllocator, Fn &&, bool);

    explicit control_block( typename push_coroutine< T & >::control_block *, boost::context::execution_context const&);

    ~control_block();

    control_block( control_block &) = delete;
    control_block & operator=( control_block &) = delete;

    void resume();

    T & get();

    bool valid() const noexcept;
};

struct pull_coroutine< void >::control_block {
    push_coroutine< void >::control_block  *    other;
    boost::context::execution_context           ctx;
    bool                                        preserve_fpu;
    int                                         state;
    std::exception_ptr                          except;

    template< typename StackAllocator, typename Fn >
    control_block( context::preallocated, StackAllocator, Fn &&, bool);

    explicit control_block( push_coroutine< void >::control_block *, boost::context::execution_context const&);

    ~control_block();

    control_block( control_block &) = delete;
    control_block & operator=( control_block &) = delete;

    void resume();

    bool valid() const noexcept;
};

}}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_COROUTINES2_DETAIL_PULL_CONTROL_BLOCK_HPP
