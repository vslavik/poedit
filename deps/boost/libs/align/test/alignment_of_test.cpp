/*
 (c) 2014 Glen Joseph Fernandes
 glenjofe at gmail dot com

 Distributed under the Boost Software
 License, Version 1.0.
 http://boost.org/LICENSE_1_0.txt
*/
#include <boost/config.hpp>
#include <boost/align/alignment_of.hpp>
#include <boost/core/lightweight_test.hpp>
#include <cstddef>

template<class T>
struct remove_reference {
    typedef T type;
};

template<class T>
struct remove_reference<T&> {
    typedef T type;
};

#if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
template<class T>
struct remove_reference<T&&> {
    typedef T type;
};
#endif

template<class T>
struct remove_all_extents {
    typedef T type;
};

template<class T>
struct remove_all_extents<T[]> {
    typedef typename remove_all_extents<T>::type type;
};

template<class T, std::size_t N>
struct remove_all_extents<T[N]> {
    typedef typename remove_all_extents<T>::type type;
};

template<class T>
struct remove_const {
    typedef T type;
};

template<class T>
struct remove_const<const T> {
    typedef T type;
};

template<class T>
struct remove_volatile {
    typedef T type;
};

template<class T>
struct remove_volatile<volatile T> {
    typedef T type;
};

template<class T>
struct remove_cv {
    typedef typename remove_volatile<typename
        remove_const<T>::type>::type type;
};

template<class T>
struct padding {
    char offset;
    typename remove_cv<typename remove_all_extents<typename
        remove_reference<T>::type>::type>::type object;
};

template<class T>
std::size_t offset()
{
    static padding<T> p = padding<T>();
    return (char*)&p.object - &p.offset;
}

template<class T>
void test_type()
{
    std::size_t result = boost::alignment::
        alignment_of<T>::value;
    BOOST_TEST_EQ(result, offset<T>());
}

template<class T>
void test_reference()
{
    test_type<T>();
    test_type<T&>();

#if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
    test_type<T&&>();
#endif
}

template<class T>
void test_array()
{
    test_reference<T>();
    test_reference<T[2]>();
    test_type<T[]>();
}

template<class T>
void test_cv()
{
    test_array<T>();
    test_array<const T>();
    test_array<volatile T>();
    test_array<const volatile T>();
}

template<class T>
struct W1 {
    T t;
};

template<class T>
class W2 {
public:
    T t;
};

template<class T>
union W3 {
    T t;
};

template<class T>
void test()
{
    test_cv<T>();
    test_cv<W1<T> >();
    test_cv<W2<T> >();
    test_cv<W3<T> >();
}

void test_integral()
{
    test<bool>();
    test<char>();
    test<signed char>();
    test<unsigned char>();
    test<wchar_t>();

#if !defined(BOOST_NO_CXX11_CHAR16_T)
    test<char16_t>();
#endif

#if !defined(BOOST_NO_CXX11_CHAR32_T)
    test<char32_t>();
#endif

    test<short>();
    test<unsigned short>();
    test<int>();
    test<unsigned int>();
    test<long>();
    test<unsigned long>();

#if !defined(BOOST_NO_LONG_LONG)
    test<long long>();
    test<unsigned long long>();
#endif
}

void test_floating_point()
{
    test<float>();
    test<double>();
    test<long double>();
}

void test_nullptr_t()
{
#if !defined(BOOST_NO_CXX11_NULLPTR) && \
    !defined(BOOST_NO_CXX11_DECLTYPE)
    test<decltype(nullptr)>();
#endif
}

class X;

void test_pointer()
{
    test<void*>();
    test<char*>();
    test<int*>();
    test<X*>();
    test<void(*)()>();
}

void test_member_pointer()
{
    test<int X::*>();
    test<int (X::*)()>();
}

enum E {
    v = 1
};

void test_enum()
{
    test<E>();
}

struct S { };
class  C { };
union  U { };

void test_class()
{
    test<S>();
    test<C>();
    test<U>();
}

int main()
{
    test_integral();
    test_floating_point();
    test_nullptr_t();
    test_pointer();
    test_member_pointer();
    test_enum();
    test_class();

    return boost::report_errors();
}
