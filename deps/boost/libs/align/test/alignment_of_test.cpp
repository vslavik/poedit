/*
 Copyright (c) 2014 Glen Joseph Fernandes
 glenfe at live dot com

 Distributed under the Boost Software License,
 Version 1.0. (See accompanying file LICENSE_1_0.txt
 or copy at http://boost.org/LICENSE_1_0.txt)
*/
#include <boost/config.hpp>
#include <boost/align/alignment_of.hpp>
#include <boost/core/lightweight_test.hpp>
#include <cstddef>

template<class T>
struct no_ref {
    typedef T type;
};

template<class T>
struct no_ref<T&> {
    typedef T type;
};

#if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
template<class T>
struct no_ref<T&&> {
    typedef T type;
};
#endif

template<class T>
struct no_extents {
    typedef T type;
};

template<class T>
struct no_extents<T[]> {
    typedef typename no_extents<T>::type type;
};

template<class T, std::size_t N>
struct no_extents<T[N]> {
    typedef typename no_extents<T>::type type;
};

template<class T>
struct no_const {
    typedef T type;
};

template<class T>
struct no_const<const T> {
    typedef T type;
};

template<class T>
struct no_volatile {
    typedef T type;
};

template<class T>
struct no_volatile<volatile T> {
    typedef T type;
};

template<class T>
struct no_cv {
    typedef typename no_volatile<typename
        no_const<T>::type>::type type;
};

template<class T>
struct padded {
    char unit;
    typename no_cv<typename no_extents<typename
        no_ref<T>::type>::type>::type object;
};

template<class T>
std::ptrdiff_t offset()
{
    static padded<T> p = padded<T>();
    return (char*)&p.object - &p.unit;
}

template<class T>
void test_impl()
{
    std::size_t result = boost::alignment::
        alignment_of<T>::value;
    BOOST_TEST_EQ(result, offset<T>());
}

template<class T>
void test_reference()
{
    test_impl<T>();
    test_impl<T&>();

#if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
    test_impl<T&&>();
#endif
}

template<class T>
void test_array()
{
    test_reference<T>();
    test_reference<T[2]>();
    test_impl<T[]>();
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
