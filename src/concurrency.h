/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 2010-2016 Vaclav Slavik
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
#endif


#define BOOST_THREAD_VERSION 4
#define BOOST_THREAD_PROVIDES_EXECUTORS

#include <boost/thread/executor.hpp>
#include <boost/thread/future.hpp>

#if !defined(HAVE_DISPATCH) && !defined(HAVE_PPL)
    #include <boost/thread/executors/basic_thread_pool.hpp>
#endif

#if defined(HAVE_PPL)
    #include <concrt.h>
    #include <ppltasks.h>
#endif

#include <memory>
#include <mutex>

#include <wx/app.h>
#include <wx/weakref.h>


namespace dispatch
{

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

#elif defined(HAVE_PPL)

class background_queue_executor : public custom_executor
{
public:
    static background_queue_executor& get();

    void submit(work&& closure)
    {
        Concurrency::create_task([f{std::move(closure)}]() mutable { f(); });
    }
};

#else // !HAVE_DISPATCH && !HAVE_PPL

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

} // namespace detail


// ----------------------------------------------------------------------
// Tasks (aka futures)
// ----------------------------------------------------------------------

template<typename T>
using promise = boost::promise<T>;

// Can't use std::current_exception with boost::promise, must use boost
// version instead. This helper takes care of it.
template<typename T>
void set_current_exception(boost::promise<T>& pr)
{
    pr.set_exception(boost::current_exception());
}

template<typename T>
void set_current_exception(std::shared_ptr<boost::promise<T>> pr) { set_current_exception(*pr); }


template<typename T>
class future;

template<typename T, typename FutureType>
class future_base
{
public:
    future_base() {}
    future_base(FutureType&& future) : f_(std::move(future.m_future)) {}

    future_base(boost::future<T>&& future) : f_(std::move(future)) {}

    FutureType& operator=(FutureType&& other) { f_ = std::move(other.f_); }
    FutureType& operator=(const FutureType& other) = delete;

    void wait() const { f_.wait(); }
    bool valid() const { return f_.valid(); }

    // Convenient async exception catching:

    template<typename Ex, typename F>
    auto catch_ex(F&& continuation) -> future<void>;

    template<typename F>
    auto catch_all(F&& continuation) -> future<void>;

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
    future(Concurrency::task<T>&& task)
    {
        auto pr = std::make_shared<promise<T>>();
        f_ = pr->get_future();
        task.then([pr](Concurrency::task<T> x) {
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
    auto then(F&& continuation) -> future<typename std::result_of<F(T)>::type>
    {
        return this->f_.then(detail::background_queue_executor::get(),
                             [f{std::move(continuation)}](boost::future<T> x){
                                 return f(x.get());
                             });
    }

    template<typename F>
    auto then_on_main(F&& continuation) -> future<typename std::result_of<F(T)>::type>
    {
        return this->f_.then(detail::main_thread_executor::get(),
                             [f{std::move(continuation)}](boost::future<T> x){
                                 return f(x.get());
                             });
    }

    template<typename Window, typename F>
    auto then_on_window(Window *self, F&& continuation) -> future<typename std::result_of<F(T)>::type>
    {
        wxWeakRef<Window> weak(self);
        return this->f_.then(detail::main_thread_executor::get(),
                             [weak, f{std::move(continuation)}](boost::future<T> x){
                                 if (weak)
                                     return f(x.get());
                                 else
                                     throw detail::window_dismissed();
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
    future(Concurrency::task<void>&& task)
    {
        auto pr = std::make_shared<promise<void>>();
        f_ = pr->get_future();
        task.then([pr](Concurrency::task<void> x) {
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
    auto then(F&& continuation) -> future<typename std::result_of<F()>::type>
    {
        return this->f_.then(detail::background_queue_executor::get(),
                             [f{std::move(continuation)}](boost::future<void> x){
                                 x.get();
                                 return f();
                             });
    }

    template<typename F>
    auto then_on_main(F&& continuation) -> future<typename std::result_of<F()>::type>
    {
        return this->f_.then(detail::main_thread_executor::get(),
                             [f{std::move(continuation)}](boost::future<void> x){
                                 x.get();
                                 return f();
                             });

    }
    template<typename Window, typename F>
    auto then_on_window(Window *self, F&& continuation) -> future<typename std::result_of<F()>::type>
    {
        wxWeakRef<Window> weak(self);
        return this->f_.then(detail::main_thread_executor::get(),
                             [weak, f{std::move(continuation)}](boost::future<void> x){
                                 if (weak)
                                 {
                                     x.get();
                                     f();
                                 }
                                 else
                                     throw detail::window_dismissed();

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
            f(std::current_exception());
        }
    });
}


/// Enqueue an operation for background processing.
template<class F>
inline auto async(F&& f) -> future<typename std::result_of<F()>::type>
{
    return {boost::async(detail::background_queue_executor::get(), std::forward<F>(f))};
}


/// Run an operation on the main thread.
template<class F>
inline auto on_main(F&& f) -> future<typename std::result_of<F()>::type>
{
    return {boost::async(detail::main_thread_executor::get(), std::forward<F>(f))};
}


/// @internal Call on shutdown to terminate queues and close executors
extern void cleanup();

} // namespace dispatch


#endif // Poedit_concurrency_h
