/*
 * Copyright (c) 2012-2014 Glen Joseph Fernandes 
 * glenfe at live dot com
 *
 * Distributed under the Boost Software License, 
 * Version 1.0. (See accompanying file LICENSE_1_0.txt 
 * or copy at http://boost.org/LICENSE_1_0.txt)
 */
#include <boost/config.hpp>
#if !defined(BOOST_NO_CXX11_SMART_PTR)
#include <boost/detail/lightweight_test.hpp>
#include <boost/smart_ptr/make_unique_object.hpp>

class type {
public:
    static unsigned int instances;

    explicit type() {
        instances++;
    }

    ~type() {
        instances--;
    }

private:
    type(const type&);
    type& operator=(const type&);
};

unsigned int type::instances = 0;

int main() {
    {
        std::unique_ptr<int> a1 = boost::make_unique_noinit<int>();
        BOOST_TEST(a1.get() != 0);
    }

    BOOST_TEST(type::instances == 0);
    {
        std::unique_ptr<type> a1 = boost::make_unique_noinit<type>();
        BOOST_TEST(a1.get() != 0);
        BOOST_TEST(type::instances == 1);
        a1.reset();
        BOOST_TEST(type::instances == 0);
    }

    BOOST_TEST(type::instances == 0);
    {
        std::unique_ptr<const type> a1 = boost::make_unique_noinit<const type>();
        BOOST_TEST(a1.get() != 0);
        BOOST_TEST(type::instances == 1);
        a1.reset();
        BOOST_TEST(type::instances == 0);
    }

    return boost::report_errors();
}
#else

int main() {
    return 0;
}

#endif
