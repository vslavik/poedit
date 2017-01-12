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

#include "concurrency.h"

#include "errors.h"
#include <wx/log.h>

using namespace dispatch;

#if defined(HAVE_DISPATCH)

#include <dispatch/dispatch.h>

void detail::dispatch_async_cxx(boost::executors::work&& f, detail::queue q)
{
    dispatch_queue_t dq = 0;
    switch (q)
    {
        case detail::queue::main:
            dq = dispatch_get_main_queue();
            break;
        case detail::queue::priority_default:
            dq = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
            break;
    }

    dispatch_async(dq, [f{std::move(f)}]() mutable {
        try
        {
            f();
        }
        catch (...)
        {
            // FIXME: This is gross. Should be reported better and properly, but this
            //        is consistent with pplx/ConcurrencyRT/futures, so do it for now.
            wxLogDebug("uncaught exception: %s", DescribeCurrentException());
        }
    });
}

#endif // HAVE_DISPATCH


namespace
{

std::unique_ptr<detail::background_queue_executor> gs_background_executor;
std::unique_ptr<detail::main_thread_executor> gs_main_thread_executor;
static std::once_flag gs_background_executor_flag, gs_main_thread_executor_flag;

}

dispatch::detail::background_queue_executor&
dispatch::detail::background_queue_executor::get()
{
    std::call_once(gs_background_executor_flag, []{
        gs_background_executor.reset(new background_queue_executor);
    });
    return *gs_background_executor;
}

dispatch::detail::main_thread_executor&
dispatch::detail::main_thread_executor::get()
{
    std::call_once(gs_main_thread_executor_flag, []{
        gs_main_thread_executor.reset(new main_thread_executor);
    });
    return *gs_main_thread_executor;
}

void dispatch::cleanup()
{
    if (gs_background_executor)
        gs_background_executor->close();
    if (gs_main_thread_executor)
        gs_main_thread_executor->close();

    gs_background_executor.reset();
    gs_main_thread_executor.reset();
}
