/*
 *          Copyright Andrey Semashev 2007 - 2015.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   formatter.hpp
 * \author Andrey Semashev
 * \date   13.07.2012
 *
 * The header contains a formatter function object definition.
 */

#ifndef BOOST_LOG_EXPRESSIONS_FORMATTER_HPP_INCLUDED_
#define BOOST_LOG_EXPRESSIONS_FORMATTER_HPP_INCLUDED_

#include <boost/ref.hpp>
#include <boost/move/core.hpp>
#include <boost/move/utility.hpp>
#if defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
#include <boost/utility/enable_if.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/type_traits/remove_cv.hpp>
#endif
#include <boost/log/detail/config.hpp>
#include <boost/log/detail/light_function.hpp>
#include <boost/log/attributes/attribute_value_set.hpp>
#include <boost/log/attributes/value_visitation.hpp>
#include <boost/log/core/record_view.hpp>
#include <boost/log/utility/formatting_ostream.hpp>
#include <boost/log/utility/functional/bind_output.hpp>
#include <boost/log/expressions/message.hpp>
#include <boost/log/detail/header.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {

BOOST_LOG_OPEN_NAMESPACE

namespace expressions {

namespace aux {

// This reference class is a workaround for a Boost.Phoenix bug: https://svn.boost.org/trac/boost/ticket/9363
// It is needed to pass output streams by non-const reference to function objects wrapped in phoenix::bind and phoenix::function.
// It's an implementation detail and will be removed when Boost.Phoenix is fixed.
template< typename StreamT >
class stream_ref :
    public reference_wrapper< StreamT >
{
public:
    BOOST_FORCEINLINE explicit stream_ref(StreamT& strm) : reference_wrapper< StreamT >(strm)
    {
    }

    template< typename T >
    BOOST_FORCEINLINE StreamT& operator<< (T& val) const
    {
        StreamT& strm = this->get();
        strm << val;
        return strm;
    }

    template< typename T >
    BOOST_FORCEINLINE StreamT& operator<< (T const& val) const
    {
        StreamT& strm = this->get();
        strm << val;
        return strm;
    }
};

//! Default log record message formatter
struct message_formatter
{
    typedef void result_type;

    message_formatter() : m_MessageName(expressions::tag::message::get_name())
    {
    }

    template< typename StreamT >
    result_type operator() (record_view const& rec, StreamT& strm) const
    {
        boost::log::visit< expressions::tag::message::value_type >(m_MessageName, rec, boost::log::bind_output(strm));
    }

private:
    const attribute_name m_MessageName;
};

} // namespace aux

} // namespace expressions

/*!
 * Log record formatter function wrapper.
 */
template< typename CharT >
class basic_formatter
{
    typedef basic_formatter this_type;
    BOOST_COPYABLE_AND_MOVABLE(this_type)

public:
    //! Result type
    typedef void result_type;

    //! Character type
    typedef CharT char_type;
    //! Output stream type
    typedef basic_formatting_ostream< char_type > stream_type;

private:
    //! Filter function type
    typedef boost::log::aux::light_function< void (record_view const&, expressions::aux::stream_ref< stream_type >) > formatter_type;

private:
    //! Formatter function
    formatter_type m_Formatter;

public:
    /*!
     * Default constructor. Creates a formatter that only outputs log message.
     */
    basic_formatter() : m_Formatter(expressions::aux::message_formatter())
    {
    }
    /*!
     * Copy constructor
     */
    basic_formatter(basic_formatter const& that) : m_Formatter(that.m_Formatter)
    {
    }
    /*!
     * Move constructor. The moved-from formatter is left in an unspecified state.
     */
    basic_formatter(BOOST_RV_REF(this_type) that) BOOST_NOEXCEPT : m_Formatter(boost::move(that.m_Formatter))
    {
    }

    /*!
     * Initializing constructor. Creates a formatter which will invoke the specified function object.
     */
#if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
    template< typename FunT >
    basic_formatter(FunT&& fun) : m_Formatter(boost::forward< FunT >(fun))
    {
    }
#elif !defined(BOOST_MSVC) || BOOST_MSVC > 1400
    template< typename FunT >
    basic_formatter(FunT const& fun, typename disable_if_c< move_detail::is_rv< FunT >::value, int >::type = 0) : m_Formatter(fun)
    {
    }
#else
    // MSVC 8 blows up in unexpected ways if we use SFINAE to disable constructor instantiation
    template< typename FunT >
    basic_formatter(FunT const& fun) : m_Formatter(fun)
    {
    }
    template< typename FunT >
    basic_formatter(rv< FunT >& fun) : m_Formatter(fun)
    {
    }
    template< typename FunT >
    basic_formatter(rv< FunT > const& fun) : m_Formatter(static_cast< FunT const& >(fun))
    {
    }
    basic_formatter(rv< this_type > const& that) : m_Formatter(that.m_Formatter)
    {
    }
#endif

    /*!
     * Move assignment. The moved-from formatter is left in an unspecified state.
     */
    basic_formatter& operator= (BOOST_RV_REF(this_type) that) BOOST_NOEXCEPT
    {
        m_Formatter.swap(that.m_Formatter);
        return *this;
    }
    /*!
     * Copy assignment.
     */
    basic_formatter& operator= (BOOST_COPY_ASSIGN_REF(this_type) that)
    {
        m_Formatter = that.m_Formatter;
        return *this;
    }
    /*!
     * Initializing assignment. Sets the specified function object to the formatter.
     */
#if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
    template< typename FunT >
    basic_formatter& operator= (FunT&& fun)
    {
        this_type(boost::forward< FunT >(fun)).swap(*this);
        return *this;
    }
#else
    template< typename FunT >
    typename disable_if< is_same< typename remove_cv< FunT >::type, this_type >, this_type& >::type
    operator= (FunT const& fun)
    {
        this_type(fun).swap(*this);
        return *this;
    }
#endif

    /*!
     * Formatting operator.
     *
     * \param rec A log record to format.
     * \param strm A stream to put the formatted characters to.
     */
    result_type operator() (record_view const& rec, stream_type& strm) const
    {
        m_Formatter(rec, expressions::aux::stream_ref< stream_type >(strm));
    }

    /*!
     * Resets the formatter to the default. The default formatter only outputs message text.
     */
    void reset()
    {
        m_Formatter = expressions::aux::message_formatter();
    }

    /*!
     * Swaps two formatters
     */
    void swap(basic_formatter& that) BOOST_NOEXCEPT
    {
        m_Formatter.swap(that.m_Formatter);
    }
};

template< typename CharT >
inline void swap(basic_formatter< CharT >& left, basic_formatter< CharT >& right) BOOST_NOEXCEPT
{
    left.swap(right);
}

#ifdef BOOST_LOG_USE_CHAR
typedef basic_formatter< char > formatter;
#endif
#ifdef BOOST_LOG_USE_WCHAR_T
typedef basic_formatter< wchar_t > wformatter;
#endif

BOOST_LOG_CLOSE_NAMESPACE // namespace log

} // namespace boost

#include <boost/log/detail/footer.hpp>

#endif // BOOST_LOG_EXPRESSIONS_FORMATTER_HPP_INCLUDED_
