//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2004-2013. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/container for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#include <boost/container/detail/config_begin.hpp>
#include <boost/container/flat_map.hpp>
#include <boost/container/allocator.hpp>
#include <boost/container/node_allocator.hpp>
#include <boost/container/adaptive_pool.hpp>

#include "print_container.hpp"
#include "dummy_test_allocator.hpp"
#include "movable_int.hpp"
#include "map_test.hpp"
#include "propagate_allocator_test.hpp"
#include "emplace_test.hpp"
#include <vector>
#include <boost/container/detail/flat_tree.hpp>

using namespace boost::container;

namespace boost {
namespace container {

//Explicit instantiation to detect compilation errors

//flat_map
template class flat_map
   < test::movable_and_copyable_int
   , test::movable_and_copyable_int
   , std::less<test::movable_and_copyable_int>
   , test::dummy_test_allocator
      < std::pair<test::movable_and_copyable_int, test::movable_and_copyable_int> >
   >;

template class flat_map
   < test::movable_and_copyable_int
   , test::movable_and_copyable_int
   , std::less<test::movable_and_copyable_int>
   , test::simple_allocator
      < std::pair<test::movable_and_copyable_int, test::movable_and_copyable_int> >
   >;

template class flat_map
   < test::movable_and_copyable_int
   , test::movable_and_copyable_int
   , std::less<test::movable_and_copyable_int>
   , std::allocator
      < std::pair<test::movable_and_copyable_int, test::movable_and_copyable_int> >
   >;

template class flat_map
   < test::movable_and_copyable_int
   , test::movable_and_copyable_int
   , std::less<test::movable_and_copyable_int>
   , allocator
      < std::pair<test::movable_and_copyable_int, test::movable_and_copyable_int> >
   >;

template class flat_map
   < test::movable_and_copyable_int
   , test::movable_and_copyable_int
   , std::less<test::movable_and_copyable_int>
   , adaptive_pool
      < std::pair<test::movable_and_copyable_int, test::movable_and_copyable_int> >
   >;

template class flat_map
   < test::movable_and_copyable_int
   , test::movable_and_copyable_int
   , std::less<test::movable_and_copyable_int>
   , node_allocator
      < std::pair<test::movable_and_copyable_int, test::movable_and_copyable_int> >
   >;

//flat_multimap
template class flat_multimap
   < test::movable_and_copyable_int
   , test::movable_and_copyable_int
   , std::less<test::movable_and_copyable_int>
   , test::dummy_test_allocator
      < std::pair<test::movable_and_copyable_int, test::movable_and_copyable_int> >
   >;

template class flat_multimap
   < test::movable_and_copyable_int
   , test::movable_and_copyable_int
   , std::less<test::movable_and_copyable_int>
   , test::simple_allocator
      < std::pair<test::movable_and_copyable_int, test::movable_and_copyable_int> >
   >;

template class flat_multimap
   < test::movable_and_copyable_int
   , test::movable_and_copyable_int
   , std::less<test::movable_and_copyable_int>
   , std::allocator
      < std::pair<test::movable_and_copyable_int, test::movable_and_copyable_int> >
   >;

template class flat_multimap
   < test::movable_and_copyable_int
   , test::movable_and_copyable_int
   , std::less<test::movable_and_copyable_int>
   , allocator
      < std::pair<test::movable_and_copyable_int, test::movable_and_copyable_int> >
   >;

template class flat_multimap
   < test::movable_and_copyable_int
   , test::movable_and_copyable_int
   , std::less<test::movable_and_copyable_int>
   , adaptive_pool
      < std::pair<test::movable_and_copyable_int, test::movable_and_copyable_int> >
   >;

template class flat_multimap
   < test::movable_and_copyable_int
   , test::movable_and_copyable_int
   , std::less<test::movable_and_copyable_int>
   , node_allocator
      < std::pair<test::movable_and_copyable_int, test::movable_and_copyable_int> >
   >;

//As flat container iterators are typedefs for vector::[const_]iterator,
//no need to explicit instantiate them

}} //boost::container


