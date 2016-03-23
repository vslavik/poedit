
//          Copyright Oliver Kowalke 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_COROUTINES2_DETAIL_PUSH_CONTROL_BLOCK_IPP
#define BOOST_COROUTINES2_DETAIL_PUSH_CONTROL_BLOCK_IPP

#include <algorithm>
#include <exception>
#include <memory>

#include <boost/assert.hpp>
#include <boost/config.hpp>

#include <boost/context/execution_context.hpp>

#include <boost/coroutine2/detail/config.hpp>
#include <boost/coroutine2/detail/forced_unwind.hpp>
#include <boost/coroutine2/detail/state.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace coroutines2 {
namespace detail {

// push_coroutine< T >

template< typename T >
template< typename StackAllocator, typename Fn >
push_coroutine< T >::control_block::control_block( context::preallocated palloc, StackAllocator salloc,
                                                   Fn && fn_, bool preserve_fpu_) :
    other( nullptr),
    ctx( std::allocator_arg, palloc, salloc,
         [=,fn=std::forward< Fn >( fn_),ctx=boost::context::execution_context::current()] (void *) mutable -> void {
            // create synthesized pull_coroutine< T >
            typename pull_coroutine< T >::control_block synthesized_cb( this, ctx);
            pull_coroutine< T > synthesized( & synthesized_cb);
            other = & synthesized_cb;
            // jump back to ctor
            T * t = reinterpret_cast< T * >( ctx( nullptr, preserve_fpu) );
            // set transferred value
            synthesized_cb.set( t);
            try {
                // call coroutine-fn with synthesized pull_coroutine as argument
                fn( synthesized);
            } catch ( forced_unwind const&) {
                // do nothing for unwinding exception
            } catch (...) {
                // store other exceptions in exception-pointer
                except = std::current_exception();
            }
            // set termination flags
            state |= static_cast< int >( state_t::complete);
            // jump back to ctx
            other->ctx( nullptr, preserve_fpu);
            BOOST_ASSERT_MSG( false, "push_coroutine is complete");
         }),
    preserve_fpu( preserve_fpu_),
    state( static_cast< int >( state_t::unwind) ),
    except() {
    // enter coroutine-fn in order to get other set
    ctx( nullptr, preserve_fpu);
}

template< typename T >
push_coroutine< T >::control_block::control_block( typename pull_coroutine< T >::control_block * cb,
                                                   boost::context::execution_context const& ctx_) :
    other( cb),
    ctx( ctx_),
    preserve_fpu( other->preserve_fpu),
    state( 0),
    except() {
}

template< typename T >
push_coroutine< T >::control_block::~control_block() {
    if ( 0 == ( state & static_cast< int >( state_t::complete ) ) &&
         0 != ( state & static_cast< int >( state_t::unwind) ) ) {
        // set early-exit flag
        state |= static_cast< int >( state_t::early_exit);
        ctx( nullptr, preserve_fpu);
    }
}

template< typename T >
void
push_coroutine< T >::control_block::resume( T const& t) {
    other->ctx = boost::context::execution_context::current();
    // pass an pointer to other context
    ctx( const_cast< T * >( & t), preserve_fpu);
    if ( except) {
        std::rethrow_exception( except);
    }
    // test early-exit-flag
    if ( 0 != ( ( other->state) & static_cast< int >( state_t::early_exit) ) ) {
        throw forced_unwind();
    }
}

template< typename T >
void
push_coroutine< T >::control_block::resume( T && t) {
    other->ctx = boost::context::execution_context::current();
    // pass an pointer to other context
    ctx( std::addressof( t), preserve_fpu);
    if ( except) {
        std::rethrow_exception( except);
    }
    // test early-exit-flag
    if ( 0 != ( ( other->state) & static_cast< int >( state_t::early_exit) ) ) {
        throw forced_unwind();
    }
}

template< typename T >
bool
push_coroutine< T >::control_block::valid() const noexcept {
    return 0 == ( state & static_cast< int >( state_t::complete) );
}


// push_coroutine< T & >

template< typename T >
template< typename StackAllocator, typename Fn >
push_coroutine< T & >::control_block::control_block( context::preallocated palloc, StackAllocator salloc,
                                                     Fn && fn_, bool preserve_fpu_) :
    other( nullptr),
    ctx( std::allocator_arg, palloc, salloc,
         [=,fn=std::forward< Fn >( fn_),ctx=boost::context::execution_context::current()] (void *) mutable -> void {
            // create synthesized pull_coroutine< T >
            typename pull_coroutine< T & >::control_block synthesized_cb( this, ctx);
            pull_coroutine< T & > synthesized( & synthesized_cb);
            other = & synthesized_cb;
            // jump back to ctor
            T * t = reinterpret_cast< T * >( ctx( nullptr, preserve_fpu) );
            // set transferred value
            synthesized_cb.t = t;
            try {
                // call coroutine-fn with synthesized pull_coroutine as argument
                fn( synthesized);
            } catch ( forced_unwind const&) {
                // do nothing for unwinding exception
            } catch (...) {
                // store other exceptions in exception-pointer
                except = std::current_exception();
            }
            // set termination flags
            state |= static_cast< int >( state_t::complete);
            // jump back to ctx
            other->ctx( nullptr, preserve_fpu);
            BOOST_ASSERT_MSG( false, "push_coroutine is complete");
         }),
    preserve_fpu( preserve_fpu_),
    state( static_cast< int >( state_t::unwind) ),
    except() {
    // enter coroutine-fn in order to get other set
    ctx( nullptr, preserve_fpu);
}

template< typename T >
push_coroutine< T & >::control_block::control_block( typename pull_coroutine< T & >::control_block * cb,
                                                     boost::context::execution_context const& ctx_) :
    other( cb),
    ctx( ctx_),
    preserve_fpu( other->preserve_fpu),
    state( 0),
    except() {
}

template< typename T >
push_coroutine< T & >::control_block::~control_block() {
    if ( 0 == ( state & static_cast< int >( state_t::complete ) ) &&
         0 != ( state & static_cast< int >( state_t::unwind) ) ) {
        // set early-exit flag
        state |= static_cast< int >( state_t::early_exit);
        ctx( nullptr, preserve_fpu);
    }
}

template< typename T >
void
push_coroutine< T & >::control_block::resume( T & t) {
    other->ctx = boost::context::execution_context::current();
    // pass an pointer to other context
    ctx( const_cast< typename std::remove_const< T >::type * >( std::addressof( t) ), preserve_fpu);
    if ( except) {
        std::rethrow_exception( except);
    }
    // test early-exit-flag
    if ( 0 != ( ( other->state) & static_cast< int >( state_t::early_exit) ) ) {
        throw forced_unwind();
    }
}

template< typename T >
bool
push_coroutine< T & >::control_block::valid() const noexcept {
    return 0 == ( state & static_cast< int >( state_t::complete) );
}


// push_coroutine< void >

template< typename StackAllocator, typename Fn >
push_coroutine< void >::control_block::control_block( context::preallocated palloc, StackAllocator salloc, Fn && fn_, bool preserve_fpu_) :
    other( nullptr),
    ctx( std::allocator_arg, palloc, salloc,
         [=,fn=std::forward< Fn >( fn_),ctx=boost::context::execution_context::current()] (void *) mutable -> void {
            // create synthesized pull_coroutine< T >
            typename pull_coroutine< void >::control_block synthesized_cb( this, ctx);
            pull_coroutine< void > synthesized( & synthesized_cb);
            other = & synthesized_cb;
            // jump back to ctor
            ctx( nullptr, preserve_fpu);
            try {
                // call coroutine-fn with synthesized pull_coroutine as argument
                fn( synthesized);
            } catch ( forced_unwind const&) {
                // do nothing for unwinding exception
            } catch (...) {
                // store other exceptions in exception-pointer
                except = std::current_exception();
            }
            // set termination flags
            state |= static_cast< int >( state_t::complete);
            // jump back to ctx
            other->ctx( nullptr, preserve_fpu);
            BOOST_ASSERT_MSG( false, "push_coroutine is complete");
         }),
    preserve_fpu( preserve_fpu_),
    state( static_cast< int >( state_t::unwind) ),
    except() {
    // enter coroutine-fn in order to get other set
    ctx( nullptr, preserve_fpu);
}

inline
push_coroutine< void >::control_block::control_block( pull_coroutine< void >::control_block * cb,
                                                      boost::context::execution_context const& ctx_) :
    other( cb),
    ctx( ctx_),
    preserve_fpu( other->preserve_fpu),
    state( 0),
    except() {
}

inline
push_coroutine< void >::control_block::~control_block() {
    if ( 0 == ( state & static_cast< int >( state_t::complete ) ) &&
         0 != ( state & static_cast< int >( state_t::unwind) ) ) {
        // set early-exit flag
        state |= static_cast< int >( state_t::early_exit);
        ctx( nullptr, preserve_fpu);
    }
}

inline
void
push_coroutine< void >::control_block::resume() {
    other->ctx = boost::context::execution_context::current();
    ctx( nullptr, preserve_fpu);
    if ( except) {
        std::rethrow_exception( except);
    }
    // test early-exit-flag
    if ( 0 != ( ( other->state) & static_cast< int >( state_t::early_exit) ) ) {
        throw forced_unwind();
    }
}

inline
bool
push_coroutine< void >::control_block::valid() const noexcept {
    return 0 == ( state & static_cast< int >( state_t::complete) );
}

}}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_COROUTINES2_DETAIL_PUSH_CONTROL_BLOCK_IPP
