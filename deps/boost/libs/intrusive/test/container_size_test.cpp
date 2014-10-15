/////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga  2014-2014
//
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/intrusive for documentation.
//
/////////////////////////////////////////////////////////////////////////////
#include <boost/intrusive/detail/config_begin.hpp>
#include <boost/detail/lightweight_test.hpp>
#include <cstddef>

#include <boost/intrusive/list.hpp>
#include <boost/intrusive/slist.hpp>
#include <boost/intrusive/set.hpp>
#include <boost/intrusive/avl_set.hpp>
#include <boost/intrusive/bs_set.hpp>
#include <boost/intrusive/sg_set.hpp>
#include <boost/intrusive/splay_set.hpp>
#include <boost/intrusive/treap_set.hpp>
#include <boost/intrusive/unordered_set.hpp>
#include <boost/static_assert.hpp>
#include "itestvalue.hpp"

using namespace boost::intrusive;

template<bool Value>
struct boolean
{
   static const bool value = Value;
};

template<class A, class B>
struct pow2_and_equal_sizes
{
   static const std::size_t a_size = sizeof(A);
   static const std::size_t b_size = sizeof(B);
   static const bool a_b_sizes_equal = a_size == b_size;
   static const bool value = !(a_size & (a_size - 1u));
};

template<class Hook>
struct node : Hook
{};

//Avoid testing for uncommon architectures
void test_sizes(boolean<false>, std::size_t)
{}

template<class C>
void test_iterator_sizes(C &, std::size_t size)
{
   typedef typename C::iterator        iterator;
   typedef typename C::const_iterator  const_iterator;
   BOOST_TEST_EQ(sizeof(iterator), size);
   BOOST_TEST_EQ(sizeof(const_iterator), size);
}

