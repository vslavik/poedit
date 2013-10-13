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

#include <string>
#include <boost/move/core.hpp>
#include <boost/move/utility.hpp>
#include <boost/spirit/include/qi_core.hpp>
#include <boost/spirit/include/qi_eoi.hpp>
#include <boost/spirit/include/qi_as.hpp>
#include <boost/log/exceptions.hpp>
#include <boost/log/attributes/value_visitation.hpp>
#include <boost/log/expressions/predicates/has_attr.hpp>
#include <boost/log/utility/type_dispatch/standard_types.hpp>
#include <boost/log/utility/string_literal.hpp>
#include <boost/log/utility/functional/logical.hpp>
#include <boost/log/utility/functional/begins_with.hpp>
#include <boost/log/utility/functional/ends_with.hpp>
#include <boost/log/utility/functional/contains.hpp>
#include <boost/log/utility/functional/matches.hpp>
#include <boost/log/utility/functional/bind.hpp>
#include <boost/log/utility/functional/as_action.hpp>
#include <boost/log/utility/functional/save_result.hpp>
#include <boost/log/detail/code_conversion.hpp>
#include <boost/log/support/xpressive.hpp>
#include <boost/xpressive/xpressive_dynamic.hpp>
#if defined(BOOST_LOG_USE_CHAR) && defined(BOOST_LOG_USE_WCHAR_T)
#include <boost/fusion/container/set.hpp>
#include <boost/fusion/sequence/intrinsic/at_key.hpp>
#include <boost/fusion/algorithm/iteration/for_each.hpp>
#endif
#include "default_filter_factory.hpp"
#include "parser_utils.hpp"
#include "spirit_encoding.hpp"
#include <boost/log/detail/header.hpp>

namespace qi = boost::spirit::qi;

namespace boost {

BOOST_LOG_OPEN_NAMESPACE

namespace aux {

template< typename CharT >
template< typename ValueT, typename PredicateT >
struct default_filter_factory< CharT >::predicate_wrapper
{
    typedef typename PredicateT::result_type result_type;

    explicit predicate_wrapper(attribute_name const& name, PredicateT const& pred) : m_name(name), m_visitor(pred)
    {
    }

    template< typename T >
    result_type operator() (T const& arg) const
    {
        bool res = false;
        boost::log::visit< ValueT >(m_name, arg, save_result_wrapper< PredicateT const&, bool >(m_visitor, res));
        return res;
    }

private:
    attribute_name m_name;
    const PredicateT m_visitor;
};

template< typename CharT >
template< typename RelationT >
struct default_filter_factory< CharT >::on_integral_argument
{
    typedef void result_type;

    on_integral_argument(attribute_name const& name, filter& f) : m_name(name), m_filter(f)
    {
    }

    result_type operator() (long val) const
    {
        typedef binder2nd< RelationT, long > predicate;
        m_filter = predicate_wrapper< log::integral_types::type, predicate >(m_name, predicate(RelationT(), val));
    }

private:
    attribute_name m_name;
    filter& m_filter;
};

template< typename CharT >
template< typename RelationT >
struct default_filter_factory< CharT >::on_fp_argument
{
    typedef void result_type;

    on_fp_argument(attribute_name const& name, filter& f) : m_name(name), m_filter(f)
    {
    }

    result_type operator() (double val) const
    {
        typedef binder2nd< RelationT, double > predicate;
        m_filter = predicate_wrapper< log::floating_point_types::type, predicate >(m_name, predicate(RelationT(), val));
    }

private:
    attribute_name m_name;
    filter& m_filter;
};

template< typename CharT >
template< typename RelationT >
struct default_filter_factory< CharT >::on_string_argument
{
    typedef void result_type;

#if defined(BOOST_LOG_USE_CHAR) && defined(BOOST_LOG_USE_WCHAR_T)
    //! A special filtering predicate that adopts the string operand to the attribute value character type
    struct predicate :
        public RelationT
    {
        struct initializer
        {
            typedef void result_type;

            explicit initializer(string_type const& val) : m_initializer(val)
            {
            }

            template< typename T >
            result_type operator() (T& val) const
            {
                try
                {
                    log::aux::code_convert(m_initializer, val);
                }
                catch (...)
                {
                    val.clear();
                }
            }

        private:
            string_type const& m_initializer;
        };

        typedef typename RelationT::result_type result_type;

        explicit predicate(RelationT const& rel, string_type const& operand) : RelationT(rel)
        {
            fusion::for_each(m_operands, initializer(operand));
        }

        template< typename T >
        result_type operator() (T const& val) const
        {
            typedef std::basic_string< typename T::value_type > operand_type;
            return RelationT::operator() (val, fusion::at_key< operand_type >(m_operands));
        }

    private:
        fusion::set< std::string, std::wstring > m_operands;
    };
#else
    typedef binder2nd< RelationT, string_type > predicate;
#endif

    on_string_argument(attribute_name const& name, filter& f) : m_name(name), m_filter(f)
    {
    }

