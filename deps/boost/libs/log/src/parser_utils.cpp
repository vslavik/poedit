/*
 *          Copyright Andrey Semashev 2007 - 2013.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   parser_utils.cpp
 * \author Andrey Semashev
 * \date   31.03.2008
 *
 * \brief  This header is the Boost.Log library implementation, see the library documentation
 *         at http://www.boost.org/libs/log/doc/log.html.
 */

#ifndef BOOST_LOG_WITHOUT_SETTINGS_PARSERS

#include <iterator>
#include <algorithm>
#include "parser_utils.hpp"
#include <boost/log/detail/header.hpp>

namespace boost {

BOOST_LOG_OPEN_NAMESPACE

namespace aux {

#ifdef BOOST_LOG_USE_CHAR

#ifndef BOOST_LOG_BROKEN_STATIC_CONSTANTS_LINKAGE

const char_constants< char >::char_type char_constants< char >::char_comment;
const char_constants< char >::char_type char_constants< char >::char_comma;
const char_constants< char >::char_type char_constants< char >::char_quote;
const char_constants< char >::char_type char_constants< char >::char_percent;
const char_constants< char >::char_type char_constants< char >::char_exclamation;
const char_constants< char >::char_type char_constants< char >::char_and;
const char_constants< char >::char_type char_constants< char >::char_or;
const char_constants< char >::char_type char_constants< char >::char_equal;
const char_constants< char >::char_type char_constants< char >::char_greater;
const char_constants< char >::char_type char_constants< char >::char_less;
const char_constants< char >::char_type char_constants< char >::char_underline;
const char_constants< char >::char_type char_constants< char >::char_backslash;
const char_constants< char >::char_type char_constants< char >::char_section_bracket_left;
const char_constants< char >::char_type char_constants< char >::char_section_bracket_right;
const char_constants< char >::char_type char_constants< char >::char_paren_bracket_left;
const char_constants< char >::char_type char_constants< char >::char_paren_bracket_right;

#endif // BOOST_LOG_BROKEN_STATIC_CONSTANTS_LINKAGE

void char_constants< char >::translate_escape_sequences(std::basic_string< char_type >& str)
{
    using namespace std; // to make sure we can use C functions unqualified

    std::basic_string< char_type >::iterator it = str.begin();
    while (it != str.end())
    {
        it = std::find(it, str.end(), '\\');
        if (std::distance(it, str.end()) >= 2)
        {
            it = str.erase(it);
            switch (*it)
            {
            case 'n':
                *it = '\n'; break;
            case 'r':
                *it = '\r'; break;
            case 'a':
                *it = '\a'; break;
            case '\\':
                ++it; break;
            case 't':
                *it = '\t'; break;
            case 'b':
                *it = '\b'; break;
            case 'x':
                {
                    std::basic_string< char_type >::iterator b = it;
                    if (std::distance(++b, str.end()) >= 2)
                    {
                        char_type c1 = *b++, c2 = *b++;
                        if (isxdigit(c1) && isxdigit(c2))
                        {
                            *it++ = char_type((to_number(c1) << 4) | to_number(c2));
                            it = str.erase(it, b);
                        }
                    }
                    break;
                }
            default:
                {
                    if (*it >= '0' && *it <= '7')
                    {
                        std::basic_string< char_type >::iterator b = it;
                        int c = (*b++) - '0';
                        if (*b >= '0' && *b <= '7')
                            c = c * 8 + (*b++) - '0';
                        if (*b >= '0' && *b <= '7')
                            c = c * 8 + (*b++) - '0';

                        *it++ = char_type(c);
                        it = str.erase(it, b);
                    }
                    break;
                }
            }
        }
    }
}

#endif // BOOST_LOG_USE_CHAR

#ifdef BOOST_LOG_USE_WCHAR_T

#ifndef BOOST_LOG_BROKEN_STATIC_CONSTANTS_LINKAGE

const char_constants< wchar_t >::char_type char_constants< wchar_t >::char_comment;
const char_constants< wchar_t >::char_type char_constants< wchar_t >::char_comma;
const char_constants< wchar_t >::char_type char_constants< wchar_t >::char_quote;
const char_constants< wchar_t >::char_type char_constants< wchar_t >::char_percent;
const char_constants< wchar_t >::char_type char_constants< wchar_t >::char_exclamation;
const char_constants< wchar_t >::char_type char_constants< wchar_t >::char_and;
const char_constants< wchar_t >::char_type char_constants< wchar_t >::char_or;
const char_constants< wchar_t >::char_type char_constants< wchar_t >::char_equal;
const char_constants< wchar_t >::char_type char_constants< wchar_t >::char_greater;
const char_constants< wchar_t >::char_type char_constants< wchar_t >::char_less;
const char_constants< wchar_t >::char_type char_constants< wchar_t >::char_underline;
const char_constants< wchar_t >::char_type char_constants< wchar_t >::char_backslash;
const char_constants< wchar_t >::char_type char_constants< wchar_t >::char_section_bracket_left;
const char_constants< wchar_t >::char_type char_constants< wchar_t >::char_section_bracket_right;
const char_constants< wchar_t >::char_type char_constants< wchar_t >::char_paren_bracket_left;
const char_constants< wchar_t >::char_type char_constants< wchar_t >::char_paren_bracket_right;

#endif // BOOST_LOG_BROKEN_STATIC_CONSTANTS_LINKAGE

void char_constants< wchar_t >::translate_escape_sequences(std::basic_string< char_type >& str)
{
    std::basic_string< char_type >::iterator it = str.begin();
    while (it != str.end())
    {
        it = std::find(it, str.end(), L'\\');
        if (std::distance(it, str.end()) >= 2)
        {
            it = str.erase(it);
            switch (*it)
            {
            case L'n':
                *it = L'\n'; break;
            case L'r':
                *it = L'\r'; break;
            case L'a':
                *it = L'\a'; break;
            case L'\\':
                ++it; break;
            case L't':
                *it = L'\t'; break;
            case L'b':
                *it = L'\b'; break;
            case L'x':
                {
                    std::basic_string< char_type >::iterator b = it;
                    if (std::distance(++b, str.end()) >= 2)
                    {
                        char_type c1 = *b++, c2 = *b++;
                        if (iswxdigit(c1) && iswxdigit(c2))
                        {
                            *it++ = char_type((to_number(c1) << 4) | to_number(c2));
                            it = str.erase(it, b);
                        }
                    }
                    break;
                }
            case L'u':
                {
                    std::basic_string< char_type >::iterator b = it;
                    if (std::distance(++b, str.end()) >= 4)
                    {
                        char_type c1 = *b++, c2 = *b++, c3 = *b++, c4 = *b++;
                        if (iswxdigit(c1) && iswxdigit(c2) && iswxdigit(c3) && iswxdigit(c4))
                        {
                            *it++ = char_type(
                                (to_number(c1) << 12) |
                                (to_number(c2) << 8) |
                                (to_number(c3) << 4) |
                                to_number(c4));
                            it = str.erase(it, b);
                        }
                    }
                    break;
                }
            case L'U':
                {
                    std::basic_string< char_type >::iterator b = it;
                    if (std::distance(++b, str.end()) >= 8)
                    {
                        char_type c1 = *b++, c2 = *b++, c3 = *b++, c4 = *b++;
                        char_type c5 = *b++, c6 = *b++, c7 = *b++, c8 = *b++;
                        if (iswxdigit(c1) && iswxdigit(c2) && iswxdigit(c3) && iswxdigit(c4) &&
                            iswxdigit(c5) && iswxdigit(c6) && iswxdigit(c7) && iswxdigit(c8))
                        {
                            *it++ = char_type(
                                (to_number(c1) << 28) |
                                (to_number(c2) << 24) |
                                (to_number(c3) << 20) |
                                (to_number(c4) << 16) |
                                (to_number(c5) << 12) |
                                (to_number(c6) << 8) |
                                (to_number(c7) << 4) |
                                to_number(c8));
                            it = str.erase(it, b);
                        }
                    }
                    break;
                }
            default:
                {
                    if (*it >= L'0' && *it <= L'7')
                    {
                        std::basic_string< char_type >::iterator b = it;
                        int c = (*b++) - L'0';
                        if (*b >= L'0' && *b <= L'7')
                            c = c * 8 + (*b++) - L'0';
                        if (*b >= L'0' && *b <= L'7')
                            c = c * 8 + (*b++) - L'0';

                        *it++ = char_type(c);
                        it = str.erase(it, b);
                    }
                    break;
                }
            }
        }
    }
}

#endif // BOOST_LOG_USE_WCHAR_T

} // namespace aux

BOOST_LOG_CLOSE_NAMESPACE // namespace log

} // namespace boost

#include <boost/log/detail/footer.hpp>

#endif // BOOST_LOG_WITHOUT_SETTINGS_PARSERS
