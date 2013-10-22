/*
 *          Copyright Andrey Semashev 2007 - 2013.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   settings_parser.cpp
 * \author Andrey Semashev
 * \date   20.07.2012
 *
 * \brief  This header is the Boost.Log library implementation, see the library documentation
 *         at http://www.boost.org/libs/log/doc/log.html.
 */

#ifndef BOOST_LOG_WITHOUT_SETTINGS_PARSERS

#include <string>
#include <iostream>
#include <locale>
#include <memory>
#include <stdexcept>
#include <boost/ref.hpp>
#include <boost/bind.hpp>
#include <boost/throw_exception.hpp>
#include <boost/io/ios_state.hpp>
#include <boost/move/core.hpp>
#include <boost/move/utility.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/spirit/include/qi_core.hpp>
#include <boost/spirit/include/qi_char.hpp>
#include <boost/spirit/include/qi_lit.hpp>
#include <boost/spirit/include/qi_eoi.hpp>
#include <boost/spirit/include/qi_eol.hpp>
#include <boost/spirit/include/qi_raw.hpp>
#include <boost/spirit/include/qi_lexeme.hpp>
#include <boost/range/iterator_range_core.hpp>
#include <boost/log/detail/config.hpp>
#include <boost/log/detail/code_conversion.hpp>
#include <boost/log/exceptions.hpp>
#include <boost/log/utility/setup/settings_parser.hpp>
#include "parser_utils.hpp"
#include "spirit_encoding.hpp"
#include <boost/log/detail/header.hpp>

namespace qi = boost::spirit::qi;

