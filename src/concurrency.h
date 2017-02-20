/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 2010-2017 Vaclav Slavik
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a
 *  copy of this software and associated documentation files (the "Software"),
 *  to deal in the Software without restriction, including without limitation
 *  the rights to use, copy, modify, merge, publish, distribute, sublicense,
 *  and/or sell copies of the Software, and to permit persons to whom the
 *  Software is furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in
 *  all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 *  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 *  DEALINGS IN THE SOFTWARE.
 *
 */

#ifndef Poedit_concurrency_h
#define Poedit_concurrency_h

#if defined(__WXOSX__) && defined(__clang__)
    #define HAVE_DISPATCH
#endif

#if defined(_MSC_VER) && (_MSC_VER >= 1800)
    #define HAVE_PPL
    #define USE_PPL_DISPATCH
#endif

#define BOOST_CHRONO_HEADER_ONLY
#define BOOST_THREAD_VERSION 4
#define BOOST_THREAD_PROVIDES_EXECUTORS

#include <boost/chrono/duration.hpp>
#include <boost/thread/executor.hpp>
#include <boost/thread/future.hpp>
#include <boost/throw_exception.hpp>

#if !defined(HAVE_DISPATCH) && !defined(USE_PPL_DISPATCH)
    #include <boost/thread/executors/basic_thread_pool.hpp>
#endif

#if defined(HAVE_PPL)
    #if defined(_MSC_VER)
        #include <concrt.h>
        #include <ppltasks.h>
        namespace pplx = Concurrency;
    #else
        #include <pplx/pplxtasks.h>
    #endif
#endif

#include <memory>
#include <mutex>

#include <wx/app.h>
#include <wx/weakref.h>


namespace dispatch
{

// forward declarations

template<typename T>
class future;


// implementation details

namespace detail
{

class custom_executor : public boost::executors::executor
{
public:
    custom_executor() : m_closed(false) {}

    void close() override
    {
      std::lock_guard<std::mutex> lock(m_mutex);
      m_closed = true;
    }

    bool closed() override
    {
      std::lock_guard<std::mutex> lock(m_mutex);
      return m_closed;
    }

    bool try_executing_one() override { return false; }

private:
    std::mutex m_mutex;
    bool m_closed;
};


#if defined(HAVE_DISPATCH)

enum class queue
{
    main,
    priority_default
};

extern void dispatch_async_cxx(boost::executors::work&& f, queue q = queue::priority_default);

class background_queue_executor : public custom_executor
{
public:
    static background_queue_executor& get();

    void submit(work&& closure) override
    {
        dispatch_async_cxx(std::forward<work>(closure));
    }
};

#elif defined(USE_PPL_DISPATCH)

class background_queue_executor : public custom_executor
{
public:
    static background_queue_executor& get();

    void submit(work&& closure)
    {
        pplx::create_task([f{std::move(closure)}]() mutable { f(); });
    }
};

#else // !HAVE_DISPATCH && !USE_PPL_DISPATCH

class background_queue_executor : public boost::basic_thread_pool
{
public:
    static background_queue_executor& get();
};

#endif // HAVE_DISPATCH etc.


class main_thread_executor : public custom_executor
{
public:
    static main_thread_executor& get();

    void submit(work&& closure)
    {
#ifdef HAVE_DISPATCH
        dispatch_async_cxx(std::forward<work>(closure), queue::main);
#else
        wxTheApp->CallAfter(std::forward<work>(closure));
#endif
    }
};


// Helper exception for when then_on_window() isn't called because the wxWindow
// was already dismissed by the user
class window_dismissed : public std::exception
{
};


// Determines continuation's argument type
// (with thanks to http://stackoverflow.com/a/21486468/237188)
template<typename T>
struct argument_type : public argument_type<decltype(&T::operator())>
{
};

template<typename C, typename Ret>
struct argument_type<Ret(C::*)() const>
{
    using arg0_type = void;
};

template<typename C, typename Ret, typename... Args>
struct argument_type<Ret(C::*)(Args...) const>
{
    using arg0_type = typename std::tuple_element<0, std::tuple<Args...>>::type;
};


// Helper for calling a continuation. Unpacks futures for continuations that
// take the value as its argument and leaves it unmodified for those that take
// a future argument.
template<typename T>
struct continuation_calling_helper
{
    static T unpack_arg(boost::future<T>&& arg) { return arg.get(); }
};

template<typename T>
struct continuation_calling_helper<dispatch::future<T>>
{
    static boost::future<T> unpack_arg(boost::future<T>&& arg) { return std::move(arg); }
};

template<>
struct continuation_calling_helper<void>
{
    static void touch_arg(boost::future<void>& arg) { arg.get(); }
};


// Helper to unwrap dispatch::futures into boost::futures so that implicit
// unwrapping of boost::future<boost::future<T>> into boost::future<T> works
template<typename T>
struct future_unwrapper
{
    typedef T type;
    typedef T return_type;

    template<typename F, typename... Args>
    static return_type call_and_unwrap(F&& f, Args&&... args)
    {
        return f(std::forward<Args>(args)...);
    }
};

template<>
struct future_unwrapper<void>
{
    typedef void type;
    typedef void return_type;

