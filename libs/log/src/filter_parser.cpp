/*
 *          Copyright Andrey Semashev 2007 - 2013.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   filter_parser.cpp
 * \author Andrey Semashev
 * \date   31.03.2008
 *
 * \brief  This header is the Boost.Log library implementation, see the library documentation
 *         at http://www.boost.org/libs/log/doc/log.html.
 */

#ifndef BOOST_LOG_WITHOUT_SETTINGS_PARSERS

#include <cstddef>
#include <map>
#include <stack>
#include <string>
#include <sstream>
#include <stdexcept>
#include <utility>
#include <boost/assert.hpp>
#include <boost/bind.hpp>
#include <boost/none.hpp>
#include <boost/move/core.hpp>
#include <boost/move/utility.hpp>
#include <boost/optional/optional.hpp>
#include <boost/utility/in_place_factory.hpp>
#include <boost/spirit/include/qi_core.hpp>
#include <boost/spirit/include/qi_char.hpp>
#include <boost/spirit/include/qi_lit.hpp>
#include <boost/spirit/include/qi_raw.hpp>
#include <boost/spirit/include/qi_lexeme.hpp>
#include <boost/spirit/include/qi_as.hpp>
#include <boost/spirit/include/qi_symbols.hpp>
#include <boost/phoenix/core.hpp>
#include <boost/phoenix/bind/bind_function_object.hpp>
#include <boost/phoenix/operator/logical.hpp>
#include <boost/range/iterator_range_core.hpp>
#include <boost/log/detail/singleton.hpp>
#include <boost/log/exceptions.hpp>
#include <boost/log/utility/setup/filter_parser.hpp>
#if !defined(BOOST_LOG_NO_THREADS)
#include <boost/log/detail/locks.hpp>
#include <boost/log/detail/light_rw_mutex.hpp>
#endif // !defined(BOOST_LOG_NO_THREADS)
#include "parser_utils.hpp"
#include "default_filter_factory.hpp"
#include "spirit_encoding.hpp"
#include <boost/log/detail/header.hpp>

namespace qi = boost::spirit::qi;

