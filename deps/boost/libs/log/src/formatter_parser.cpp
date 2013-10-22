/*
 *          Copyright Andrey Semashev 2007 - 2013.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   formatter_parser.cpp
 * \author Andrey Semashev
 * \date   07.04.2008
 *
 * \brief  This header is the Boost.Log library implementation, see the library documentation
 *         at http://www.boost.org/libs/log/doc/log.html.
 */

#ifndef BOOST_LOG_WITHOUT_SETTINGS_PARSERS

#undef BOOST_MPL_LIMIT_VECTOR_SIZE
#define BOOST_MPL_LIMIT_VECTOR_SIZE 50

#include <ctime>
#include <map>
#include <string>
#include <sstream>
#include <stdexcept>
#include <boost/assert.hpp>
#include <boost/bind.hpp>
#include <boost/move/core.hpp>
#include <boost/move/utility.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/mpl/copy.hpp>
#include <boost/mpl/push_back.hpp>
#include <boost/mpl/back_inserter.hpp>
#include <boost/optional/optional.hpp>
#include <boost/utility/in_place_factory.hpp>
#include <boost/range/iterator_range_core.hpp>
#include <boost/spirit/include/qi_core.hpp>
#include <boost/spirit/include/qi_char.hpp>
#include <boost/spirit/include/qi_lit.hpp>
#include <boost/spirit/include/qi_raw.hpp>
#include <boost/spirit/include/qi_lexeme.hpp>
#include <boost/spirit/include/qi_as.hpp>
#include <boost/spirit/include/qi_symbols.hpp>
#include <boost/phoenix/core.hpp>
#include <boost/phoenix/operator.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time/local_time/local_time.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/log/expressions/attr.hpp>
#include <boost/log/expressions/message.hpp>
#include <boost/log/expressions/formatters/stream.hpp>
#include <boost/log/attributes/attribute_name.hpp>
#include <boost/log/attributes/named_scope.hpp>
#include <boost/log/exceptions.hpp>
#include <boost/log/detail/singleton.hpp>
#include <boost/log/detail/process_id.hpp>
#include <boost/log/detail/code_conversion.hpp>
#include <boost/log/detail/default_attribute_names.hpp>
#include <boost/log/utility/functional/nop.hpp>
#include <boost/log/utility/setup/formatter_parser.hpp>
#include <boost/log/utility/type_dispatch/standard_types.hpp>
#include <boost/log/utility/type_dispatch/date_time_types.hpp>
#if !defined(BOOST_LOG_NO_THREADS)
#include <boost/log/detail/thread_id.hpp>
#include <boost/log/detail/locks.hpp>
#include <boost/log/detail/light_rw_mutex.hpp>
#endif
#include "parser_utils.hpp"
#include "spirit_encoding.hpp"
#include <boost/log/detail/header.hpp>

namespace qi = boost::spirit::qi;

