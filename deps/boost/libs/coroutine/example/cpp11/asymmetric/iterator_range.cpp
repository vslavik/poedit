
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <cstdlib>
#include <iostream>

#include <boost/coroutine/all.hpp>

#include <boost/range/adaptor/filtered.hpp>
#include <boost/range/iterator_range.hpp>
#include <iostream>

int main()
{
    using namespace boost;
    coroutines::asymmetric_coroutine<int>::pull_type c([](coroutines::asymmetric_coroutine<int>::push_type& yield) {
        for (int i = 0; i < 5; ++i) {
            yield(i);
        }
    });
    auto crange = make_iterator_range(begin(c), end(c));
    for (auto n : crange
            | adaptors::filtered([](int n){return n % 2 == 0;})) // the filtered adaptor needs const operator==.
        {
        std::cout << n << std::endl;
    }
}