class recursive_flat_map
{
   public:
   recursive_flat_map(const recursive_flat_map &c)
      : id_(c.id_), map_(c.map_)
   {}

   recursive_flat_map & operator =(const recursive_flat_map &c)
   {
      id_ = c.id_;
      map_= c.map_;
      return *this;
   }

   int id_;
   flat_map<recursive_flat_map, recursive_flat_map> map_;

   friend bool operator< (const recursive_flat_map &a, const recursive_flat_map &b)
   {  return a.id_ < b.id_;   }
};


class recursive_flat_multimap
{
public:
   recursive_flat_multimap(const recursive_flat_multimap &c)
      : id_(c.id_), map_(c.map_)
   {}

   recursive_flat_multimap & operator =(const recursive_flat_multimap &c)
   {
      id_ = c.id_;
      map_= c.map_;
      return *this;
   }
   int id_;
   flat_map<recursive_flat_multimap, recursive_flat_multimap> map_;
   friend bool operator< (const recursive_flat_multimap &a, const recursive_flat_multimap &b)
   {  return a.id_ < b.id_;   }
};

template<class C>
void test_move()
{
   //Now test move semantics
   C original;
   C move_ctor(boost::move(original));
   C move_assign;
   move_assign = boost::move(move_ctor);
   move_assign.swap(original);
}

template<class T, class A>
class flat_map_propagate_test_wrapper
   : public boost::container::flat_map
      < T, T, std::less<T>
      , typename boost::container::allocator_traits<A>::template
         portable_rebind_alloc< std::pair<T, T> >::type>
{
   BOOST_COPYABLE_AND_MOVABLE(flat_map_propagate_test_wrapper)
   typedef boost::container::flat_map
      < T, T, std::less<T>
      , typename boost::container::allocator_traits<A>::template
         portable_rebind_alloc< std::pair<T, T> >::type> Base;
   public:
   flat_map_propagate_test_wrapper()
      : Base()
   {}

   flat_map_propagate_test_wrapper(const flat_map_propagate_test_wrapper &x)
      : Base(x)
   {}

   flat_map_propagate_test_wrapper(BOOST_RV_REF(flat_map_propagate_test_wrapper) x)
      : Base(boost::move(static_cast<Base&>(x)))
   {}

   flat_map_propagate_test_wrapper &operator=(BOOST_COPY_ASSIGN_REF(flat_map_propagate_test_wrapper) x)
   {  this->Base::operator=(x);  return *this; }

   flat_map_propagate_test_wrapper &operator=(BOOST_RV_REF(flat_map_propagate_test_wrapper) x)
   {  this->Base::operator=(boost::move(static_cast<Base&>(x)));  return *this; }

   void swap(flat_map_propagate_test_wrapper &x)
   {  this->Base::swap(x);  }
};

