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

#include "concurrency.h"

#ifdef HAVE_DISPATCH


#include <dispatch/dispatch.h>

void call_on_main_thread_impl(std::function<void()>&& f)
{
    std::function<void()> func(std::move(f));
    dispatch_async(dispatch_get_main_queue(), [func]{ func(); });
}

void background_queue::enqueue(std::function<void()>&& f)
{
    std::function<void()> func(std::move(f));
    dispatch_queue_t q = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
    dispatch_async(q, [func]{ func(); });
}

void background_queue::cleanup()
{
}


#else // !HAVE_DISPATCH


#include "ThreadPool.h"

#include <thread>

namespace
{

std::unique_ptr<ThreadPool> gs_pool;
static std::once_flag initializationFlag;

ThreadPool& pool()
{
    std::call_once(initializationFlag, []{
        gs_pool.reset(new ThreadPool(std::thread::hardware_concurrency() + 1));
    });
    return *gs_pool;
}

} // anonymous namespace

void background_queue::enqueue(std::function<void()>&& f)
{
    pool().enqueue_func(std::move(f));
}

void background_queue::cleanup()
{
    gs_pool.reset();
}


#endif // !HAVE_DISPATCH
