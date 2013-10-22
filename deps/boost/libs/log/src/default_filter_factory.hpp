/*
 *          Copyright Andrey Semashev 2007 - 2013.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   default_filter_factory.hpp
 * \author Andrey Semashev
 * \date   29.05.2010
 *
 * \brief  This header is the Boost.Log library implementation, see the library documentation
 *         at http://www.boost.org/libs/log/doc/log.html.
 */

#ifndef BOOST_DEFAULT_FILTER_FACTORY_HPP_INCLUDED_
#define BOOST_DEFAULT_FILTER_FACTORY_HPP_INCLUDED_

#include <boost/log/utility/setup/filter_parser.hpp>
#include <boost/log/detail/header.hpp>

namespace boost {

BOOST_LOG_OPEN_NAMESPACE

namespace aux {

//! The default filter factory that supports creating filters for the standard types (see utility/type_dispatch/standard_types.hpp)
template< typename CharT >
class default_filter_factory :
    public filter_factory< CharT >
{
    //! Base type
    typedef filter_factory< CharT > base_type;
    //! Self type
    typedef default_filter_factory< CharT > this_type;

    template< typename ValueT, typename PredicateT >
    struct predicate_wrapper;

    template< typename RelationT >
    struct on_integral_argument;
    template< typename RelationT >
    struct on_fp_argument;
    template< typename RelationT >
    struct on_string_argument;
    template< typename RelationT >
    struct on_regex_argument;

public:
    //  Type imports
    typedef typename base_type::char_type char_type;
    typedef typename base_type::string_type string_type;

    //! The callback for equality relation filter
    virtual filter on_equality_relation(attribute_name const& name, string_type const& arg);
    //! The callback for inequality relation filter
    virtual filter on_inequality_relation(attribute_name const& name, string_type const& arg);
    //! The callback for less relation filter
    virtual filter on_less_relation(attribute_name const& name, string_type const& arg);
    //! The callback for greater relation filter
    virtual filter on_greater_relation(attribute_name const& name, string_type const& arg);
    //! The callback for less or equal relation filter
    virtual filter on_less_or_equal_relation(attribute_name const& name, string_type const& arg);
    //! The callback for greater or equal relation filter
    virtual filter on_greater_or_equal_relation(attribute_name const& name, string_type const& arg);

    //! The callback for custom relation filter
    virtual filter on_custom_relation(attribute_name const& name, string_type const& rel, string_type const& arg);

    //! The function parses the argument value for a binary relation and constructs the corresponding filter
    template< typename RelationT >
    static filter parse_argument(attribute_name const& name, string_type const& arg);
};

} // namespace aux

BOOST_LOG_CLOSE_NAMESPACE // namespace log

} // namespace boost

#include <boost/log/detail/footer.hpp>

#endif // BOOST_DEFAULT_FILTER_FACTORY_HPP_INCLUDED_
