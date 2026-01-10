/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 2013-2026 Vaclav Slavik
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

#ifndef Poedit_errors_h
#define Poedit_errors_h

#include <wx/string.h>
#include <exception>
#include <stdexcept>
#include <functional>

#include <boost/exception_ptr.hpp>

/// Any exception error.
/// Pretty much the same as std::runtime_error, except using Unicode strings.
class Exception : public std::runtime_error
{
public:
    Exception(const wxString& what)
        : std::runtime_error(what.utf8_string()), m_what(what) {}

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


namespace errors::detail
{

struct Rethrower
{
	virtual void rethrow() = 0;
    virtual ~Rethrower() {}
};

template<typename T>
struct RethrowerImpl : public Rethrower
{
    RethrowerImpl(T&& func) : rethrow_exception(std::move(func)) {}
    void rethrow() override { rethrow_exception(); }

    T rethrow_exception;
};

wxString DescribeExceptionImpl(Rethrower& rethrower);

template<typename T>
inline wxString DescribeException(T&& rethrow_exception)
{
    RethrowerImpl<T> rethrower(std::move(rethrow_exception));
    return DescribeExceptionImpl(rethrower);
}

} // namespace errors::detail

/// Helper to convert an exception into a human-readable string
inline wxString DescribeException(std::exception_ptr e)
{
    return errors::detail::DescribeException([e]{ std::rethrow_exception(e); });
}

inline wxString DescribeException(boost::exception_ptr e)
{
    return errors::detail::DescribeException([e]{ boost::rethrow_exception(e); });
}

inline wxString DescribeCurrentException()
{
    return DescribeException(std::current_exception());
}

#endif // Poedit_errors_h