namespace boost{
namespace container {
namespace test{

bool flat_tree_ordered_insertion_test()
{
   using namespace boost::container;
   const std::size_t NumElements = 100;

   //Ordered insertion multimap
   {
      std::multimap<int, int> int_mmap;
      for(std::size_t i = 0; i != NumElements; ++i){
         int_mmap.insert(std::multimap<int, int>::value_type(static_cast<int>(i), static_cast<int>(i)));
      }
      //Construction insertion
      flat_multimap<int, int> fmmap(ordered_range, int_mmap.begin(), int_mmap.end());
      if(!CheckEqualContainers(&int_mmap, &fmmap))
         return false;
      //Insertion when empty
      fmmap.clear();
      fmmap.insert(ordered_range, int_mmap.begin(), int_mmap.end());
      if(!CheckEqualContainers(&int_mmap, &fmmap))
         return false;
      //Re-insertion
      fmmap.insert(ordered_range, int_mmap.begin(), int_mmap.end());
      std::multimap<int, int> int_mmap2(int_mmap);
      int_mmap2.insert(int_mmap.begin(), int_mmap.end());
      if(!CheckEqualContainers(&int_mmap2, &fmmap))
         return false;
      //Re-re-insertion
      fmmap.insert(ordered_range, int_mmap2.begin(), int_mmap2.end());
      std::multimap<int, int> int_mmap4(int_mmap2);
      int_mmap4.insert(int_mmap2.begin(), int_mmap2.end());
      if(!CheckEqualContainers(&int_mmap4, &fmmap))
         return false;
      //Re-re-insertion of even
      std::multimap<int, int> int_even_mmap;
      for(std::size_t i = 0; i < NumElements; i+=2){
         int_mmap.insert(std::multimap<int, int>::value_type(static_cast<int>(i), static_cast<int>(i)));
      }
      fmmap.insert(ordered_range, int_even_mmap.begin(), int_even_mmap.end());
      int_mmap4.insert(int_even_mmap.begin(), int_even_mmap.end());
      if(!CheckEqualContainers(&int_mmap4, &fmmap))
         return false;
   }

   //Ordered insertion map
   {
      std::map<int, int> int_map;
      for(std::size_t i = 0; i != NumElements; ++i){
         int_map.insert(std::map<int, int>::value_type(static_cast<int>(i), static_cast<int>(i)));
      }
      //Construction insertion
      flat_map<int, int> fmap(ordered_unique_range, int_map.begin(), int_map.end());
      if(!CheckEqualContainers(&int_map, &fmap))
         return false;
      //Insertion when empty
      fmap.clear();
      fmap.insert(ordered_unique_range, int_map.begin(), int_map.end());
      if(!CheckEqualContainers(&int_map, &fmap))
         return false;
      //Re-insertion
      fmap.insert(ordered_unique_range, int_map.begin(), int_map.end());
      std::map<int, int> int_map2(int_map);
      int_map2.insert(int_map.begin(), int_map.end());
      if(!CheckEqualContainers(&int_map2, &fmap))
         return false;
      //Re-re-insertion
      fmap.insert(ordered_unique_range, int_map2.begin(), int_map2.end());
      std::map<int, int> int_map4(int_map2);
      int_map4.insert(int_map2.begin(), int_map2.end());
      if(!CheckEqualContainers(&int_map4, &fmap))
         return false;
      //Re-re-insertion of even
      std::map<int, int> int_even_map;
      for(std::size_t i = 0; i < NumElements; i+=2){
         int_map.insert(std::map<int, int>::value_type(static_cast<int>(i), static_cast<int>(i)));
      }
      fmap.insert(ordered_unique_range, int_even_map.begin(), int_even_map.end());
      int_map4.insert(int_even_map.begin(), int_even_map.end());
      if(!CheckEqualContainers(&int_map4, &fmap))
         return false;
   }

   return true;
}

}}}


template<class VoidAllocator>
struct GetAllocatorMap
{
   template<class ValueType>
   struct apply
   {
      typedef flat_map< ValueType
                 , ValueType
                 , std::less<ValueType>
                 , typename allocator_traits<VoidAllocator>
                    ::template portable_rebind_alloc< std::pair<ValueType, ValueType> >::type
                 > map_type;

      typedef flat_multimap< ValueType
                 , ValueType
                 , std::less<ValueType>
                 , typename allocator_traits<VoidAllocator>
                    ::template portable_rebind_alloc< std::pair<ValueType, ValueType> >::type
                 > multimap_type;
   };
};

