/*
 *          Copyright Andrey Semashev 2007 - 2013.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   timer.cpp
 * \author Andrey Semashev
 * \date   02.12.2007
 *
 * \brief  This header is the Boost.Log library implementation, see the library documentation
 *         at http://www.boost.org/libs/log/doc/log.html.
 */

#include <boost/config.hpp>
#include <boost/log/attributes/timer.hpp>
#include <boost/log/attributes/attribute_value_impl.hpp>

#if defined(BOOST_WINDOWS) && !defined(BOOST_LOG_NO_QUERY_PERFORMANCE_COUNTER)

#include "windows_version.hpp"
#include <boost/limits.hpp>
#include <boost/assert.hpp>
#include <boost/cstdint.hpp>
#include <boost/detail/interlocked.hpp>
#include <boost/log/detail/locks.hpp>
#include <boost/log/detail/spin_mutex.hpp>
#include <windows.h>
#include <boost/log/detail/header.hpp>

namespace boost {

BOOST_LOG_OPEN_NAMESPACE

namespace attributes {

//! Factory implementation
class BOOST_LOG_VISIBLE timer::impl :
    public attribute::impl
{
private:
#if !defined(BOOST_LOG_NO_THREADS)
    //! Synchronization mutex type
    typedef log::aux::spin_mutex mutex_type;
    //! Synchronization mutex
    mutex_type m_Mutex;
#endif
    //! Frequency factor for calculating duration
    uint64_t m_FrequencyFactor;
    //! Last value of the performance counter
    uint64_t m_LastCounter;
    //! Elapsed time duration, in microseconds
    uint64_t m_Duration;

public:
    //! Constructor
    impl() : m_Duration(0)
    {
        LARGE_INTEGER li;
        QueryPerformanceFrequency(&li);
        BOOST_ASSERT(li.QuadPart != 0LL);
        m_FrequencyFactor = static_cast< uint64_t >(li.QuadPart) / 1000000ULL;

        QueryPerformanceCounter(&li);
        m_LastCounter = static_cast< uint64_t >(li.QuadPart);
    }

    //! The method returns the actual attribute value. It must not return NULL.
    attribute_value get_value()
    {
        LARGE_INTEGER li;
        QueryPerformanceCounter(&li);

        uint64_t duration;
        {
            BOOST_LOG_EXPR_IF_MT(log::aux::exclusive_lock_guard< mutex_type > _(m_Mutex);)

            const uint64_t counts = static_cast< uint64_t >(li.QuadPart) - m_LastCounter;
            m_LastCounter = static_cast< uint64_t >(li.QuadPart);
            m_Duration += counts / m_FrequencyFactor;
            duration = m_Duration;
        }

        // All these dances are needed simply to construct Boost.DateTime duration without truncating the value
        value_type res;
        if (duration < static_cast< uint64_t >((std::numeric_limits< value_type::fractional_seconds_type >::max)()))
        {
            res = value_type(0, 0, 0, static_cast< value_type::fractional_seconds_type >(
                duration * (1000000 / value_type::traits_type::ticks_per_second)));
        }
        else
        {
            uint64_t total_seconds = duration / 1000000ULL;
            value_type::fractional_seconds_type usec = static_cast< value_type::fractional_seconds_type >(duration % 1000000ULL);
            if (total_seconds < static_cast< uint64_t >((std::numeric_limits< value_type::sec_type >::max)()))
            {
                res = value_type(0, 0, static_cast< value_type::sec_type >(total_seconds), usec);
            }
            else
            {
                uint64_t total_hours = total_seconds / 3600ULL;
                value_type::sec_type seconds = static_cast< value_type::sec_type >(total_seconds % 3600ULL);
                res = value_type(static_cast< value_type::hour_type >(total_hours), 0, seconds, usec);
            }
        }

        return attribute_value(new attribute_value_impl< value_type >(res));
    }
};

//! Constructor
timer::timer() : attribute(new impl())
{
}

//! Constructor for casting support
timer::timer(cast_source const& source) : attribute(source.as< impl >())
{
}

} // namespace attributes

BOOST_LOG_CLOSE_NAMESPACE // namespace log

} // namespace boost

#include <boost/log/detail/footer.hpp>

#else // defined(BOOST_WINDOWS) && !defined(BOOST_LOG_NO_QUERY_PERFORMANCE_COUNTER)

#include <boost/log/detail/header.hpp>

namespace boost {

BOOST_LOG_OPEN_NAMESPACE

namespace attributes {

//! Factory implementation
class BOOST_LOG_VISIBLE timer::impl :
    public attribute::impl
{
public:
    //! Time type
    typedef utc_time_traits::time_type time_type;

private:
    //! Base time point
    const time_type m_BaseTimePoint;

public:
    /*!
     * Constructor. Starts time counting.
     */
    impl() : m_BaseTimePoint(utc_time_traits::get_clock()) {}

    attribute_value get_value()
    {
        return attribute_value(new attribute_value_impl< value_type >(
            utc_time_traits::get_clock() - m_BaseTimePoint));
    }
};

//! Constructor
timer::timer() : attribute(new impl())
{
}

//! Constructor for casting support
timer::timer(cast_source const& source) : attribute(source.as< impl >())
{
}

} // namespace attributes

BOOST_LOG_CLOSE_NAMESPACE // namespace log

} // namespace boost

#include <boost/log/detail/footer.hpp>

#endif // defined(BOOST_WINDOWS) && !defined(BOOST_LOG_NO_QUERY_PERFORMANCE_COUNTER)
