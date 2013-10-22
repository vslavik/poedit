// Copyright (C) 2012 Vicente Botet
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#define BOOST_THREAD_VERSION 4

#include <boost/thread/future.hpp>
#include <iostream>

int p1() { return 5; }

void p() { }

#if defined BOOST_THREAD_USES_MOVE
boost::future<void> void_compute()
{
  return BOOST_THREAD_MAKE_RV_REF(boost::make_future());
}
#endif

boost::future<int> compute(int x)
{
  if (x == 0) return boost::make_future(0);
  if (x < 0) return boost::make_future(-1);
  //boost::future<int> f1 = boost::async([]() { return x+1; });
  boost::future<int> f1 = boost::async(boost::launch::async, p1);
  return boost::move(f1);
}
boost::shared_future<int> shared_compute(int x)
{
  if (x == 0) return boost::make_shared_future(0);
  if (x < 0) return boost::make_shared_future(-1);
  //boost::future<int> f1 = boost::async([]() { return x+1; });
  boost::shared_future<int> f1 = boost::async(p1).share();
  return boost::move(f1);
}


int main()
{
#if defined BOOST_THREAD_USES_MOVE
  {
    boost::future<void> f = void_compute();
    f.get();
  }
#endif
  {
    boost::future<int> f = compute(2);
    std::cout << f.get() << std::endl;
  }
  {
    boost::future<int> f = compute(0);
    std::cout << f.get() << std::endl;
  }
  {
    boost::shared_future<int> f = shared_compute(2);
    std::cout << f.get() << std::endl;
  }
  return 0;
}