//Test sizes for common 32 and 64 bit architectures
void test_sizes(boolean<true>, std::size_t wordsize)
{
   {  //list
      list<node< node<list_base_hook<> > > > c;
      BOOST_TEST_EQ(sizeof(c), wordsize*3);
      test_iterator_sizes(c, wordsize);
   }
   {
      list<node< node<list_base_hook<> > >, constant_time_size<false> > c;
      BOOST_TEST_EQ(sizeof(c), wordsize*2);
      test_iterator_sizes(c, wordsize);
   }
   {
      list< node< list_base_hook<> >, header_holder_type< pointer_holder< list_node<void*> > > > c;
      BOOST_TEST_EQ(sizeof(c), wordsize*2);
      test_iterator_sizes(c, wordsize);
   }
   {
      list< node< list_base_hook<> >, constant_time_size<false>, header_holder_type< pointer_holder< list_node<void*> > > > c;
      BOOST_TEST_EQ(sizeof(c), wordsize*1);
      test_iterator_sizes(c, wordsize);
   }
   {  //slist
      slist<node< node< slist_base_hook<> > > > c;
      BOOST_TEST_EQ(sizeof(c), wordsize*2);
      test_iterator_sizes(c, wordsize);
   }
   {
      slist<node< node< slist_base_hook<> > > , constant_time_size<false> > c;
      BOOST_TEST_EQ(sizeof(c), wordsize*1);
      test_iterator_sizes(c, wordsize);
   }
   {
      slist<node< node< slist_base_hook<> > > , cache_last<true> > c;
      BOOST_TEST_EQ(sizeof(c), wordsize*3);
      test_iterator_sizes(c, wordsize);
   }
   {  //set
      set<node< node< set_base_hook<> > > > c;
      BOOST_TEST_EQ(sizeof(c), wordsize*5);
      test_iterator_sizes(c, wordsize);
   }
   {
      set<node< node< set_base_hook<> > > , constant_time_size<false> > c;
      BOOST_TEST_EQ(sizeof(c), wordsize*4);
      test_iterator_sizes(c, wordsize);
   }
   {
      set<node< node< set_base_hook<optimize_size<true> > > > , constant_time_size<false> > c;
      BOOST_TEST_EQ(sizeof(c), wordsize*3);
      test_iterator_sizes(c, wordsize);
   }
   {
      set< node< set_base_hook<> >, header_holder_type< pointer_holder< rbtree_node<void*> > > > c;
      BOOST_TEST_EQ(sizeof(c), wordsize*2);
      test_iterator_sizes(c, wordsize);
   }
   {
      set< node< set_base_hook<> >, constant_time_size<false>, header_holder_type< pointer_holder< rbtree_node<void*> > > > c;
      BOOST_TEST_EQ(sizeof(c), wordsize*1);
      test_iterator_sizes(c, wordsize);
   }
   {  //avl
      avl_set<node< node< avl_set_base_hook<> > > > c;
      BOOST_TEST_EQ(sizeof(c), wordsize*5);
      test_iterator_sizes(c, wordsize);
   }
   {
      avl_set<node< node< avl_set_base_hook<> > > , constant_time_size<false> > c;
      BOOST_TEST_EQ(sizeof(c), wordsize*4);
      test_iterator_sizes(c, wordsize);
   }
   {
      avl_set<node< node< avl_set_base_hook<optimize_size<true> > > > , constant_time_size<false> > c;
      BOOST_TEST_EQ(sizeof(c), wordsize*3);
      test_iterator_sizes(c, wordsize);
   }
   {
      avl_set< node< avl_set_base_hook<> >, header_holder_type< pointer_holder< avltree_node<void*> > > > c;
      BOOST_TEST_EQ(sizeof(c), wordsize*2);
      test_iterator_sizes(c, wordsize);
   }
   {
      avl_set< node< avl_set_base_hook<> >, constant_time_size<false>, header_holder_type< pointer_holder< avltree_node<void*> > > > c;
      BOOST_TEST_EQ(sizeof(c), wordsize*1);
      test_iterator_sizes(c, wordsize);
   }
   {  //splay
      splay_set<node< node< bs_set_base_hook<> > > > c;
      BOOST_TEST_EQ(sizeof(c), wordsize*4);
      test_iterator_sizes(c, wordsize);
   }
   {
      splay_set<node< node< bs_set_base_hook<> > > , constant_time_size<false> > c;
      BOOST_TEST_EQ(sizeof(c), wordsize*3);
      test_iterator_sizes(c, wordsize);
   }
   {  //scapegoat
      sg_set<node< bs_set_base_hook<> > > c;
      BOOST_TEST_EQ(sizeof(c), (wordsize*5+sizeof(float)*2));
      test_iterator_sizes(c, wordsize);
   }
   {  //treap
      treap_set<node< bs_set_base_hook<> > > c;
      BOOST_TEST_EQ(sizeof(c), wordsize*4);
      test_iterator_sizes(c, wordsize);
   }
   {
      treap_set<node< bs_set_base_hook<> > , constant_time_size<false> > c;
      BOOST_TEST_EQ(sizeof(c), wordsize*3);
      test_iterator_sizes(c, wordsize);
   }
   {  //unordered
      typedef unordered_set<node< unordered_set_base_hook<> > > cont_type;
      cont_type::bucket_type buckets[1];
      cont_type c(cont_type::bucket_traits(buckets, 1));
      BOOST_TEST_EQ(sizeof(c), wordsize*3);
      test_iterator_sizes(c, wordsize*2);
   }
   {
      typedef unordered_set<node< unordered_set_base_hook<> > , power_2_buckets<true>  > cont_type;
      cont_type::bucket_type buckets[1];
      cont_type c(cont_type::bucket_traits(buckets, 1));
      BOOST_TEST_EQ(sizeof(c), wordsize*3);
      test_iterator_sizes(c, wordsize*2);
   }
   {
      typedef unordered_set<node< unordered_set_base_hook<> >, constant_time_size<false> > cont_type;
      cont_type::bucket_type buckets[1];
      cont_type c(cont_type::bucket_traits(buckets, 1));
      BOOST_TEST_EQ(sizeof(c), wordsize*2);
      test_iterator_sizes(c, wordsize*2);
   }
   {
      typedef unordered_set<node< unordered_set_base_hook< optimize_multikey<true> > >, constant_time_size<false> > cont_type;
      cont_type::bucket_type buckets[1];
      cont_type c(cont_type::bucket_traits(buckets, 1));
      BOOST_TEST_EQ(sizeof(c), wordsize*2);
      test_iterator_sizes(c, wordsize*2);
   }
   {
      typedef unordered_set<node< unordered_set_base_hook< optimize_multikey<true> > >, incremental<true> > cont_type;
      cont_type::bucket_type buckets[1];
      cont_type c(cont_type::bucket_traits(buckets, 1));
      BOOST_TEST_EQ(sizeof(c), wordsize*4);
      test_iterator_sizes(c, wordsize*2);
   }
}

int main()
{
   test_sizes(boolean< pow2_and_equal_sizes<std::size_t, void*>::value >(), sizeof(std::size_t));
   return ::boost::report_errors();
}

#include <boost/intrusive/detail/config_end.hpp>