template<class VoidAllocator>
int test_map_variants()
{
   typedef typename GetAllocatorMap<VoidAllocator>::template apply<int>::map_type MyMap;
   typedef typename GetAllocatorMap<VoidAllocator>::template apply<test::movable_int>::map_type MyMoveMap;
   typedef typename GetAllocatorMap<VoidAllocator>::template apply<test::movable_and_copyable_int>::map_type MyCopyMoveMap;
   typedef typename GetAllocatorMap<VoidAllocator>::template apply<test::copyable_int>::map_type MyCopyMap;

   typedef typename GetAllocatorMap<VoidAllocator>::template apply<int>::multimap_type MyMultiMap;
   typedef typename GetAllocatorMap<VoidAllocator>::template apply<test::movable_int>::multimap_type MyMoveMultiMap;
   typedef typename GetAllocatorMap<VoidAllocator>::template apply<test::movable_and_copyable_int>::multimap_type MyCopyMoveMultiMap;
   typedef typename GetAllocatorMap<VoidAllocator>::template apply<test::copyable_int>::multimap_type MyCopyMultiMap;

   typedef std::map<int, int>                                     MyStdMap;
   typedef std::multimap<int, int>                                MyStdMultiMap;

   if (0 != test::map_test<
                  MyMap
                  ,MyStdMap
                  ,MyMultiMap
                  ,MyStdMultiMap>()){
      std::cout << "Error in map_test<MyBoostMap>" << std::endl;
      return 1;
   }

   if (0 != test::map_test<
                  MyMoveMap
                  ,MyStdMap
                  ,MyMoveMultiMap
                  ,MyStdMultiMap>()){
      std::cout << "Error in map_test<MyBoostMap>" << std::endl;
      return 1;
   }

   if (0 != test::map_test<
                  MyCopyMoveMap
                  ,MyStdMap
                  ,MyCopyMoveMultiMap
                  ,MyStdMultiMap>()){
      std::cout << "Error in map_test<MyBoostMap>" << std::endl;
      return 1;
   }

   if (0 != test::map_test<
                  MyCopyMap
                  ,MyStdMap
                  ,MyCopyMultiMap
                  ,MyStdMultiMap>()){
      std::cout << "Error in map_test<MyBoostMap>" << std::endl;
      return 1;
   }

   return 0;
}


int main()
{
   using namespace boost::container::test;

   //Allocator argument container
   {
      flat_map<int, int> map_((std::allocator<std::pair<int, int> >()));
      flat_multimap<int, int> multimap_((std::allocator<std::pair<int, int> >()));
   }
   //Now test move semantics
   {
      test_move<flat_map<recursive_flat_map, recursive_flat_map> >();
      test_move<flat_multimap<recursive_flat_multimap, recursive_flat_multimap> >();
   }

   ////////////////////////////////////
   //    Ordered insertion test
   ////////////////////////////////////
   if(!flat_tree_ordered_insertion_test()){
      return 1;
   }

   ////////////////////////////////////
   //    Testing allocator implementations
   ////////////////////////////////////
   //       std::allocator
   if(test_map_variants< std::allocator<void> >()){
      std::cerr << "test_map_variants< std::allocator<void> > failed" << std::endl;
      return 1;
   }
   //       boost::container::allocator
   if(test_map_variants< allocator<void> >()){
      std::cerr << "test_map_variants< allocator<void> > failed" << std::endl;
      return 1;
   }
   //       boost::container::node_allocator
   if(test_map_variants< node_allocator<void> >()){
      std::cerr << "test_map_variants< node_allocator<void> > failed" << std::endl;
      return 1;
   }
   //       boost::container::adaptive_pool
   if(test_map_variants< adaptive_pool<void> >()){
      std::cerr << "test_map_variants< adaptive_pool<void> > failed" << std::endl;
      return 1;
   }

   ////////////////////////////////////
   //    Emplace testing
   ////////////////////////////////////
   const test::EmplaceOptions MapOptions = (test::EmplaceOptions)(test::EMPLACE_HINT_PAIR | test::EMPLACE_ASSOC_PAIR);

   if(!boost::container::test::test_emplace<flat_map<test::EmplaceInt, test::EmplaceInt>, MapOptions>())
      return 1;
   if(!boost::container::test::test_emplace<flat_multimap<test::EmplaceInt, test::EmplaceInt>, MapOptions>())
      return 1;

   ////////////////////////////////////
   //    Allocator propagation testing
   ////////////////////////////////////
   if(!boost::container::test::test_propagate_allocator<flat_map_propagate_test_wrapper>())
      return 1;

   return 0;
}

#include <boost/container/detail/config_end.hpp>

