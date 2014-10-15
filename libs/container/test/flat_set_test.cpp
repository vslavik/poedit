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
#include <set>
#include <boost/container/flat_set.hpp>
#include <boost/container/allocator.hpp>
#include <boost/container/node_allocator.hpp>
#include <boost/container/adaptive_pool.hpp>

#include "print_container.hpp"
#include "dummy_test_allocator.hpp"
#include "movable_int.hpp"
#include "set_test.hpp"
#include "propagate_allocator_test.hpp"
#include "emplace_test.hpp"
#include <vector>
#include <boost/container/detail/flat_tree.hpp>

using namespace boost::container;

namespace boost {
namespace container {

//Explicit instantiation to detect compilation errors

//flat_set
template class flat_set
   < test::movable_and_copyable_int
   , std::less<test::movable_and_copyable_int>
   , test::dummy_test_allocator<test::movable_and_copyable_int>
   >;

template class flat_set
   < test::movable_and_copyable_int
   , std::less<test::movable_and_copyable_int>
   , test::simple_allocator<test::movable_and_copyable_int>
   >;

template class flat_set
   < test::movable_and_copyable_int
   , std::less<test::movable_and_copyable_int>
   , std::allocator<test::movable_and_copyable_int>
   >;

template class flat_set
   < test::movable_and_copyable_int
   , std::less<test::movable_and_copyable_int>
   , allocator<test::movable_and_copyable_int>
   >;

template class flat_set
   < test::movable_and_copyable_int
   , std::less<test::movable_and_copyable_int>
   , adaptive_pool<test::movable_and_copyable_int>
   >;

template class flat_set
   < test::movable_and_copyable_int
   , std::less<test::movable_and_copyable_int>
   , node_allocator<test::movable_and_copyable_int>
   >;

//flat_multiset
template class flat_multiset
   < test::movable_and_copyable_int
   , std::less<test::movable_and_copyable_int>
   , test::dummy_test_allocator<test::movable_and_copyable_int>
   >;

template class flat_multiset
   < test::movable_and_copyable_int
   , std::less<test::movable_and_copyable_int>
   , test::simple_allocator<test::movable_and_copyable_int>
   >;

template class flat_multiset
   < test::movable_and_copyable_int
   , std::less<test::movable_and_copyable_int>
   , std::allocator<test::movable_and_copyable_int>
   >;

template class flat_multiset
   < test::movable_and_copyable_int
   , std::less<test::movable_and_copyable_int>
   , allocator<test::movable_and_copyable_int>
   >;

template class flat_multiset
   < test::movable_and_copyable_int
   , std::less<test::movable_and_copyable_int>
   , adaptive_pool<test::movable_and_copyable_int>
   >;

template class flat_multiset
   < test::movable_and_copyable_int
   , std::less<test::movable_and_copyable_int>
   , node_allocator<test::movable_and_copyable_int>
   >;

namespace container_detail {

//Instantiate base class as previous instantiations don't instantiate inherited members
template class flat_tree
   < test::movable_and_copyable_int
   , test::movable_and_copyable_int
   , identity<test::movable_and_copyable_int>
   , std::less<test::movable_and_copyable_int>
   , test::dummy_test_allocator<test::movable_and_copyable_int>
   >;

template class flat_tree
   < test::movable_and_copyable_int
   , test::movable_and_copyable_int
   , identity<test::movable_and_copyable_int>
   , std::less<test::movable_and_copyable_int>
   , test::simple_allocator<test::movable_and_copyable_int>
   >;

template class flat_tree
   < test::movable_and_copyable_int
   , test::movable_and_copyable_int
   , identity<test::movable_and_copyable_int>
   , std::less<test::movable_and_copyable_int>
   , std::allocator<test::movable_and_copyable_int>
   >;

template class flat_tree
   < test::movable_and_copyable_int
   , test::movable_and_copyable_int
   , identity<test::movable_and_copyable_int>
   , std::less<test::movable_and_copyable_int>
   , allocator<test::movable_and_copyable_int>
   >;

template class flat_tree
   < test::movable_and_copyable_int
   , test::movable_and_copyable_int
   , identity<test::movable_and_copyable_int>
   , std::less<test::movable_and_copyable_int>
   , adaptive_pool<test::movable_and_copyable_int>
   >;

template class flat_tree
   < test::movable_and_copyable_int
   , test::movable_and_copyable_int
   , identity<test::movable_and_copyable_int>
   , std::less<test::movable_and_copyable_int>
   , node_allocator<test::movable_and_copyable_int>
   >;

}  //container_detail {

//As flat container iterators are typedefs for vector::[const_]iterator,
//no need to explicit instantiate them

}} //boost::container

//Test recursive structures
class recursive_flat_set
{
   public:
   recursive_flat_set(const recursive_flat_set &c)
      : id_(c.id_), flat_set_(c.flat_set_)
   {}

   recursive_flat_set & operator =(const recursive_flat_set &c)
   {
      id_ = c.id_;
      flat_set_= c.flat_set_;
      return *this;
   }
   int id_;
   flat_set<recursive_flat_set> flat_set_;
   friend bool operator< (const recursive_flat_set &a, const recursive_flat_set &b)
   {  return a.id_ < b.id_;   }
};


