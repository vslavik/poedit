/////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga  2007-2013
//
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/intrusive for documentation.
//
/////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTRUSIVE_TEST_CONTAINER_HPP
#define BOOST_INTRUSIVE_TEST_CONTAINER_HPP

#include <boost/detail/lightweight_test.hpp>
#include <boost/intrusive/detail/mpl.hpp>
#include <boost/intrusive/detail/simple_disposers.hpp>
#include <boost/intrusive/detail/iterator.hpp>
#include <boost/move/utility_core.hpp>

namespace boost {
namespace intrusive {
namespace test {

template<class T>
struct is_unordered
{
   static const bool value = false;
};

template<class Container>
struct test_container_typedefs
{
   typedef typename Container::value_type       value_type;
   typedef typename Container::iterator         iterator;
   typedef typename Container::const_iterator   const_iterator;
   typedef typename Container::reference        reference;
   typedef typename Container::const_reference  const_reference;
   typedef typename Container::pointer          pointer;
   typedef typename Container::const_pointer    const_pointer;
   typedef typename Container::difference_type  difference_type;
   typedef typename Container::size_type        size_type;
   typedef typename Container::value_traits     value_traits;
};

template< class Container >
void test_container( Container & c )
{
   typedef typename Container::const_iterator   const_iterator;
   typedef typename Container::iterator         iterator;
   typedef typename Container::size_type        size_type;

   {test_container_typedefs<Container> dummy;  (void)dummy;}
   const size_type num_elem = c.size();
   BOOST_TEST( c.empty() == (num_elem == 0) );
   {
      iterator it(c.begin()), itend(c.end());
      size_type i;
      for(i = 0; i < num_elem; ++i){
         ++it;
      }
      BOOST_TEST( it == itend );
      BOOST_TEST( c.size() == i );
   }

   //Check iterator conversion
   BOOST_TEST(const_iterator(c.begin()) == c.cbegin() );
   {
      const_iterator it(c.cbegin()), itend(c.cend());
      size_type i;
      for(i = 0; i < num_elem; ++i){
         ++it;
      }
      BOOST_TEST( it == itend );
      BOOST_TEST( c.size() == i );
   }
   static_cast<const Container&>(c).check();
}


template< class Container, class Data >
void test_sequence_container(Container & c, Data & d)
{
   assert( d.size() > 2 );
   {
      c.clear();

      BOOST_TEST( c.size() == 0 );
      BOOST_TEST( c.empty() );


      {
      typename Data::iterator i = d.begin();
      c.insert( c.begin(), *i );
      BOOST_TEST( &*c.iterator_to(*c.begin())  == &*i );
      BOOST_TEST( &*c.iterator_to(*c.cbegin()) == &*i );
      BOOST_TEST( &*Container::s_iterator_to(*c.begin())  == &*i );
      BOOST_TEST( &*Container::s_iterator_to(*c.cbegin()) == &*i );
      c.insert( c.end(), *(++i) );
      }
      BOOST_TEST( c.size() == 2 );
      BOOST_TEST( !c.empty() );

      typename Container::iterator i;
      i = c.erase( c.begin() );

      BOOST_TEST( c.size() == 1 );
      BOOST_TEST( !c.empty() );

      i = c.erase( c.begin() );

      BOOST_TEST( c.size() == 0 );
      BOOST_TEST( c.empty() );

      c.insert( c.begin(), *d.begin() );

      BOOST_TEST( c.size() == 1 );
      BOOST_TEST( !c.empty() );

      {
      typename Data::iterator di = d.begin();
      ++++di;
      c.insert( c.begin(), *(di) );
      }

      i = c.erase( c.begin(), c.end() );
      BOOST_TEST( i == c.end() );

      BOOST_TEST( c.empty() );

      c.insert( c.begin(), *d.begin() );

      BOOST_TEST( c.size() == 1 );

      BOOST_TEST( c.begin() != c.end() );

      i = c.erase_and_dispose( c.begin(), detail::null_disposer() );
      BOOST_TEST( i == c.begin() );

      c.assign(d.begin(), d.end());

      BOOST_TEST( c.size() == d.size() );

      c.clear();

      BOOST_TEST( c.size() == 0 );
      BOOST_TEST( c.empty() );
   }
   {
      c.clear();
      c.insert( c.begin(), d.begin(), d.end() );
      Container move_c(::boost::move(c));
      BOOST_TEST( move_c.size() == d.size() );
      BOOST_TEST( c.empty());

      c = ::boost::move(move_c);
      BOOST_TEST( c.size() == d.size() );
      BOOST_TEST( move_c.empty());
   }
}

template< class Container, class Data >
void test_common_unordered_and_associative_container(Container & c, Data & d, boost::intrusive::detail::true_ unordered)
{
   (void)unordered;
   typedef typename Container::size_type  size_type;

   assert( d.size() > 2 );

   c.clear();
   c.insert(d.begin(), d.end());

   for( typename Data::const_iterator di = d.begin(), de = d.end();
      di != de; ++di )
   {
      BOOST_TEST( c.find(*di) != c.end() );
   }

   typename Data::const_iterator db = d.begin();
   typename Data::const_iterator da = db++;

   size_type old_size = c.size();

   c.erase(*da, c.hash_function(), c.key_eq());
   BOOST_TEST( c.size() == old_size-1 );
   //This should not eras anyone
   size_type second_erase = c.erase_and_dispose
      ( *da, c.hash_function(), c.key_eq(), detail::null_disposer() );
   BOOST_TEST( second_erase == 0 );

   BOOST_TEST( c.count(*da, c.hash_function(), c.key_eq()) == 0 );
   BOOST_TEST( c.count(*db, c.hash_function(), c.key_eq()) != 0 );

   BOOST_TEST( c.find(*da, c.hash_function(), c.key_eq()) == c.end() );
   BOOST_TEST( c.find(*db, c.hash_function(), c.key_eq()) != c.end() );

   BOOST_TEST( c.equal_range(*db, c.hash_function(), c.key_eq()).first != c.end() );

   c.clear();

   BOOST_TEST( c.equal_range(*da, c.hash_function(), c.key_eq()).first == c.end() );

   //
   //suggested_upper_bucket_count
   //
   //Maximum fallbacks to the highest possible value
   typename Container::size_type sz = Container::suggested_upper_bucket_count(size_type(-1));
   BOOST_TEST( sz > size_type(-1)/2 );
   //In the rest of cases the upper bound is returned
   sz = Container::suggested_upper_bucket_count(size_type(-1)/2);
   BOOST_TEST( sz >= size_type(-1)/2 );
   sz = Container::suggested_upper_bucket_count(size_type(-1)/4);
   BOOST_TEST( sz >= size_type(-1)/4 );
   sz = Container::suggested_upper_bucket_count(0);
   BOOST_TEST( sz > 0 );
   //
   //suggested_lower_bucket_count
   //
   sz = Container::suggested_lower_bucket_count(size_type(-1));
   BOOST_TEST( sz <= size_type(-1) );
   //In the rest of cases the lower bound is returned
   sz = Container::suggested_lower_bucket_count(size_type(-1)/2);
   BOOST_TEST( sz <= size_type(-1)/2 );
   sz = Container::suggested_lower_bucket_count(size_type(-1)/4);
   BOOST_TEST( sz <= size_type(-1)/4 );
   //Minimum fallbacks to the lowest possible value
   sz = Container::suggested_upper_bucket_count(0);
   BOOST_TEST( sz > 0 );
}

template< class Container, class Data >
void test_common_unordered_and_associative_container(Container & c, Data & d, boost::intrusive::detail::false_ unordered)
{
   (void)unordered;
   typedef typename Container::size_type  size_type;

   assert( d.size() > 2 );

   c.clear();
   typename Container::reference r = *d.begin();
   c.insert(d.begin(), ++d.begin());
   BOOST_TEST( &*Container::s_iterator_to(*c.begin())  == &r );
   BOOST_TEST( &*Container::s_iterator_to(*c.cbegin()) == &r );

   c.clear();
   c.insert(d.begin(), d.end());

   for( typename Data::const_iterator di = d.begin(), de = d.end();
      di != de; ++di )
   {
      BOOST_TEST( c.find(*di, c.key_comp()) != c.end() );
   }

   typename Data::const_iterator db = d.begin();
   typename Data::const_iterator da = db++;

   size_type old_size = c.size();

   c.erase(*da, c.key_comp());
   BOOST_TEST( c.size() == old_size-1 );
   //This should not erase any
   size_type second_erase = c.erase_and_dispose( *da, c.key_comp(), detail::null_disposer() );
   BOOST_TEST( second_erase == 0 );

   BOOST_TEST( c.count(*da, c.key_comp()) == 0 );
   BOOST_TEST( c.count(*db, c.key_comp()) != 0 );
   BOOST_TEST( c.find(*da, c.key_comp()) == c.end() );
   BOOST_TEST( c.find(*db, c.key_comp()) != c.end() );
   BOOST_TEST( c.equal_range(*db, c.key_comp()).first != c.end() );
   c.clear();
   BOOST_TEST( c.equal_range(*da, c.key_comp()).first == c.end() );
}

template< class Container, class Data >
void test_common_unordered_and_associative_container(Container & c, Data & d)
{
   typedef typename Container::size_type  size_type;
   {
      assert( d.size() > 2 );

      c.clear();
      typename Container::reference r = *d.begin();
      c.insert(d.begin(), ++d.begin());
      BOOST_TEST( &*c.iterator_to(*c.begin())  == &r );
      BOOST_TEST( &*c.iterator_to(*c.cbegin()) == &r );

      c.clear();
      c.insert(d.begin(), d.end());

      for( typename Data::const_iterator di = d.begin(), de = d.end();
         di != de; ++di )
      {
         BOOST_TEST( c.find(*di) != c.end() );
      }

      typename Data::const_iterator db = d.begin();
      typename Data::const_iterator da = db++;

      size_type old_size = c.size();

      c.erase(*da);
      BOOST_TEST( c.size() == old_size-1 );
      //This should erase nothing
      size_type second_erase = c.erase_and_dispose( *da, detail::null_disposer() );
      BOOST_TEST( second_erase == 0 );

      BOOST_TEST( c.count(*da) == 0 );
      BOOST_TEST( c.count(*db) != 0 );

      BOOST_TEST( c.find(*da) == c.end() );
      BOOST_TEST( c.find(*db) != c.end() );

      BOOST_TEST( c.equal_range(*db).first != c.end() );
      BOOST_TEST( c.equal_range(*da).first == c.equal_range(*da).second );
   }
   {
      c.clear();
      c.insert( d.begin(), d.end() );
      size_type orig_size = c.size();
      Container move_c(::boost::move(c));
      BOOST_TEST(orig_size == move_c.size());
      BOOST_TEST( c.empty());
      for( typename Data::const_iterator di = d.begin(), de = d.end();
         di != de; ++di )
      {  BOOST_TEST( move_c.find(*di) != move_c.end() );   }

      c = ::boost::move(move_c);
      for( typename Data::const_iterator di = d.begin(), de = d.end();
         di != de; ++di )
      {  BOOST_TEST( c.find(*di) != c.end() );   }
      BOOST_TEST( move_c.empty());
   }
   typedef detail::bool_<is_unordered<Container>::value> enabler;
   test_common_unordered_and_associative_container(c, d, enabler());
}

template< class Container, class Data >
void test_associative_container_invariants(Container & c, Data & d)
{
   typedef typename Container::const_iterator const_iterator;
   for( typename Data::const_iterator di = d.begin(), de = d.end();
      di != de; ++di)
   {
      const_iterator ci = c.find(*di);
      BOOST_TEST( ci != c.end() );
      BOOST_TEST( ! c.value_comp()(*ci, *di) );
      const_iterator cil = c.lower_bound(*di);
      const_iterator ciu = c.upper_bound(*di);
      std::pair<const_iterator, const_iterator> er = c.equal_range(*di);
      BOOST_TEST( cil == er.first );
      BOOST_TEST( ciu == er.second );
      if(ciu != c.end()){
         BOOST_TEST( c.value_comp()(*cil, *ciu) );
      }
      if(c.count(*di) > 1){
         const_iterator ci_next = cil; ++ci_next;
         for( ; ci_next != ciu; ++cil, ++ci_next){
            BOOST_TEST( !c.value_comp()(*ci_next, *cil) );
         }
      }
   }
}

template< class Container, class Data >
void test_associative_container(Container & c, Data & d)
{
   assert( d.size() > 2 );

   c.clear();
   c.insert(d.begin(),d.end());

   test_associative_container_invariants(c, d);

   const Container & cr = c;

   test_associative_container_invariants(cr, d);
}

template< class Container, class Data >
void test_unordered_associative_container_invariants(Container & c, Data & d)
{
   typedef typename Container::size_type size_type;
   typedef typename Container::const_iterator const_iterator;

   for( typename Data::const_iterator di = d.begin(), de = d.end() ;
      di != de ; ++di ){
      const_iterator i = c.find(*di);
      size_type nb = c.bucket(*i);
      size_type bucket_elem = boost::intrusive::iterator_distance(c.begin(nb), c.end(nb));
      BOOST_TEST( bucket_elem ==  c.bucket_size(nb) );
      BOOST_TEST( &*c.local_iterator_to(*c.find(*di)) == &*i );
      BOOST_TEST( &*c.local_iterator_to(*const_cast<const Container &>(c).find(*di)) == &*i );
      BOOST_TEST( &*Container::s_local_iterator_to(*c.find(*di)) == &*i );
      BOOST_TEST( &*Container::s_local_iterator_to(*const_cast<const Container &>(c).find(*di)) == &*i );
      std::pair<const_iterator, const_iterator> er = c.equal_range(*di);
      size_type cnt = boost::intrusive::iterator_distance(er.first, er.second);
      BOOST_TEST( cnt == c.count(*di));
      if(cnt > 1){
         const_iterator n = er.first;
         i = n++;
         const_iterator e = er.second;
         for(; n != e; ++i, ++n){
            BOOST_TEST( c.key_eq()(*i, *n) );
            BOOST_TEST( c.hash_function()(*i) == c.hash_function()(*n) );
         }
      }
   }

   size_type blen = c.bucket_count();
   size_type total_objects = 0;
   for(size_type i = 0; i < blen; ++i){
      total_objects += c.bucket_size(i);
   }
   BOOST_TEST( total_objects ==  c.size() );
}

template< class Container, class Data >
void test_unordered_associative_container(Container & c, Data & d)
{
   c.clear();
   c.insert( d.begin(), d.end() );

   test_unordered_associative_container_invariants(c, d);

   const Container & cr = c;

   test_unordered_associative_container_invariants(cr, d);
}

template< class Container, class Data >
void test_unique_container(Container & c, Data & d)
{
   //typedef typename Container::value_type value_type;
   c.clear();
   c.insert(d.begin(),d.end());
   typename Container::size_type old_size = c.size();
   //value_type v(*d.begin());
   //c.insert(v);
   Data d2(1);
   (&d2.front())->value_ = (&d.front())->value_;
   c.insert(d2.front());
   BOOST_TEST( c.size() == old_size );
   c.clear();
}

template< class Container, class Data >
void test_non_unique_container(Container & c, Data & d)
{
   //typedef typename Container::value_type value_type;
   c.clear();
   c.insert(d.begin(),d.end());
   typename Container::size_type old_size = c.size();
   //value_type v(*d.begin());
   //c.insert(v);
   Data d2(1);
   (&d2.front())->value_ = (&d.front())->value_;
   c.insert(d2.front());
   BOOST_TEST( c.size() == (old_size+1) );
   c.clear();
}

}}}

#endif   //#ifndef BOOST_INTRUSIVE_TEST_CONTAINER_HPP
