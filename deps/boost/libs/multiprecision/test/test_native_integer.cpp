///////////////////////////////////////////////////////////////
//  Copyright 2012 John Maddock. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_

//
// Compare arithmetic results using fixed_int to GMP results.
//

#ifdef _MSC_VER
#  define _SCL_SECURE_NO_WARNINGS
#endif

#include <boost/multiprecision/integer.hpp>
#include "test.hpp"

#ifdef BOOST_MSVC
#pragma warning(disable:4146)
#endif

template <class I, class H>
void test()
{
   using namespace boost::multiprecision;

   I i(0);

   BOOST_CHECK_THROW(lsb(i), std::range_error);
   BOOST_CHECK(bit_test(bit_set(i, 0), 0));
   BOOST_CHECK_EQUAL(bit_set(i, 0), 1);
   BOOST_CHECK_EQUAL(bit_unset(i, 0), 0);
   BOOST_CHECK_EQUAL(bit_flip(bit_set(i, 0), 0), 0);

   unsigned max_index = (std::numeric_limits<I>::digits) - 1;
   BOOST_CHECK(bit_test(bit_set(i, max_index), max_index));
   BOOST_CHECK_EQUAL(bit_unset(i, max_index), 0);
   BOOST_CHECK_EQUAL(bit_flip(bit_set(i, max_index), max_index), 0);

   if(std::numeric_limits<I>::is_signed)
   {
      i = static_cast<I>(-1);
      BOOST_CHECK_THROW(lsb(i), std::range_error);
   }

   H mx = (std::numeric_limits<H>::max)();

   BOOST_CHECK_EQUAL(multiply(i, mx, mx), static_cast<I>(mx) * static_cast<I>(mx));
   BOOST_CHECK_EQUAL(add(i, mx, mx), static_cast<I>(mx) + static_cast<I>(mx));
   if(std::numeric_limits<I>::is_signed)
   {
      BOOST_CHECK_EQUAL(subtract(i, mx, static_cast<H>(-mx)), static_cast<I>(mx) - static_cast<I>(-mx));
      BOOST_CHECK_EQUAL(add(i, static_cast<H>(-mx), static_cast<H>(-mx)), static_cast<I>(-mx) + static_cast<I>(-mx));
   }

   i = (std::numeric_limits<I>::max)();
   I j = 12345;
   I r, q;
   divide_qr(i, j, q, r);
   BOOST_CHECK_EQUAL(q, i / j);
   BOOST_CHECK_EQUAL(r, i % j);
   BOOST_CHECK_EQUAL(integer_modulus(i, j), i % j);
   I p = 456;
   BOOST_CHECK_EQUAL(powm(i, p, j), pow(cpp_int(i), static_cast<unsigned>(p)) % j);
}

int main()
{
   using namespace boost::multiprecision;

   test<boost::int32_t, boost::int16_t>();
   test<boost::int64_t, boost::int32_t>();
   test<boost::uint32_t, boost::uint16_t>();
   test<boost::uint64_t, boost::uint32_t>();
   
   return boost::report_errors();
}



