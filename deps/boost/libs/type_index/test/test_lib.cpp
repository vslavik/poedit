#define TEST_LIB_SOURCE
#include "test_lib.hpp"

namespace user_defined_namespace {
    class user_defined{};
}

namespace test_lib {

boost::typeindex::type_index get_integer() {
    return boost::typeindex::type_id<int>();
}

boost::typeindex::type_index get_user_defined_class() {
    return boost::typeindex::type_id<user_defined_namespace::user_defined>();
}

boost::typeindex::type_index get_const_integer() {
    return boost::typeindex::type_id_with_cvr<const int>();
}

boost::typeindex::type_index get_const_user_defined_class() {
    return boost::typeindex::type_id_with_cvr<const user_defined_namespace::user_defined>();
}


// Just do nothing
void accept_typeindex(const boost::typeindex::type_index&) {}

}

