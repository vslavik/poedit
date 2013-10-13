/*
 *          Copyright Andrey Semashev 2007 - 2013.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   util_explicit_operator_bool.cpp
 * \author Andrey Semashev
 * \date   17.07.2010
 *
 * \brief  This test checks that explicit operator bool can be used in
 *         the valid contexts.
 */

#define BOOST_TEST_MODULE util_explicit_operator_bool

#include <boost/log/utility/explicit_operator_bool.hpp>

namespace {

    // A test object that has the operator of explicit conversion to bool
    struct checkable
    {
        BOOST_LOG_EXPLICIT_OPERATOR_BOOL()
        bool operator! () const
        {
            return false;
        }
    };

} // namespace

int main(int, char*[])
{
    checkable val;
    if (val)
    {
        return 1;
    }

    return 0;
}