    result_type operator() (string_type const& val) const
    {
        m_filter = predicate_wrapper< log::string_types::type, predicate >(m_name, predicate(RelationT(), val));
    }

private:
    attribute_name m_name;
    filter& m_filter;
};

template< typename CharT >
template< typename RelationT >
struct default_filter_factory< CharT >::on_regex_argument
{
    typedef void result_type;

#if defined(BOOST_LOG_USE_CHAR) && defined(BOOST_LOG_USE_WCHAR_T)
    //! A special filtering predicate that adopts the string operand to the attribute value character type
    struct predicate :
        public RelationT
    {
        struct initializer
        {
            typedef void result_type;

            explicit initializer(string_type const& val) : m_initializer(val)
            {
            }

            template< typename T >
            result_type operator() (T& val) const
            {
                try
                {
                    typedef typename T::char_type char_type;
                    std::basic_string< char_type > str;
                    log::aux::code_convert(m_initializer, str);
                    val = T::compile(str.c_str(), str.size(), T::ECMAScript | T::optimize);
                }
                catch (...)
                {
                }
            }

        private:
            string_type const& m_initializer;
        };

        typedef typename RelationT::result_type result_type;

        explicit predicate(RelationT const& rel, string_type const& operand) : RelationT(rel)
        {
            fusion::for_each(m_operands, initializer(operand));
        }

        template< typename T >
        result_type operator() (T const& val) const
        {
            typedef typename T::value_type char_type;
            typedef xpressive::basic_regex< const char_type* > regex_type;
            return RelationT::operator() (val, fusion::at_key< regex_type >(m_operands));
        }

    private:
        fusion::set< xpressive::cregex, xpressive::wcregex > m_operands;
    };
#else
    //! A special filtering predicate that adopts the string operand to the attribute value character type
    struct predicate :
        public RelationT
    {
        typedef typename RelationT::result_type result_type;
        typedef xpressive::basic_regex< typename string_type::value_type const* > regex_type;

        explicit predicate(RelationT const& rel, string_type const& operand) :
            RelationT(rel),
            m_operand(regex_type::compile(operand.c_str(), operand.size(), regex_type::ECMAScript | regex_type::optimize))
        {
        }

        template< typename T >
        result_type operator() (T const& val) const
        {
            return RelationT::operator() (val, m_operand);
        }

    private:
        regex_type m_operand;
    };
#endif

    on_regex_argument(attribute_name const& name, filter& f) : m_name(name), m_filter(f)
    {
    }

    result_type operator() (string_type const& val) const
    {
        m_filter = predicate_wrapper< log::string_types::type, predicate >(m_name, predicate(RelationT(), val));
    }

private:
    attribute_name m_name;
    filter& m_filter;
};


//! The callback for equality relation filter
template< typename CharT >
filter default_filter_factory< CharT >::on_equality_relation(attribute_name const& name, string_type const& arg)
{
    return parse_argument< equal_to >(name, arg);
}

//! The callback for inequality relation filter
template< typename CharT >
filter default_filter_factory< CharT >::on_inequality_relation(attribute_name const& name, string_type const& arg)
{
    return parse_argument< not_equal_to >(name, arg);
}

//! The callback for less relation filter
template< typename CharT >
filter default_filter_factory< CharT >::on_less_relation(attribute_name const& name, string_type const& arg)
{
    return parse_argument< less >(name, arg);
}

//! The callback for greater relation filter
template< typename CharT >
filter default_filter_factory< CharT >::on_greater_relation(attribute_name const& name, string_type const& arg)
{
    return parse_argument< greater >(name, arg);
}

//! The callback for less or equal relation filter
template< typename CharT >
filter default_filter_factory< CharT >::on_less_or_equal_relation(attribute_name const& name, string_type const& arg)
{
    return parse_argument< less_equal >(name, arg);
}

//! The callback for greater or equal relation filter
template< typename CharT >
filter default_filter_factory< CharT >::on_greater_or_equal_relation(attribute_name const& name, string_type const& arg)
{
    return parse_argument< greater_equal >(name, arg);
}

//! The callback for custom relation filter
template< typename CharT >
filter default_filter_factory< CharT >::on_custom_relation(attribute_name const& name, string_type const& rel, string_type const& arg)
{
    typedef log::aux::char_constants< char_type > constants;

    filter f;
    if (rel == constants::begins_with_keyword())
        on_string_argument< begins_with_fun >(name, f)(arg);
    else if (rel == constants::ends_with_keyword())
        on_string_argument< ends_with_fun >(name, f)(arg);
    else if (rel == constants::contains_keyword())
        on_string_argument< contains_fun >(name, f)(arg);
    else if (rel == constants::matches_keyword())
        on_regex_argument< matches_fun >(name, f)(arg);
    else
    {
        BOOST_LOG_THROW_DESCR(parse_error, "The custom attribute relation \"" + log::aux::to_narrow(rel) + "\" is not supported");
    }

    return boost::move(f);
}


//! The function parses the argument value for a binary relation and constructs the corresponding filter
template< typename CharT >
template< typename RelationT >
filter default_filter_factory< CharT >::parse_argument(attribute_name const& name, string_type const& arg)
{
    typedef log::aux::encoding_specific< typename log::aux::encoding< char_type >::type > encoding_specific;
    const qi::real_parser< double, qi::strict_real_policies< double > > real_;

    filter f;
    const on_fp_argument< RelationT > on_fp(name, f);
    const on_integral_argument< RelationT > on_int(name, f);
    const on_string_argument< RelationT > on_str(name, f);

    const bool res = qi::parse
    (
        arg.c_str(), arg.c_str() + arg.size(),
        (
            real_[boost::log::as_action(on_fp)] |
            qi::long_[boost::log::as_action(on_int)] |
            qi::as< string_type >()[ +encoding_specific::print ][boost::log::as_action(on_str)]
        ) >> qi::eoi
    );

    if (!res)
        BOOST_LOG_THROW_DESCR(parse_error, "Failed to parse relation operand");

    return boost::move(f);
}

//  Explicitly instantiate factory implementation
#ifdef BOOST_LOG_USE_CHAR
template class default_filter_factory< char >;
#endif
#ifdef BOOST_LOG_USE_WCHAR_T
template class default_filter_factory< wchar_t >;
#endif

} // namespace aux

BOOST_LOG_CLOSE_NAMESPACE // namespace log

} // namespace boost

#include <boost/log/detail/footer.hpp>
