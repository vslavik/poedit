/*
 *  This file is part of Poedit (https://poedit.com)
 *
 *  Copyright (C) 2026 Vaclav Slavik
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

#ifndef Poedit_wrap_cpprestsdk_h
#define Poedit_wrap_cpprestsdk_h

#if defined(_MSC_VER) && _MSC_VER >= 1950

#include "../deps/custom_build/compat/cpprestsdk_vs2026.h"

// cpprestsdk uses this non-standard member, removed in Visual Studio 2026.
static_assert(std::is_same_v<std::ios_base::openmode, int>);
#pragma push_macro("_Openprot")
#define _Openprot openmode(_SH_DENYNO)

#endif


#include <cpprest/asyncrt_utils.h>
#include <cpprest/http_client.h>
#include <cpprest/http_msg.h>
#include <cpprest/filestream.h>


#if defined(_MSC_VER) && _MSC_VER >= 1950
#undef _Openprot
#endif

#endif // Poedit_wrap_cpprestsdk_h
