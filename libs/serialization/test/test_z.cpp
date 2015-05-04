
/*
#include <boost/config.hpp>

#include <iostream>
#include <type_traits>

struct A {
    A();
};

struct NA {
    NA(int);
};

#ifndef BOOST_NO_CXX11_HDR_TYPE_TRAITS
    #pragma message("BOOST_NO_CXX11_HDR_TYPE_TRAITS NOT defined")
#else
    #pragma message("BOOST_NO_CXX11_HDR_TYPE_TRAITS defined")
#endif

int main(int argc, char * argv[]){
    static_assert(
        std::is_default_constructible<A>::value,
        "A is NOT default constructible"
    );
    static_assert(
        ! std::is_default_constructible<NA>::value,
        "NA IS default constructible"
    );

    std::cout << std::boolalpha
    << "A is default-constructible? "
    << std::is_default_constructible<A>::value << '\n'
    << "A is trivially default-constructible? "
    << std::is_trivially_default_constructible<A>::value << '\n'
    << "NA is default-constructible? "
    << std::is_default_constructible<NA>::value << '\n'
    << "NA is trivially default-constructible? "
    << std::is_trivially_default_constructible<NA>::value << '\n'
    ;
    return 0;
}
*/
#include "../../config/test/config_info.cpp"