namespace boost {

BOOST_LOG_OPEN_NAMESPACE

BOOST_LOG_ANONYMOUS_NAMESPACE {

//! Settings parsing grammar
template< typename CharT >
class settings_grammar :
    public qi::grammar<
        const CharT*,
        typename log::aux::encoding_specific< typename log::aux::encoding< CharT >::type >::space_type
    >
{
private:
    typedef CharT char_type;
    typedef const char_type* iterator_type;
    typedef log::aux::encoding_specific< typename log::aux::encoding< char_type >::type > encoding_specific;
    typedef settings_grammar< char_type > this_type;
    typedef qi::grammar< iterator_type, typename encoding_specific::space_type > base_type;
    typedef typename base_type::start_type rule_type;

    typedef std::basic_string< char_type > string_type;
    typedef log::aux::char_constants< char_type > constants;
    typedef basic_settings< char_type > settings_type;

private:
    //! Current section name
    std::string m_SectionName;
    //! Current parameter name
    std::string m_ParameterName;
    //! Settings instance
    settings_type& m_Settings;
    //! Locale from the source stream
    std::locale m_Locale;
    //! Current line number
    unsigned int& m_LineCounter;

    //! A parser for a comment
    rule_type comment;
    //! A parser for a section name
    rule_type section_name;
    //! A parser for a quoted string
    rule_type quoted_string;
    //! A parser for a parameter name and value
    rule_type parameter;
    //! A parser for a single line
    rule_type line;

public:
    //! Constructor
    explicit settings_grammar(settings_type& setts, unsigned int& line_counter, std::locale const& loc) :
        base_type(line, "settings_grammar"),
        m_Settings(setts),
        m_Locale(loc),
        m_LineCounter(line_counter)
    {
        comment = qi::lit(constants::char_comment) >> *qi::char_;

        section_name =
            qi::raw[ qi::lit(constants::char_section_bracket_left) >> +(encoding_specific::graph - constants::char_section_bracket_right) >> constants::char_section_bracket_right ]
                [boost::bind(&this_type::set_section_name, this, _1)] >>
            -comment;

        quoted_string = qi::lexeme
        [
            qi::lit(constants::char_quote) >>
            *(
                 (qi::lit(constants::char_backslash) >> qi::char_) |
                 (qi::char_ - qi::lit(constants::char_quote))
            ) >>
            qi::lit(constants::char_quote)
        ];

        parameter =
            // Parameter name
            qi::raw[ qi::lexeme[ encoding_specific::alpha >> *(encoding_specific::graph - constants::char_equal) ] ]
                [boost::bind(&this_type::set_parameter_name, this, _1)] >>
            qi::lit(constants::char_equal) >>
            // Parameter value
            (
                qi::raw[ quoted_string ][boost::bind(&this_type::set_parameter_quoted_value, this, _1)] |
                qi::raw[ +encoding_specific::graph ][boost::bind(&this_type::set_parameter_value, this, _1)]
            ) >>
            -comment;

        line = (comment | section_name | parameter) >> qi::eoi;
    }

private:
    //! The method sets the parsed section name
    void set_section_name(iterator_range< iterator_type > const& sec)
    {
        // Trim square brackets
        m_SectionName = log::aux::to_narrow(string_type(sec.begin() + 1, sec.end() - 1), m_Locale);
        algorithm::trim(m_SectionName, m_Locale);
        if (m_SectionName.empty())
        {
            // The section starter is broken
            BOOST_LOG_THROW_DESCR_PARAMS(parse_error, "The section header is invalid.", (m_LineCounter));
        }

        // For compatibility with Boost.Log v1, we replace the "Sink:" prefix with "Sinks."
        // so that all sink parameters are placed in the common Sinks section.
        if (m_SectionName.compare(0, 5, "Sink:") == 0)
            m_SectionName = "Sinks." + m_SectionName.substr(5);
    }

    //! The method sets the parsed parameter name
    void set_parameter_name(iterator_range< iterator_type > const& name)
    {
        if (m_SectionName.empty())
        {
            // The parameter encountered before any section starter
            BOOST_LOG_THROW_DESCR_PARAMS(parse_error, "Parameters are only allowed within sections.", (m_LineCounter));
        }

        m_ParameterName = log::aux::to_narrow(string_type(name.begin(), name.end()), m_Locale);
    }

    //! The method sets the parsed parameter value (non-quoted)
    void set_parameter_value(iterator_range< iterator_type > const& value)
    {
        string_type val(value.begin(), value.end());
        m_Settings[m_SectionName][m_ParameterName] = val;
        m_ParameterName.clear();
    }

    //! The method sets the parsed parameter value (quoted)
    void set_parameter_quoted_value(iterator_range< iterator_type > const& value)
    {
        // Cut off the quotes
        string_type val(value.begin() + 1, value.end() - 1);
        constants::translate_escape_sequences(val);
        m_Settings[m_SectionName][m_ParameterName] = val;
        m_ParameterName.clear();
    }

    //  Assignment and copying are prohibited
    BOOST_LOG_DELETED_FUNCTION(settings_grammar(settings_grammar const&))
    BOOST_LOG_DELETED_FUNCTION(settings_grammar& operator= (settings_grammar const&))
};

} // namespace

//! The function parses library settings from an input stream
template< typename CharT >
basic_settings< CharT > parse_settings(std::basic_istream< CharT >& strm)
{
    typedef CharT char_type;
    typedef std::basic_string< char_type > string_type;
    typedef settings_grammar< char_type > settings_grammar_type;
    typedef basic_settings< char_type > settings_type;
    typedef log::aux::encoding_specific< typename log::aux::encoding< char_type >::type > encoding_specific;

    if (!strm.good())
        BOOST_THROW_EXCEPTION(std::invalid_argument("The input stream for parsing settings is not valid"));

    io::basic_ios_exception_saver< char_type > exceptions_guard(strm, std::ios_base::badbit);

    // Engage parsing
    settings_type settings;
    unsigned int line_number = 1;
    std::locale loc = strm.getloc();
    settings_grammar_type gram(settings, line_number, loc);

    string_type line;
    while (!strm.eof())
    {
        std::getline(strm, line);
        algorithm::trim(line, loc);

        if (!line.empty())
        {
            if (!qi::phrase_parse(line.c_str(), line.c_str() + line.size(), gram, encoding_specific::space))
            {
                BOOST_LOG_THROW_DESCR_PARAMS(parse_error, "Could not parse settings from stream.", (line_number));
            }
            line.clear();
        }

        ++line_number;
    }

    return boost::move(settings);
}


#ifdef BOOST_LOG_USE_CHAR
template BOOST_LOG_SETUP_API basic_settings< char > parse_settings< char >(std::basic_istream< char >& strm);
#endif
#ifdef BOOST_LOG_USE_WCHAR_T
template BOOST_LOG_SETUP_API basic_settings< wchar_t > parse_settings< wchar_t >(std::basic_istream< wchar_t >& strm);
#endif

BOOST_LOG_CLOSE_NAMESPACE // namespace log

} // namespace boost

#include <boost/log/detail/footer.hpp>

#endif // BOOST_LOG_WITHOUT_SETTINGS_PARSERS