    template<typename F, typename... Args>
    static void call_and_unwrap(F&& f, Args&&... args)
    {
        f(std::forward<Args>(args)...);
    }
};

template<typename T>
struct future_unwrapper<dispatch::future<T>>
{
    typedef T type;
    typedef boost::future<T> return_type;

    template<typename F, typename... Args>
    static return_type call_and_unwrap(F&& f, Args&&... args)
    {
        return f(std::forward<Args>(args)...).move_to_boost();
    }
};

template<typename F, typename... Args>
inline auto call_and_unwrap_if_future(F&& f, Args&&... args) -> typename future_unwrapper<typename std::result_of<F(Args...)>::type>::return_type
{
    return future_unwrapper<typename std::result_of<F(Args...)>::type>::call_and_unwrap(std::forward<F>(f), std::forward<Args>(args)...);
}

} // namespace detail


// ----------------------------------------------------------------------
// Tasks (aka futures)
// ----------------------------------------------------------------------

using boost::exception_ptr;
using boost::future_status;
template<typename T> using promise = boost::promise<T>;

exception_ptr current_exception();

// Can't use std::current_exception with boost::promise, must use boost
// version instead. This helper takes care of it.
template<typename T>
void set_current_exception(boost::promise<T>& pr)
{
    pr.set_exception(current_exception());
}

template<typename T>
void set_current_exception(std::shared_ptr<boost::promise<T>> pr) { set_current_exception(*pr); }


template<typename T, typename FutureType>
class future_base
{
public:
    future_base() {}
    future_base(FutureType&& future) : f_(std::move(future.m_future)) {}

    future_base(boost::future<T>&& future) : f_(std::move(future)) {}
    future_base(boost::future<boost::future<T>>&& future) : f_(future.unwrap()) {}

    FutureType& operator=(FutureType&& other) { f_ = std::move(other.f_); }
    FutureType& operator=(const FutureType& other) = delete;

    void wait() const { f_.wait(); }

    template<class Rep, class Period>
    future_status wait_for(const boost::chrono::duration<Rep,Period>& timeout_duration) const { return f_.wait_for(timeout_duration); }

    bool valid() const { return f_.valid(); }

    // Convenient async exception catching:

    template<typename Ex, typename F>
    auto catch_ex(F&& continuation) -> future<void>;

    template<typename F>
    auto catch_all(F&& continuation) -> future<void>;

    boost::future<T> move_to_boost() { return std::move(f_); }

protected:
    boost::future<T> f_;
};

/// More advanced wrapper around boost::future
template<typename T>
class future : public future_base<T, future<T>>
{
public:
    future() {}
    using future_base<T, future<T>>::future_base;

#ifdef HAVE_PPL
    future(pplx::task<T>&& task)
    {
        auto pr = std::make_shared<promise<T>>();
        this->f_ = pr->get_future();
        task.then([pr](pplx::task<T> x) {
            try
            {
                pr->set_value(x.get());
            }
            catch (...)
            {
                set_current_exception(pr);
            }
        });
    }
#endif

    T get() { return this->f_.get(); }

    template<typename F>
    auto then(F&& continuation) -> future<typename detail::future_unwrapper<typename std::result_of<F(T)>::type>::type>
    {
        typedef detail::continuation_calling_helper<typename detail::argument_type<typename std::decay<F>::type>::arg0_type> cch;
        return this->f_.then(detail::background_queue_executor::get(),
                             [f{std::move(continuation)}](boost::future<T> x){
                                 return detail::call_and_unwrap_if_future(f, cch::unpack_arg(std::move(x)));
                             });
    }

    template<typename F>
    auto then_on_main(F&& continuation) -> future<typename detail::future_unwrapper<typename std::result_of<F(T)>::type>::type>
    {
        typedef detail::continuation_calling_helper<typename detail::argument_type<typename std::decay<F>::type>::arg0_type> cch;
        return this->f_.then(detail::main_thread_executor::get(),
                             [f{std::move(continuation)}](boost::future<T> x) {
                                 return detail::call_and_unwrap_if_future(f, cch::unpack_arg(std::move(x)));
                             });
    }

    template<typename Window, typename F>
    auto then_on_window(Window *self, F&& continuation) -> future<typename detail::future_unwrapper<typename std::result_of<F(T)>::type>::type>
    {
        typedef detail::continuation_calling_helper<typename detail::argument_type<typename std::decay<F>::type>::arg0_type> cch;
        wxWeakRef<Window> weak(self);
        return this->f_.then(detail::main_thread_executor::get(),
                             [weak, f{std::move(continuation)}](boost::future<T> x){
                                 if (weak)
                                     return detail::call_and_unwrap_if_future(f, cch::unpack_arg(std::move(x)));
                                 else
                                     BOOST_THROW_EXCEPTION(detail::window_dismissed());
                             });
    };

