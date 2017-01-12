/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 2013-2017 Vaclav Slavik
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

#ifndef _ERRORS_H_
#define _ERRORS_H_

#include <wx/string.h>
#include <exception>
#include <stdexcept>

#include <boost/exception_ptr.hpp>

/// Any exception error.
/// Pretty much the same as std::runtime_error, except using Unicode strings.
class Exception : public std::runtime_error
{
public:
    Exception(const wxString& what)
        : std::runtime_error(std::string(what.utf8_str())), m_what(what) {}

    const wxString& What() const { return m_what; }

    // prevent use of std::exception::what() on Exception-cast instances:
private:
#ifdef _MSC_VER
    const char* what() const override { return std::runtime_error::what(); }
#else
    const char* what() const noexcept override { return std::runtime_error::what(); }
#endif

private:
    wxString m_what;
};


/// Helper to convert an exception into a human-readable string
template<typename Rethrow>
inline wxString DoDescribeException(Rethrow&& rethrow_exception)
{
    try
    {
        rethrow_exception();
        return "no error"; // silence stupid VC++
    }
    catch (const Exception& e)
    {
        return e.What();
    }
    catch (const std::exception& e)
    {
        const char *msg = e.what();
        // try interpreting as UTF-8 first as the most likely one (from external sources)
        wxString s = wxString::FromUTF8(msg);
        if (s.empty())
        {
            s = wxString(msg);
            if (s.empty()) // not in current locale either, fall back to Latin1
                s = wxString(msg, wxConvISO8859_1);
        }
        return s;
    }
    catch (...)
    {
        return "unknown error";
    }
}

inline wxString DescribeException(std::exception_ptr e)
{
    return DoDescribeException([e]{ std::rethrow_exception(e); });
}

inline wxString DescribeException(boost::exception_ptr e)
{
    return DoDescribeException([e]{ boost::rethrow_exception(e); });
}

inline wxString DescribeCurrentException()
{
    return DescribeException(std::current_exception());
}

#endif // _ERRORS_H_
