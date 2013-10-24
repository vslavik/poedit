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
 *         at http://www.boost.org/doc/libs/release/libs/log/doc/html/index.html.
 */

#if !defined(BOOST_LOG_WITHOUT_SETTINGS_PARSERS) && !defined(BOOST_LOG_WITHOUT_DEFAULT_FACTORIES)

#include <string>
#include <boost/move/core.hpp>
#include <boost/move/utility.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/mpl/copy.hpp>
#include <boost/mpl/contains.hpp>
#include <boost/mpl/back_inserter.hpp>
#include <boost/spirit/include/qi_core.hpp>
#include <boost/spirit/include/qi_eoi.hpp>
#include <boost/spirit/include/qi_as.hpp>
#include <boost/log/exceptions.hpp>
#include <boost/log/expressions/predicates/has_attr.hpp>
#include <boost/log/utility/type_dispatch/standard_types.hpp>
#include <boost/log/utility/string_literal.hpp>
#include <boost/log/utility/functional/logical.hpp>
#include <boost/log/utility/functional/begins_with.hpp>
#include <boost/log/utility/functional/ends_with.hpp>
#include <boost/log/utility/functional/contains.hpp>
#include <boost/log/utility/functional/as_action.hpp>
#include <boost/log/detail/code_conversion.hpp>
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

BOOST_LOG_ANONYMOUS_NAMESPACE {

#if defined(BOOST_LOG_USE_CHAR) && defined(BOOST_LOG_USE_WCHAR_T)

//! A special filtering predicate that adopts the string operand to the attribute value character type
template< typename RelationT >
class string_predicate :
    public RelationT
{
private:
    template< typename StringT >
    struct initializer
    {
        typedef void result_type;

        explicit initializer(StringT const& val) : m_initializer(val)
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
        StringT const& m_initializer;
    };

public:
    typedef RelationT relation_type;
    typedef typename relation_type::result_type result_type;

    template< typename StringT >
    string_predicate(relation_type const& rel, StringT const& operand) : relation_type(rel)
    {
        fusion::for_each(m_operands, initializer< StringT >(operand));
    }

    template< typename T >
    typename boost::enable_if<
        mpl::contains< log::string_types::type, T >,
        result_type
    >::type operator() (T const& val) const
    {
        typedef std::basic_string< typename T::value_type > operand_type;
        return relation_type::operator() (val, fusion::at_key< operand_type >(m_operands));
    }

private:
    fusion::set< std::string, std::wstring > m_operands;
};

#else // defined(BOOST_LOG_USE_CHAR) && defined(BOOST_LOG_USE_WCHAR_T)

//! A special filtering predicate that adopts the string operand to the attribute value character type
template< typename RelationT >
class string_predicate :
    public RelationT
{
private:
#if defined(BOOST_LOG_USE_CHAR)
    typedef std::basic_string< char > string_type;
#elif defined(BOOST_LOG_USE_WCHAR_T)
    typedef std::basic_string< wchar_t > string_type;
#else
#error Boost.Log: Inconsistent character configuration
#endif

public:
    typedef RelationT relation_type;
    typedef typename relation_type::result_type result_type;

public:
    string_predicate(relation_type const& rel, string_type const& operand) : relation_type(rel), m_operand(operand)
    {
    }

    template< typename T >
    result_type operator() (T const& val) const
    {
        return relation_type::operator() (val, m_operand);
    }

private:
    const string_type m_operand;
};

#endif // defined(BOOST_LOG_USE_CHAR) && defined(BOOST_LOG_USE_WCHAR_T)

//! A filtering predicate for numeric relations
template< typename NumericT, typename RelationT >
class numeric_predicate :
    public string_predicate< RelationT >
{
    typedef string_predicate< RelationT > base_type;

public:
    typedef NumericT numeric_type;
    typedef typename base_type::relation_type relation_type;
    typedef typename base_type::result_type result_type;

public:
    template< typename StringT >
    numeric_predicate(relation_type const& rel, StringT const& string_operand, numeric_type numeric_operand) :
        base_type(rel, string_operand),
        m_numeric_operand(numeric_operand)
    {
    }

    template< typename T >
    typename boost::enable_if<
        mpl::contains< log::string_types, T >,
        result_type
    >::type operator() (T const& val) const
    {
        return base_type::operator() (val);
    }

    template< typename T >
    typename boost::disable_if<
        mpl::contains< log::string_types, T >,
        result_type
    >::type operator() (T const& val) const
    {
        return relation_type::operator() (val, m_numeric_operand);
    }

private:
    const numeric_type m_numeric_operand;
};


template< typename RelationT >
struct on_string_argument
{
    typedef void result_type;

    on_string_argument(attribute_name const& name, filter& f) : m_name(name), m_filter(f)
    {
    }

    template< typename StringT >
    result_type operator() (StringT const& val) const
    {
        typedef string_predicate< RelationT > predicate;
        m_filter = predicate_wrapper< log::string_types, predicate >(m_name, predicate(RelationT(), val));
    }

private:
    attribute_name m_name;
    filter& m_filter;
};

template< typename RelationT, typename StringT >
struct on_integral_argument
{
    typedef void result_type;

    on_integral_argument(attribute_name const& name, StringT const& operand, filter& f) : m_name(name), m_operand(operand), m_filter(f)
    {
    }

    result_type operator() (long val) const
    {
        typedef numeric_predicate< long, RelationT > predicate;
        typedef mpl::copy<
            log::string_types,
            mpl::back_inserter< log::numeric_types >
        >::type value_types;
        m_filter = predicate_wrapper< value_types, predicate >(m_name, predicate(RelationT(), m_operand, val));
    }

private:
    attribute_name m_name;
    StringT const& m_operand;
    filter& m_filter;
};

template< typename RelationT, typename StringT >
struct on_fp_argument
{
    typedef void result_type;

    on_fp_argument(attribute_name const& name, StringT const& operand, filter& f) : m_name(name), m_operand(operand), m_filter(f)
    {
    }

    result_type operator() (double val) const
    {
        typedef numeric_predicate< double, RelationT > predicate;
        typedef mpl::copy<
            log::string_types,
            mpl::back_inserter< log::floating_point_types >
        >::type value_types;
        m_filter = predicate_wrapper< value_types, predicate >(m_name, predicate(RelationT(), m_operand, val));
    }

private:
    attribute_name m_name;
    StringT const& m_operand;
    filter& m_filter;
};

} // namespace

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
        f = parse_matches_relation(name, arg);
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
    const on_fp_argument< RelationT, string_type > on_fp(name, arg, f);
    const on_integral_argument< RelationT, string_type > on_int(name, arg, f);
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

#endif // !defined(BOOST_LOG_WITHOUT_SETTINGS_PARSERS) && !defined(BOOST_LOG_WITHOUT_DEFAULT_FACTORIES)
