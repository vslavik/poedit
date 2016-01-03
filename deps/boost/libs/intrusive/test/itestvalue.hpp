/////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Olaf Krzikalla 2004-2006.
// (C) Copyright Ion Gaztanaga  2006-2013.
//
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/intrusive for documentation.
//
/////////////////////////////////////////////////////////////////////////////
#ifndef BOOST_INTRUSIVE_DETAIL_ITESTVALUE_HPP
#define BOOST_INTRUSIVE_DETAIL_ITESTVALUE_HPP

#include <iostream>
#include <boost/intrusive/options.hpp>
#include <boost/functional/hash.hpp>
#include "nonhook_node.hpp"
#include <boost/container/vector.hpp>

namespace boost{
namespace intrusive{

struct testvalue_filler
{
   void *dummy_[3];
};

template<class Hooks, bool ConstantTimeSize>
struct testvalue
   :  testvalue_filler
   ,  Hooks::base_hook_type
   ,  Hooks::auto_base_hook_type
{
   typename Hooks::member_hook_type        node_;
   typename Hooks::auto_member_hook_type   auto_node_;
   typename Hooks::nonhook_node_member_type nhn_member_;
   int value_;

   static const bool constant_time_size = ConstantTimeSize;

   testvalue()
   {}

   testvalue(int i)
      :  value_(i)
   {}

   testvalue (const testvalue& src)
      :  value_ (src.value_)
   {}

   // testvalue is used in boost::container::vector and thus prev and next
   // have to be handled appropriately when copied:
   testvalue & operator= (const testvalue& src)
   {
      Hooks::base_hook_type::operator=(static_cast<const typename Hooks::base_hook_type&>(src));
      Hooks::auto_base_hook_type::operator=(static_cast<const typename Hooks::auto_base_hook_type&>(src));
      this->node_       = src.node_;
      this->auto_node_  = src.auto_node_;
      this->nhn_member_ = src.nhn_member_;
      value_ = src.value_;
      return *this;
   }

   void swap_nodes(testvalue &other)
   {
      Hooks::base_hook_type::swap_nodes(static_cast<typename Hooks::base_hook_type&>(other));
      Hooks::auto_base_hook_type::swap_nodes(static_cast<typename Hooks::auto_base_hook_type&>(other));
      node_.swap_nodes(other.node_);
      auto_node_.swap_nodes(other.auto_node_);
      nhn_member_.swap_nodes(other.nhn_member_);
   }

   bool is_linked() const
   {
      return Hooks::base_hook_type::is_linked() ||
      Hooks::auto_base_hook_type::is_linked() ||
      node_.is_linked() ||
      auto_node_.is_linked() ||
      nhn_member_.is_linked();
   }

   ~testvalue()
   {}

   bool operator< (const testvalue &other) const
   {  return value_ < other.value_;  }

   bool operator==(const testvalue &other) const
   {  return value_ == other.value_;  }

   bool operator!=(const testvalue &other) const
   {  return value_ != other.value_;  }

   friend bool operator< (int other1, const testvalue &other2)
   {  return other1 < other2.value_;  }

   friend bool operator< (const testvalue &other1, int other2)
   {  return other1.value_ < other2;  }

   friend bool operator== (int other1, const testvalue &other2)
   {  return other1 == other2.value_;  }

   friend bool operator== (const testvalue &other1, int other2)
   {  return other1.value_ == other2;  }

   friend bool operator!= (int other1, const testvalue &other2)
   {  return other1 != other2.value_;  }

   friend bool operator!= (const testvalue &other1, int other2)
   {  return other1.value_ != other2;  }
};

template < typename Node_Algorithms, class Hooks, bool ConstantTimeSize >
void swap_nodes(testvalue< Hooks, ConstantTimeSize >& lhs, testvalue< Hooks, ConstantTimeSize >& rhs)
{
    lhs.swap_nodes(rhs);
}

template < typename Value_Type >
std::size_t hash_value(const Value_Type& t)
{
   boost::hash<int> hasher;
   return hasher((&t)->value_);
}

template < typename Value_Type >
bool priority_order(const Value_Type& t1, const Value_Type& t2)
{
   std::size_t hash1 = hash_value(t1);
   boost::hash_combine(hash1, &t1);
   std::size_t hash2 = hash_value(t2);
   boost::hash_combine(hash2, &t2);
   return hash1 < hash2;
}

template<class Hooks, bool constant_time_size>
std::ostream& operator<<
   (std::ostream& s, const testvalue<Hooks, constant_time_size>& t)
{  return s << t.value_;   }

struct even_odd
{
   template < typename value_type_1, typename value_type_2 >
   bool operator()
      (const value_type_1& v1
      ,const value_type_2& v2) const
   {
      if (((&v1)->value_ & 1) == ((&v2)->value_ & 1))
         return (&v1)->value_ < (&v2)->value_;
      else
         return (&v2)->value_ & 1;
   }
};

struct is_even
{
   template <typename value_type>
   bool operator()
      (const value_type& v1) const
   {  return ((&v1)->value_ & 1) == 0;  }
};

struct is_odd
{
   template <typename value_type>
   bool operator()
      (const value_type& v1) const
   {  return ((&v1)->value_ & 1) != 0;  }
};

template <typename>
struct Value_Container;

template < class Hooks, bool ConstantTimeSize >
struct Value_Container< testvalue< Hooks, ConstantTimeSize > >
{
    typedef boost::container::vector< testvalue< Hooks, ConstantTimeSize > > type;
};

template < typename T >
class pointer_holder
{
   public:
   pointer_holder() : _ptr(new T())
   {}

   ~pointer_holder()
   { delete _ptr;   }

   const T* get_node() const { return _ptr; }
   T* get_node() { return _ptr; }

   private:
   T* const _ptr;
};

}  //namespace boost{
}  //namespace intrusive{

#endif