    template<typename Window>
    auto then_on_window(Window *self, void (Window::*method)(T))
    {
        return then_on_window(self, [self,method](T x) {
            ((*self).*method)(x);
        });
    }
};


template<>
class future<void> : public future_base<void, future<void>>
{
public:
    future() {}
    using future_base<void, future<void>>::future_base;

#ifdef HAVE_PPL
    future(pplx::task<void>&& task)
    {
        auto pr = std::make_shared<promise<void>>();
        this->f_ = pr->get_future();
        task.then([pr](pplx::task<void> x) {
            try
            {
                x.get();
                pr->set_value();
            }
            catch (...)
            {
                set_current_exception(pr);
            }
        });

    }
#endif

    void get() { this->f_.get(); }

    template<typename F>
    auto then(F&& continuation) -> future<typename detail::future_unwrapper<typename std::result_of<F()>::type>::type>
    {
        typedef detail::continuation_calling_helper<typename detail::argument_type<typename std::decay<F>::type>::arg0_type> cch;
        return this->f_.then(detail::background_queue_executor::get(),
                             [f{std::move(continuation)}](boost::future<void> x){
                                 cch::touch_arg(x);
                                 return detail::call_and_unwrap_if_future(f);
                             });
    }

    template<typename F>
    auto then_on_main(F&& continuation) -> future<typename detail::future_unwrapper<typename std::result_of<F()>::type>::type>
    {
        typedef detail::continuation_calling_helper<typename detail::argument_type<typename std::decay<F>::type>::arg0_type> cch;
        return this->f_.then(detail::main_thread_executor::get(),
                             [f{std::move(continuation)}](boost::future<void> x){
                                 cch::touch_arg(x);
                                 detail::call_and_unwrap_if_future(f);
                             });

    }
    template<typename Window, typename F>
    auto then_on_window(Window *self, F&& continuation) -> future<typename detail::future_unwrapper<typename std::result_of<F()>::type>::type>
    {
        typedef detail::continuation_calling_helper<typename detail::argument_type<typename std::decay<F>::type>::arg0_type> cch;
        wxWeakRef<Window> weak(self);
        return this->f_.then(detail::main_thread_executor::get(),
                             [weak, f{std::move(continuation)}](boost::future<void> x){
                                 if (weak)
                                 {
                                     cch::touch_arg(x);
                                     detail::call_and_unwrap_if_future(f);
                                 }
                                 else
                                     BOOST_THROW_EXCEPTION(detail::window_dismissed());

                             });
    };

    template<typename Window>
    auto then_on_window(Window *self, void (Window::*method)())
    {
        return then_on_window(self, [self,method]() {
            ((*self).*method)();
        });
    }

private:
    boost::future<void> m_future;
};



template<typename T, typename FutureType>
template<typename Ex, typename F>
auto future_base<T, FutureType>::catch_ex(F&& continuation) -> future<void>
{
    return f_.then(detail::main_thread_executor::get(), [f{std::forward<F>(continuation)}](boost::future<T> x) {
        try
        {
            x.get();
        }
        catch (Ex& ex)
        {
            f(ex);
        }
    });
}

template<typename T, typename FutureType>
template<typename F>
auto future_base<T, FutureType>::catch_all(F&& continuation) -> future<void>
{
    return f_.then(detail::main_thread_executor::get(), [f{std::forward<F>(continuation)}](boost::future<T> x) {
        try
        {
            x.get();
        }
        catch (detail::window_dismissed&)
        {
            // ignore this one, it's not an error
        }
        catch (...)
        {
            f(dispatch::current_exception());
        }
    });
}



/// Create ready future, i.e. with directly set value
template<typename T>
auto make_ready_future(T&& value) -> future<T>
{
  return boost::make_ready_future(std::forward<T>(value));
}

inline future<void> make_ready_future()
{
  return boost::make_ready_future();
}

template<typename T>
auto make_exceptional_future_from_current() -> future<T>
{
    promise<T> p;
    set_current_exception(p);
    return p.get_future();
}

template<typename T>
auto make_exceptional_future(exception_ptr ex) -> future<T>
{
    promise<T> p;
    p.set_exception(ex);
    return p.get_future();
}


/// Chaining of promises
template<typename T>
void fulfill_promise_from_future(std::shared_ptr<promise<T>> p, future<T>&& f)
{
    f.then([p](future<T> val)
    {
        try
        {
            p->set_value(val.get());
        }
        catch (...)
        {
            set_current_exception(p);
        }
    });
}


/// Enqueue an operation for background processing.
template<class F>
inline auto async(F&& f) -> future<typename detail::future_unwrapper<typename std::result_of<F()>::type>::type>
{
    return {boost::async(detail::background_queue_executor::get(), [f{std::forward<F>(f)}]() {
        return detail::call_and_unwrap_if_future(f);
    })};
}


/// Run an operation on the main thread.
template<class F>
inline auto on_main(F&& f) -> future<typename detail::future_unwrapper<typename std::result_of<F()>::type>::type>
{
    return {boost::async(detail::main_thread_executor::get(), [f{std::forward<F>(f)}]() {
        return detail::call_and_unwrap_if_future(f);
    })};
}


/// @internal Call on shutdown to terminate queues and close executors
extern void cleanup();

} // namespace dispatch


#endif // Poedit_concurrency_h