//Test recursive structures
class recursive_flat_multiset
{
   public:
   recursive_flat_multiset(const recursive_flat_multiset &c)
      : id_(c.id_), flat_multiset_(c.flat_multiset_)
   {}

   recursive_flat_multiset & operator =(const recursive_flat_multiset &c)
   {
      id_ = c.id_;
      flat_multiset_= c.flat_multiset_;
      return *this;
   }
   int id_;
   flat_multiset<recursive_flat_multiset> flat_multiset_;
   friend bool operator< (const recursive_flat_multiset &a, const recursive_flat_multiset &b)
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
class flat_set_propagate_test_wrapper
   : public boost::container::flat_set<T, std::less<T>, A>
{
   BOOST_COPYABLE_AND_MOVABLE(flat_set_propagate_test_wrapper)
   typedef boost::container::flat_set<T, std::less<T>, A> Base;
   public:
   flat_set_propagate_test_wrapper()
      : Base()
   {}

   flat_set_propagate_test_wrapper(const flat_set_propagate_test_wrapper &x)
      : Base(x)
   {}

   flat_set_propagate_test_wrapper(BOOST_RV_REF(flat_set_propagate_test_wrapper) x)
      : Base(boost::move(static_cast<Base&>(x)))
   {}

   flat_set_propagate_test_wrapper &operator=(BOOST_COPY_ASSIGN_REF(flat_set_propagate_test_wrapper) x)
   {  this->Base::operator=(x);  return *this; }

   flat_set_propagate_test_wrapper &operator=(BOOST_RV_REF(flat_set_propagate_test_wrapper) x)
   {  this->Base::operator=(boost::move(static_cast<Base&>(x)));  return *this; }

   void swap(flat_set_propagate_test_wrapper &x)
   {  this->Base::swap(x);  }
};

namespace boost{
namespace container {
namespace test{

bool flat_tree_ordered_insertion_test()
{
   using namespace boost::container;
   const std::size_t NumElements = 100;

   //Ordered insertion multiset
   {
      std::multiset<int> int_mset;
      for(std::size_t i = 0; i != NumElements; ++i){
         int_mset.insert(static_cast<int>(i));
      }
      //Construction insertion
      flat_multiset<int> fmset(ordered_range, int_mset.begin(), int_mset.end());
      if(!CheckEqualContainers(&int_mset, &fmset))
         return false;
      //Insertion when empty
      fmset.clear();
      fmset.insert(ordered_range, int_mset.begin(), int_mset.end());
      if(!CheckEqualContainers(&int_mset, &fmset))
         return false;
      //Re-insertion
      fmset.insert(ordered_range, int_mset.begin(), int_mset.end());
      std::multiset<int> int_mset2(int_mset);
      int_mset2.insert(int_mset.begin(), int_mset.end());
      if(!CheckEqualContainers(&int_mset2, &fmset))
         return false;
      //Re-re-insertion
      fmset.insert(ordered_range, int_mset2.begin(), int_mset2.end());
      std::multiset<int> int_mset4(int_mset2);
      int_mset4.insert(int_mset2.begin(), int_mset2.end());
      if(!CheckEqualContainers(&int_mset4, &fmset))
         return false;
      //Re-re-insertion of even
      std::multiset<int> int_even_mset;
      for(std::size_t i = 0; i < NumElements; i+=2){
         int_mset.insert(static_cast<int>(i));
      }
      fmset.insert(ordered_range, int_even_mset.begin(), int_even_mset.end());
      int_mset4.insert(int_even_mset.begin(), int_even_mset.end());
      if(!CheckEqualContainers(&int_mset4, &fmset))
         return false;
   }

   //Ordered insertion set
   {
      std::set<int> int_set;
      for(std::size_t i = 0; i != NumElements; ++i){
         int_set.insert(static_cast<int>(i));
      }
      //Construction insertion
      flat_set<int> fset(ordered_unique_range, int_set.begin(), int_set.end());
      if(!CheckEqualContainers(&int_set, &fset))
         return false;
      //Insertion when empty
      fset.clear();
      fset.insert(ordered_unique_range, int_set.begin(), int_set.end());
      if(!CheckEqualContainers(&int_set, &fset))
         return false;
      //Re-insertion
      fset.insert(ordered_unique_range, int_set.begin(), int_set.end());
      std::set<int> int_set2(int_set);
      int_set2.insert(int_set.begin(), int_set.end());
      if(!CheckEqualContainers(&int_set2, &fset))
         return false;
      //Re-re-insertion
      fset.insert(ordered_unique_range, int_set2.begin(), int_set2.end());
      std::set<int> int_set4(int_set2);
      int_set4.insert(int_set2.begin(), int_set2.end());
      if(!CheckEqualContainers(&int_set4, &fset))
         return false;
      //Re-re-insertion of even
      std::set<int> int_even_set;
      for(std::size_t i = 0; i < NumElements; i+=2){
         int_set.insert(static_cast<int>(i));
      }
      fset.insert(ordered_unique_range, int_even_set.begin(), int_even_set.end());
      int_set4.insert(int_even_set.begin(), int_even_set.end());
      if(!CheckEqualContainers(&int_set4, &fset))
         return false;
   }

   return true;
}

}}}


template<class VoidAllocator>
struct GetAllocatorSet
{
   template<class ValueType>
   struct apply
   {
      typedef flat_set < ValueType
                       , std::less<ValueType>
                       , typename allocator_traits<VoidAllocator>
                           ::template portable_rebind_alloc<ValueType>::type
                        > set_type;