namespace boost {

BOOST_LOG_OPEN_NAMESPACE

BOOST_LOG_ANONYMOUS_NAMESPACE {

//! Filter factories repository
template< typename CharT >
struct filters_repository :
    public log::aux::lazy_singleton< filters_repository< CharT > >
{
    typedef CharT char_type;
    typedef log::aux::lazy_singleton< filters_repository< char_type > > base_type;
    typedef std::basic_string< char_type > string_type;
    typedef filter_factory< char_type > filter_factory_type;

    //! Attribute name ordering predicate
    struct attribute_name_order
    {
        typedef bool result_type;
        result_type operator() (attribute_name const& left, attribute_name const& right) const
        {
            return left.id() < right.id();
        }
    };

    typedef std::map< attribute_name, shared_ptr< filter_factory_type >, attribute_name_order > factories_map;

#if !defined(BOOST_LOG_BROKEN_FRIEND_TEMPLATE_INSTANTIATIONS)
    friend class log::aux::lazy_singleton< filters_repository< char_type > >;
#else
    friend class base_type;
#endif

#if !defined(BOOST_LOG_NO_THREADS)
    //! Synchronization mutex
    mutable log::aux::light_rw_mutex m_Mutex;
#endif
    //! The map of filter factories
    factories_map m_Map;
    //! Default factory
    mutable aux::default_filter_factory< char_type > m_DefaultFactory;

    //! The method returns the filter factory for the specified attribute name
    filter_factory_type& get_factory(attribute_name const& name) const
    {
        typename factories_map::const_iterator it = m_Map.find(name);
        if (it != m_Map.end())
            return *it->second;
        else
            return m_DefaultFactory;
    }

private:
    filters_repository()
    {
    }
};

//! Filter parsing grammar
template< typename CharT >
class filter_grammar :
    public qi::grammar<
        const CharT*,
        typename log::aux::encoding_specific< typename log::aux::encoding< CharT >::type >::space_type
    >
{
private:
    typedef CharT char_type;
    typedef const char_type* iterator_type;
    typedef log::aux::encoding_specific< typename log::aux::encoding< char_type >::type > encoding_specific;
    typedef std::basic_string< char_type > string_type;
    typedef log::aux::char_constants< char_type > constants;
    typedef filter_grammar< char_type > this_type;
    typedef qi::grammar< iterator_type, typename encoding_specific::space_type > base_type;
    typedef typename base_type::start_type rule_type;
    typedef filter_factory< char_type > filter_factory_type;

    typedef filter (filter_factory_type::*comparison_relation_handler_t)(attribute_name const&, string_type const&);

private:
    //! Parsed attribute name
    mutable attribute_name m_AttributeName;
    //! The second operand of a relation
    mutable optional< string_type > m_Operand;
    //! Comparison relation handler
    comparison_relation_handler_t m_ComparisonRelation;
    //! The custom relation string
    mutable string_type m_CustomRelation;

    //! Filter subexpressions as they are parsed
    mutable std::stack< filter > m_Subexpressions;

    //! A parser for an attribute name in a single relation
    rule_type attr_name;
    //! A parser for a quoted string
    rule_type quoted_string_operand;
    //! A parser for an operand in a single relation
    rule_type operand;
    //! A parser for a single relation that consists of two operands and an operation between them
    rule_type relation;
    //! A set of comparison relation symbols with corresponding pointers to callbacks
    qi::symbols< char_type, comparison_relation_handler_t > comparison_relation;
    //! A parser for a custom relation word
    rule_type custom_relation;
    //! A parser for a term, which can be a relation, an expression in parenthesis or a negation thereof
    rule_type term;
    //! A parser for the complete filter expression that consists of one or several terms with boolean operations between them
    rule_type expression;

public:
    //! Constructor
    filter_grammar() :
        base_type(expression),
        m_ComparisonRelation(NULL)
    {
        attr_name = qi::lexeme
        [
            // An attribute name in form %name%
            qi::as< string_type >()[ qi::lit(constants::char_percent) >> +(encoding_specific::print - constants::char_percent) >> qi::lit(constants::char_percent) ]
                [boost::bind(&filter_grammar::on_attribute_name, this, _1)]
        ];

        quoted_string_operand = qi::raw
        [
            qi::lexeme
            [
                // A quoted string with C-style escape sequences support
                qi::lit(constants::char_quote) >>
                *(
                     (qi::lit(constants::char_backslash) >> qi::char_) |
                     (qi::char_ - qi::lit(constants::char_quote))
                ) >>
                qi::lit(constants::char_quote)
            ]
        ]
            [boost::bind(&filter_grammar::on_quoted_string_operand, this, _1)];

        operand =
        (
            quoted_string_operand |
            // A single word, enclosed with white spaces. It cannot contain parenthesis, since it is used by the filter parser.
            qi::raw[ qi::lexeme[ +(encoding_specific::graph - qi::lit(constants::char_paren_bracket_left) - qi::lit(constants::char_paren_bracket_right)) ] ]
                [boost::bind(&filter_grammar::on_operand, this, _1)]
        );

        // Custom relation is a keyword that may contain either alphanumeric characters or an underscore
        custom_relation = qi::as< string_type >()[ qi::lexeme[ +(encoding_specific::alnum | qi::char_(constants::char_underline)) ] ]
            [boost::bind(&filter_grammar::set_custom_relation, this, _1)];

        comparison_relation.add
            (constants::equal_keyword(), &filter_factory_type::on_equality_relation)
            (constants::not_equal_keyword(), &filter_factory_type::on_inequality_relation)
            (constants::greater_keyword(), &filter_factory_type::on_greater_relation)
            (constants::less_keyword(), &filter_factory_type::on_less_relation)
            (constants::greater_or_equal_keyword(), &filter_factory_type::on_greater_or_equal_relation)
            (constants::less_or_equal_keyword(), &filter_factory_type::on_less_or_equal_relation);

        relation =
        (
            attr_name >> // The relation may be as simple as a sole attribute name, in which case the filter checks for the attribute value presence
            -(
                (comparison_relation[boost::bind(&filter_grammar::set_comparison_relation, this, _1)] >> operand) |
                (custom_relation >> operand)
            )
        )
            [boost::bind(&filter_grammar::on_relation_complete, this)];

        term =
        (
            (qi::lit(constants::char_paren_bracket_left) >> expression >> constants::char_paren_bracket_right) |
            ((qi::lit(constants::not_keyword()) | constants::char_exclamation) >> term)[boost::bind(&filter_grammar::on_negation, this)] |
            relation
        );

        expression =
        (
            term >>
            *(
                ((qi::lit(constants::and_keyword()) | constants::char_and) >> term)[boost::bind(&filter_grammar::on_and, this)] |
                ((qi::lit(constants::or_keyword()) | constants::char_or) >> term)[boost::bind(&filter_grammar::on_or, this)]
            )
        );
    }

    //! The method returns the constructed filter
    filter get_filter()
    {
        BOOST_ASSERT(!m_Subexpressions.empty());
        return boost::move(m_Subexpressions.top());
    }

private:
    //! The attribute name handler
    void on_attribute_name(string_type const& name)
    {
        m_AttributeName = attribute_name(log::aux::to_narrow(name));
    }

    //! The operand string handler
    void on_operand(iterator_range< iterator_type > const& arg)
    {
        // An attribute name should have been parsed at this point
        if (!m_AttributeName)
            BOOST_LOG_THROW_DESCR(parse_error, "Invalid filter definition: operand is not expected");

        m_Operand = boost::in_place(arg.begin(), arg.end());
    }

    //! The quoted string handler
    void on_quoted_string_operand(iterator_range< iterator_type > const& arg)
    {
        // An attribute name should have been parsed at this point
        if (!m_AttributeName)
            BOOST_LOG_THROW_DESCR(parse_error, "Invalid filter definition: quoted string operand is not expected");

        // Cut off the quotes
        string_type str(arg.begin() + 1, arg.end() - 1);

        // Translate escape sequences
        constants::translate_escape_sequences(str);
        m_Operand = str;
    }

    //! The method saves the relation word into an internal string
    void set_comparison_relation(comparison_relation_handler_t rel)
    {
        m_ComparisonRelation = rel;
    }

    //! The method saves the relation word into an internal string
    void set_custom_relation(string_type const& rel)
    {
        m_CustomRelation = rel;
    }

    //! The comparison relation handler
    void on_relation_complete()
    {
        if (!!m_AttributeName)
        {
            filters_repository< char_type > const& repo = filters_repository< char_type >::get();
            filter_factory_type& factory = repo.get_factory(m_AttributeName);

            if (!!m_Operand)
            {
                if (!!m_ComparisonRelation)
                {
                    m_Subexpressions.push((factory.*m_ComparisonRelation)(m_AttributeName, m_Operand.get()));
                    m_ComparisonRelation = NULL;
                }
                else if (!m_CustomRelation.empty())
                {
                    m_Subexpressions.push(factory.on_custom_relation(m_AttributeName, m_CustomRelation, m_Operand.get()));
                    m_CustomRelation.clear();
                }
                else
                {
                    // This should never happen
                    BOOST_ASSERT_MSG(false, "Filter parser internal error: the attribute name or subexpression operation is not set while trying to construct a relation");
                    BOOST_LOG_THROW_DESCR(parse_error, "Filter parser internal error: the attribute name or subexpression operation is not set while trying to construct a subexpression");
                }

                m_Operand = none;
            }
            else
            {
                // This branch is taken if the relation is a single attribute name, which is recognized as the attribute presence check
                BOOST_ASSERT_MSG(!m_ComparisonRelation && m_CustomRelation.empty(), "Filter parser internal error: the relation operation is set while operand is not");
                m_Subexpressions.push(factory.on_exists_test(m_AttributeName));
            }

            m_AttributeName = attribute_name();
        }
        else
        {
            // This should never happen
            BOOST_ASSERT_MSG(false, "Filter parser internal error: the attribute name is not set while trying to construct a relation");
            BOOST_LOG_THROW_DESCR(parse_error, "Filter parser internal error: the attribute name is not set while trying to construct a relation");
        }
    }

    //! The negation operation handler
    void on_negation()
    {
        if (!m_Subexpressions.empty())
        {
            m_Subexpressions.top() = !phoenix::bind(m_Subexpressions.top(), phoenix::placeholders::_1);
        }
        else
        {
            // This would happen if a filter consists of a single '!'
            BOOST_LOG_THROW_DESCR(parse_error, "Filter parsing error: a negation operator applied to nothingness");
        }
    }

    //! The logical AND operation handler
    void on_and()
    {
        if (!m_Subexpressions.empty())
        {
            filter right = boost::move(m_Subexpressions.top());
            m_Subexpressions.pop();
            if (!m_Subexpressions.empty())
            {
                filter const& left = m_Subexpressions.top();
                m_Subexpressions.top() = phoenix::bind(left, phoenix::placeholders::_1) && phoenix::bind(right, phoenix::placeholders::_1);
                return;
            }
        }

        // This should never happen
        BOOST_LOG_THROW_DESCR(parse_error, "Filter parser internal error: the subexpression is not set while trying to construct a filter");
    }

    //! The logical OR operation handler
    void on_or()
    {
        if (!m_Subexpressions.empty())
        {
            filter right = boost::move(m_Subexpressions.top());
            m_Subexpressions.pop();
            if (!m_Subexpressions.empty())
            {
                filter const& left = m_Subexpressions.top();
                m_Subexpressions.top() = phoenix::bind(left, phoenix::placeholders::_1) || phoenix::bind(right, phoenix::placeholders::_1);
                return;
            }
        }

        // This should never happen
        BOOST_LOG_THROW_DESCR(parse_error, "Filter parser internal error: the subexpression is not set while trying to construct a filter");
    }

    //  Assignment and copying are prohibited
    BOOST_LOG_DELETED_FUNCTION(filter_grammar(filter_grammar const&))
    BOOST_LOG_DELETED_FUNCTION(filter_grammar& operator= (filter_grammar const&))
};

} // namespace

//! The function registers a filter factory object for the specified attribute name
template< typename CharT >
void register_filter_factory(attribute_name const& name, shared_ptr< filter_factory< CharT > > const& factory)
{
    BOOST_ASSERT(!!name);
    BOOST_ASSERT(!!factory);

    filters_repository< CharT >& repo = filters_repository< CharT >::get();

    BOOST_LOG_EXPR_IF_MT(log::aux::exclusive_lock_guard< log::aux::light_rw_mutex > lock(repo.m_Mutex);)
    repo.m_Map[name] = factory;
}

//! The function parses a filter from the string
template< typename CharT >
filter parse_filter(const CharT* begin, const CharT* end)
{
    typedef CharT char_type;
    typedef log::aux::encoding_specific< typename log::aux::encoding< char_type >::type > encoding_specific;

    filter_grammar< char_type > gram;
    const char_type* p = begin;

    BOOST_LOG_EXPR_IF_MT(filters_repository< CharT >& repo = filters_repository< CharT >::get();)
    BOOST_LOG_EXPR_IF_MT(log::aux::shared_lock_guard< log::aux::light_rw_mutex > lock(repo.m_Mutex);)

    bool result = qi::phrase_parse(p, end, gram, encoding_specific::space);
    if (!result || p != end)
    {
        std::ostringstream strm;
        strm << "Could not parse the filter, parsing stopped at position " << p - begin;
        BOOST_LOG_THROW_DESCR(parse_error, strm.str());
    }

    return gram.get_filter();
}

#ifdef BOOST_LOG_USE_CHAR

template BOOST_LOG_SETUP_API
void register_filter_factory< char >(attribute_name const& name, shared_ptr< filter_factory< char > > const& factory);
template BOOST_LOG_SETUP_API
filter parse_filter< char >(const char* begin, const char* end);

#endif // BOOST_LOG_USE_CHAR

#ifdef BOOST_LOG_USE_WCHAR_T

template BOOST_LOG_SETUP_API
void register_filter_factory< wchar_t >(attribute_name const& name, shared_ptr< filter_factory< wchar_t > > const& factory);
template BOOST_LOG_SETUP_API
filter parse_filter< wchar_t >(const wchar_t* begin, const wchar_t* end);

#endif // BOOST_LOG_USE_WCHAR_T

BOOST_LOG_CLOSE_NAMESPACE // namespace log

} // namespace boost

#include <boost/log/detail/footer.hpp>

#endif // BOOST_LOG_WITHOUT_SETTINGS_PARSERS
