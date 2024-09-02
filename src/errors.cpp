/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 2013-2024 Vaclav Slavik
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

#include "errors.h"

#include <wx/translation.h>

#if defined(HAVE_HTTP_CLIENT) && !defined(__WXOSX__)
#include <cpprest/http_client.h>
#include <boost/algorithm/string.hpp>
#endif

namespace
{

inline wxString from_c_string(const char *msg)
{
    // try interpreting as UTF-8 first as the most likely one (from external sources)
    wxString s = wxString::FromUTF8(msg);
    if (!s.empty())
        return s;

    s = wxString(msg);
    if (!s.empty())
        return s;

    // not in current locale either, fall back to Latin1
    return wxString(msg, wxConvISO8859_1);
}

} // anonymous namespace


wxString errors::detail::DescribeExceptionImpl(Rethrower& rethrower)
{
    try
    {
        rethrower.rethrow();
        return "no error"; // silence stupid VC++
    }
    catch (const Exception& e)
    {
        return e.What();
    }
#if defined(HAVE_HTTP_CLIENT) && !defined(__WXOSX__)
    catch (const web::http::http_exception & e)
    {
        // rephrase the errors more humanly; the default form is too cryptic
        // also strip trailing newlines that C++REST tends to add
        std::string msg(e.what());
        if (!boost::starts_with(msg, "WinHttp"))
        {
            boost::trim_right(msg);
            return from_c_string(msg.c_str());  // preserve actual messages
        }

        msg = e.error_code().message();
        if (msg.empty())
            return from_c_string(e.what());  // give up

        boost::trim_right(msg);
        return wxString::Format(_("Network error: %s (%d)"), from_c_string(msg.c_str()), e.error_code().value());
    }
#endif // defined(HAVE_HTTP_CLIENT) && !defined(__WXOSX__)
    catch (const std::exception& e)
    {
        return from_c_string(e.what());
    }
    catch (...)
    {
        return _("Unknown error");
    }
}
