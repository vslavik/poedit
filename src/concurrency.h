/*
 *  This file is part of Poedit (http://poedit.net)
 *
 *  Copyright (C) 2010-2015 Vaclav Slavik
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

#include <functional>

#include <wx/app.h>
#include <wx/weakref.h>

#include "ThreadPool.h"

// ----------------------------------------------------------------------
// Background operations
// ----------------------------------------------------------------------

class background_queue
{
public:
    /**
        Enqueue an operation for background processing.
        
        Return future for it.
     */
    template<class F, class... Args>
    static auto add(F&& f, Args&&... args)
        -> std::future<typename std::result_of<F(Args...)>::type>
    {
        return ms_pool.enqueue(f, args...);
    }

private:
    // TODO: Use NSOperationQeueue on OS X
    static ThreadPool ms_pool;
};


// ----------------------------------------------------------------------
// Helpers for running code on the main thread
// ----------------------------------------------------------------------

#if defined(__WXOSX__) && defined(__clang__)
    #define HAVE_DISPATCH
    extern void call_on_main_thread_impl(std::function<void()> func);
#endif

/**
    Simply calls the callable @a func on the main thread, asynchronously.
 */
template<typename F>
void call_on_main_thread(F&& func)
{
#ifdef HAVE_DISPATCH
    call_on_main_thread_impl(func);
#else
    wxTheApp->CallAfter(func);
#endif
}

#if defined(__clang__)

template<typename... Args>
auto on_main_thread_impl(std::function<void(Args...)> func) -> std::function<void(Args...)>
{
    return [func](Args... args){
        call_on_main_thread([func,args...]{
            func(args...);
        });
    };
}

#else // sigh... neither VS2013 nor GCC 4.8 can deal with the above

// Visual Studio 2013 is broken and won't parse the above; 2015 fixes it.
inline auto on_main_thread_impl(std::function<void()> func) -> std::function<void()>
{
    return [func](){ call_on_main_thread([=]{ func(); }); };
}
template<typename A1>
auto on_main_thread_impl(std::function<void(A1)> func) -> std::function<void(A1)>
{
    return [func](A1 a1){ call_on_main_thread([=]{ func(a1); }); };
}
template<typename A1, typename A2>
auto on_main_thread_impl(std::function<void(A1,A2)> func) -> std::function<void(A1,A2)>
{
    return [func](A1 a1, A2 a2){ call_on_main_thread([=]{ func(a1,a2); }); };
}
template<typename A1, typename A2, typename A3>
auto on_main_thread_impl(std::function<void(A1, A2, A3)> func) -> std::function<void(A1, A2, A3)>
{
    return [func](A1 a1, A2 a2, A3 a3){ call_on_main_thread([=]{ func(a1, a2, a3); }); };
}

#endif

/**
    Wraps a callable into std::function called on the main thread.
    
    Returned function takes the same arguments as @a func and is called on the
    main thread. I.e. the returned object may be called from any thread, but
    @a func is guaranteed to execute on the main one.
    
    Notice that it is necessary to specify template parameters because they
    cannot be deduced.
    
    Example usage:
    
        on_main_thread<int,std::string>(this, [=](int i, std::string s){
            ...
        })
 */
template<typename... Args, typename F>
auto on_main_thread(F&& func) -> std::function<void(Args...)>
{
    return on_main_thread_impl(std::function<void(Args...)>(func));
}

/**
    Like on_main_thread<> but is only called if @a window is still valid
    (using a wxWeakRef<> to check).
 */
template<typename... Args, typename F, typename Class>
auto on_main_thread_for_window(Class *self, F&& func) -> std::function<void(Args...)>
{
    wxWeakRef<Class> weak(self);
    return on_main_thread<Args...>([=](Args... args){
        if (weak)
            func(args...);
    });
}


/**
    Wraps a method into function called on the main thread.
    
    Returned function takes the same arguments as the provided method @a func
    and is called on the main thread. I.e. the returned object may be called
    from any thread, but @a func is guaranteed to execute on the main one.
    
    Example usage:
    
        on_main_thread(this, &CrowdinOpenDialog::OnFetchedProjects)
 */
template<typename Class, typename... Args>
auto on_main_thread(Class *self, void (Class::*func)(Args...)) -> std::function<void(Args...)>
{
    wxWeakRef<Class> weak(self);
    return on_main_thread<Args...>([=](Args... args){
        if (weak)
            ((*weak.get()).*func)(args...);
    });
}

#endif // Poedit_concurrency_h