namespace boost {

BOOST_LOG_OPEN_NAMESPACE

BOOST_LOG_ANONYMOUS_NAMESPACE {

//! The structure contains formatter factories repository
template< typename CharT >
struct formatters_repository :
    public log::aux::lazy_singleton< formatters_repository< CharT > >
{
    //! Base class type
    typedef log::aux::lazy_singleton< formatters_repository< CharT > > base_type;

#if !defined(BOOST_LOG_BROKEN_FRIEND_TEMPLATE_INSTANTIATIONS)
    friend class log::aux::lazy_singleton< formatters_repository< CharT > >;
#else
    friend class base_type;
#endif

    typedef formatter_factory< CharT > formatter_factory_type;

    //! Attribute name ordering predicate
    struct attribute_name_order
    {
        typedef bool result_type;
        result_type operator() (attribute_name const& left, attribute_name const& right) const
        {
            return left.id() < right.id();
        }
    };

    //! Map of formatter factories
    typedef std::map< attribute_name, shared_ptr< formatter_factory_type >, attribute_name_order > factories_map;


#if !defined(BOOST_LOG_NO_THREADS)
    //! Synchronization mutex
    mutable log::aux::light_rw_mutex m_Mutex;
#endif
    //! The map of formatter factories
    factories_map m_Map;

private:
    formatters_repository()
    {
    }
};

//! Function object for formatter chaining
template< typename CharT, typename SecondT >
struct chained_formatter
{
    typedef void result_type;
    typedef basic_formatter< CharT > formatter_type;
    typedef typename formatter_type::stream_type stream_type;

#if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
    explicit chained_formatter(formatter_type&& first, SecondT&& second) :
#else
    template< typename T >
    explicit chained_formatter(BOOST_RV_REF(formatter_type) first, T const& second) :
#endif
        m_first(boost::move(first)), m_second(boost::move(second))
    {
    }

    result_type operator() (record_view const& rec, stream_type& strm) const
    {
        m_first(rec, strm);
        m_second(rec, strm);
    }

private:
    formatter_type m_first;
    SecondT m_second;
};

//! Formatter parsing grammar
template< typename CharT >
class formatter_grammar :
    public qi::grammar< const CharT* >
{
private:
    typedef CharT char_type;
    typedef const char_type* iterator_type;
    typedef qi::grammar< iterator_type > base_type;
    typedef typename base_type::start_type rule_type;
    typedef std::basic_string< char_type > string_type;
    typedef basic_formatter< char_type > formatter_type;
    typedef boost::log::aux::char_constants< char_type > constants;
    typedef log::aux::encoding_specific< typename log::aux::encoding< char_type >::type > encoding_specific;
    typedef formatter_factory< char_type > formatter_factory_type;
    typedef typename formatter_factory_type::args_map args_map;

private:
    //! The formatter being constructed
    optional< formatter_type > m_Formatter;

    //! Attribute name
    attribute_name m_AttrName;
    //! Formatter factory arguments
    args_map m_FactoryArgs;

    //! Formatter argument name
    mutable string_type m_ArgName;
    //! Argument value
    mutable string_type m_ArgValue;

    //! A parser for a quoted string argument value
    rule_type quoted_string_arg_value;
    //! A parser for an argument value
    rule_type arg_value;
    //! A parser for a named argument
    rule_type arg;
    //! A parser for an optional argument list for a formatter
    rule_type arg_list;
    //! A parser for an attribute placeholder
    rule_type attr_name;
    //! A parser for the complete formatter expression
    rule_type expression;

public:
    //! Constructor
    formatter_grammar() :
        base_type(expression)
    {
        quoted_string_arg_value = qi::raw
        [
            // A quoted string with C-style escape sequences support
            qi::lit(constants::char_quote) >>
            *(
                 (qi::lit(constants::char_backslash) >> qi::char_) |
                 (qi::char_ - qi::lit(constants::char_quote))
            ) >>
            qi::lit(constants::char_quote)
        ]
            [boost::bind(&formatter_grammar::on_quoted_string_arg_value, this, _1)];

        arg_value =
        (
            quoted_string_arg_value |
            qi::raw[ *(encoding_specific::graph - constants::char_comma - constants::char_paren_bracket_left - constants::char_paren_bracket_right) ]
                [boost::bind(&formatter_grammar::on_arg_value, this, _1)]
        );

        arg =
        (
            *encoding_specific::space >>
            qi::raw[ encoding_specific::alpha >> *encoding_specific::alnum ][boost::bind(&formatter_grammar::on_arg_name, this, _1)] >>
            *encoding_specific::space >>
            constants::char_equal >>
            *encoding_specific::space >>
            arg_value >>
            *encoding_specific::space
        );

        arg_list =
        (
            qi::lit(constants::char_paren_bracket_left) >>
            (arg[boost::bind(&formatter_grammar::push_arg, this)] % qi::lit(constants::char_comma)) >>
            qi::lit(constants::char_paren_bracket_right)
        );

        attr_name =
        (
            qi::lit(constants::char_percent) >>
            (
                qi::raw[ qi::lit(constants::message_text_keyword()) ]
                    [boost::bind(&formatter_grammar::on_attr_name, this, _1)] |
                (
                    qi::raw[ +(encoding_specific::print - constants::char_paren_bracket_left - constants::char_percent) ]
                        [boost::bind(&formatter_grammar::on_attr_name, this, _1)] >>
                    -arg_list
                )
            ) >>
            qi::lit(constants::char_percent)
        )
            [boost::bind(&formatter_grammar::push_attr, this)];

        expression =
        (
            qi::raw
            [
                *(
                    (qi::lit(constants::char_backslash) >> qi::char_) |
                    (qi::char_ - qi::lit(constants::char_quote) - qi::lit(constants::char_percent))
                )
            ][boost::bind(&formatter_grammar::push_string, this, _1)] %
            attr_name
        );
    }

    //! Returns the parsed formatter
    formatter_type get_formatter()
    {
        if (!m_Formatter)
        {
            // This may happen if parser input is an empty string
            return formatter_type(nop());
        }

        return boost::move(m_Formatter.get());
    }

private:
    //! The method is called when an argument name is discovered
    void on_arg_name(iterator_range< iterator_type > const& name)
    {
        m_ArgName.assign(name.begin(), name.end());
    }
    //! The method is called when an argument value is discovered
    void on_quoted_string_arg_value(iterator_range< iterator_type > const& value)
    {
        // Cut off the quotes
        m_ArgValue.assign(value.begin() + 1, value.end() - 1);
        constants::translate_escape_sequences(m_ArgValue);
    }
    //! The method is called when an argument value is discovered
    void on_arg_value(iterator_range< iterator_type > const& value)
    {
        m_ArgValue.assign(value.begin(), value.end());
    }
    //! The method is called when an argument is filled
    void push_arg()
    {
        m_FactoryArgs[m_ArgName] = m_ArgValue;
        m_ArgName.clear();
        m_ArgValue.clear();
    }

    //! The method is called when an attribute name is discovered
    void on_attr_name(iterator_range< iterator_type > const& name)
    {
        if (name.empty())
            BOOST_LOG_THROW_DESCR(parse_error, "Empty attribute name encountered");

        // For compatibility with Boost.Log v1 we recognize %_% as the message attribute name
        if (std::char_traits< char_type >::compare(constants::message_text_keyword(), name.begin(), name.size()) == 0)
            m_AttrName = log::aux::default_attribute_names::message();
        else
            m_AttrName = attribute_name(log::aux::to_narrow(string_type(name.begin(), name.end())));
    }
    //! The method is called when an attribute is filled
    void push_attr()
    {
        BOOST_ASSERT_MSG(!!m_AttrName, "Attribute name is not set");

        if (m_AttrName == log::aux::default_attribute_names::message())
        {
            // We make a special treatment for the message text formatter
            append_formatter(expressions::stream << expressions::message);
        }
        else
        {
            formatters_repository< char_type > const& repo = formatters_repository< char_type >::get();
            typename formatters_repository< char_type >::factories_map::const_iterator it = repo.m_Map.find(m_AttrName);
            if (it != repo.m_Map.end())
            {
                // We've found a user-defined factory for this attribute
                append_formatter(it->second->create_formatter(m_AttrName, m_FactoryArgs));
            }
            else
            {
                // No user-defined factory, shall use the most generic formatter we can ever imagine at this point
                typedef mpl::copy<
                    // We have to exclude std::time_t since it's an integral type and will conflict with one of the standard types
                    boost_time_period_types,
                    mpl::back_inserter<
                        mpl::copy<
                            boost_time_duration_types,
                            mpl::back_inserter< boost_date_time_types >
                        >::type
                    >
                >::type time_related_types;

                typedef mpl::copy<
                    mpl::copy<
                        mpl::vector<
                            attributes::named_scope_list,
#if !defined(BOOST_LOG_NO_THREADS)
                            log::aux::thread::id,
#endif
                            log::aux::process::id
                        >,
                        mpl::back_inserter< time_related_types >
                    >::type,
                    mpl::back_inserter< default_attribute_types >
                >::type supported_types;

                append_formatter(expressions::stream << expressions::attr< supported_types::type >(m_AttrName));
            }
        }

        // Eventually, clear all the auxiliary data
        m_AttrName = attribute_name();
        m_FactoryArgs.clear();
    }

    //! The method is called when a string literal is discovered
    void push_string(iterator_range< iterator_type > const& str)
    {
        if (!str.empty())
        {
            string_type s(str.begin(), str.end());
            constants::translate_escape_sequences(s);
            append_formatter(expressions::stream << s);
        }
    }

    //! The method appends a formatter part to the final formatter
    template< typename FormatterT >
    void append_formatter(FormatterT fmt)
    {
        if (!!m_Formatter)
            m_Formatter = boost::in_place(chained_formatter< char_type, FormatterT >(boost::move(m_Formatter.get()), boost::move(fmt)));
        else
            m_Formatter = boost::in_place(boost::move(fmt));
    }

    //  Assignment and copying are prohibited
    BOOST_LOG_DELETED_FUNCTION(formatter_grammar(formatter_grammar const&))
    BOOST_LOG_DELETED_FUNCTION(formatter_grammar& operator= (formatter_grammar const&))
};

} // namespace

//! The function registers a user-defined formatter factory
template< typename CharT >
void register_formatter_factory(attribute_name const& name, shared_ptr< formatter_factory< CharT > > const& factory)
{
    BOOST_ASSERT(!!name);
    BOOST_ASSERT(!!factory);

    formatters_repository< CharT >& repo = formatters_repository< CharT >::get();
    BOOST_LOG_EXPR_IF_MT(log::aux::exclusive_lock_guard< log::aux::light_rw_mutex > _(repo.m_Mutex);)
    repo.m_Map[name] = factory;
}

//! The function parses a formatter from the string
template< typename CharT >
basic_formatter< CharT > parse_formatter(const CharT* begin, const CharT* end)
{
    typedef CharT char_type;

    formatter_grammar< char_type > gram;
    const char_type* p = begin;

    BOOST_LOG_EXPR_IF_MT(formatters_repository< CharT >& repo = formatters_repository< CharT >::get();)
    BOOST_LOG_EXPR_IF_MT(log::aux::shared_lock_guard< log::aux::light_rw_mutex > _(repo.m_Mutex);)

    bool result = qi::parse(p, end, gram);
    if (!result || p != end)
    {
        std::ostringstream strm;
        strm << "Could not parse the formatter, parsing stopped at position " << p - begin;
        BOOST_LOG_THROW_DESCR(parse_error, strm.str());
    }

    return gram.get_formatter();
}

#ifdef BOOST_LOG_USE_CHAR
template BOOST_LOG_SETUP_API
void register_formatter_factory< char >(
    attribute_name const& attr_name, shared_ptr< formatter_factory< char > > const& factory);
template BOOST_LOG_SETUP_API
basic_formatter< char > parse_formatter< char >(const char* begin, const char* end);
#endif // BOOST_LOG_USE_CHAR

#ifdef BOOST_LOG_USE_WCHAR_T
template BOOST_LOG_SETUP_API
void register_formatter_factory< wchar_t >(
    attribute_name const& attr_name, shared_ptr< formatter_factory< wchar_t > > const& factory);
template BOOST_LOG_SETUP_API
basic_formatter< wchar_t > parse_formatter< wchar_t >(const wchar_t* begin, const wchar_t* end);
#endif // BOOST_LOG_USE_WCHAR_T

BOOST_LOG_CLOSE_NAMESPACE // namespace log

} // namespace boost

#include <boost/log/detail/footer.hpp>

#endif // BOOST_LOG_WITHOUT_SETTINGS_PARSERS
