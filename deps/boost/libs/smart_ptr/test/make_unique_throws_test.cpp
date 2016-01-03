/*
 * Copyright (c) 2014 Glen Joseph Fernandes 
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
        if (instances == 0) {
            throw true;
        }
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
    BOOST_TEST(type::instances == 0);
    try {
        boost::make_unique<type>();
        BOOST_ERROR("make_unique did not throw");
    } catch (...) {
        BOOST_TEST(type::instances == 0);
    }

    return boost::report_errors();
}
#else

int main() {
    return 0;
}

#endif