      typedef flat_multiset < ValueType
                            , std::less<ValueType>
                            , typename allocator_traits<VoidAllocator>
                                 ::template portable_rebind_alloc<ValueType>::type
                            > multiset_type;
   };
};

template<class VoidAllocator>
int test_set_variants()
{
   typedef typename GetAllocatorSet<VoidAllocator>::template apply<int>::set_type MySet;
   typedef typename GetAllocatorSet<VoidAllocator>::template apply<test::movable_int>::set_type MyMoveSet;
   typedef typename GetAllocatorSet<VoidAllocator>::template apply<test::movable_and_copyable_int>::set_type MyCopyMoveSet;
   typedef typename GetAllocatorSet<VoidAllocator>::template apply<test::copyable_int>::set_type MyCopySet;

   typedef typename GetAllocatorSet<VoidAllocator>::template apply<int>::multiset_type MyMultiSet;
   typedef typename GetAllocatorSet<VoidAllocator>::template apply<test::movable_int>::multiset_type MyMoveMultiSet;
   typedef typename GetAllocatorSet<VoidAllocator>::template apply<test::movable_and_copyable_int>::multiset_type MyCopyMoveMultiSet;
   typedef typename GetAllocatorSet<VoidAllocator>::template apply<test::copyable_int>::multiset_type MyCopyMultiSet;

   typedef std::set<int>                                          MyStdSet;
   typedef std::multiset<int>                                     MyStdMultiSet;

   if (0 != test::set_test<
                  MySet
                  ,MyStdSet
                  ,MyMultiSet
                  ,MyStdMultiSet>()){
      std::cout << "Error in set_test<MyBoostSet>" << std::endl;
      return 1;
   }

   if (0 != test::set_test<
                  MyMoveSet
                  ,MyStdSet
                  ,MyMoveMultiSet
                  ,MyStdMultiSet>()){
      std::cout << "Error in set_test<MyBoostSet>" << std::endl;
      return 1;
   }

   if (0 != test::set_test<
                  MyCopyMoveSet
                  ,MyStdSet
                  ,MyCopyMoveMultiSet
                  ,MyStdMultiSet>()){
      std::cout << "Error in set_test<MyBoostSet>" << std::endl;
      return 1;
   }

   if (0 != test::set_test<
                  MyCopySet
                  ,MyStdSet
                  ,MyCopyMultiSet
                  ,MyStdMultiSet>()){
      std::cout << "Error in set_test<MyBoostSet>" << std::endl;
      return 1;
   }

   return 0;
}


int main()
{
   using namespace boost::container::test;

   //Allocator argument container
   {
      flat_set<int> set_((std::allocator<int>()));
      flat_multiset<int> multiset_((std::allocator<int>()));
   }
   //Now test move semantics
   {
      test_move<flat_set<recursive_flat_set> >();
      test_move<flat_multiset<recursive_flat_multiset> >();
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
   if(test_set_variants< std::allocator<void> >()){
      std::cerr << "test_set_variants< std::allocator<void> > failed" << std::endl;
      return 1;
   }
   //       boost::container::allocator
   if(test_set_variants< allocator<void> >()){
      std::cerr << "test_set_variants< allocator<void> > failed" << std::endl;
      return 1;
   }
   //       boost::container::node_allocator
   if(test_set_variants< node_allocator<void> >()){
      std::cerr << "test_set_variants< node_allocator<void> > failed" << std::endl;
      return 1;
   }
   //       boost::container::adaptive_pool
   if(test_set_variants< adaptive_pool<void> >()){
      std::cerr << "test_set_variants< adaptive_pool<void> > failed" << std::endl;
      return 1;
   }

   ////////////////////////////////////
   //    Emplace testing
   ////////////////////////////////////
   const test::EmplaceOptions SetOptions = (test::EmplaceOptions)(test::EMPLACE_HINT | test::EMPLACE_ASSOC);

   if(!boost::container::test::test_emplace<flat_set<test::EmplaceInt>, SetOptions>())
      return 1;
   if(!boost::container::test::test_emplace<flat_multiset<test::EmplaceInt>, SetOptions>())
      return 1;

   ////////////////////////////////////
   //    Allocator propagation testing
   ////////////////////////////////////
   if(!boost::container::test::test_propagate_allocator<flat_set_propagate_test_wrapper>())
      return 1;

   return 0;
}

#include <boost/container/detail/config_end.hpp>

