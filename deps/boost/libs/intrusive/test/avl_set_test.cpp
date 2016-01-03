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
#include <boost/intrusive/avl_set.hpp>
#include "itestvalue.hpp"
#include "bptr_value.hpp"
#include "smart_ptr.hpp"
#include "generic_set_test.hpp"

namespace boost { namespace intrusive { namespace test {

#if !defined (BOOST_INTRUSIVE_VARIADIC_TEMPLATES)
template<class T, class O1, class O2, class O3, class O4, class O5>
#else
template<class T, class ...Options>
#endif
struct has_insert_before<boost::intrusive::avl_set<T,
   #if !defined (BOOST_INTRUSIVE_VARIADIC_TEMPLATES)
   O1, O2, O3, O4, O5
   #else
   Options...
   #endif
> >
{
   static const bool value = true;
};

}}}


using namespace boost::intrusive;

struct my_tag;

template<class VoidPointer>
struct hooks
{
   typedef avl_set_base_hook<void_pointer<VoidPointer> >    base_hook_type;
   typedef avl_set_base_hook
      <link_mode<auto_unlink>
      , void_pointer<VoidPointer>
      , tag<my_tag>
      , optimize_size<true> >                               auto_base_hook_type;
   typedef avl_set_member_hook
      <void_pointer<VoidPointer> >                          member_hook_type;
   typedef avl_set_member_hook
      < link_mode<auto_unlink>
      , void_pointer<VoidPointer> >                         auto_member_hook_type;
   typedef nonhook_node_member< avltree_node_traits<VoidPointer, false >,
                                avltree_algorithms
                              > nonhook_node_member_type;
};

// container generator with void node allocator
template < bool Default_Holder >
struct GetContainer_With_Holder
{
template< class ValueType
        , class Option1 =void
        , class Option2 =void
        , class Option3 =void
        >
struct GetContainer
{
   typedef boost::intrusive::avl_set
      < ValueType
      , Option1
      , Option2
      , Option3
      > type;
};
};

// container generator with standard (non-void) node allocator
template <>
struct GetContainer_With_Holder< false >
{
template< class ValueType
        , class Option1 =void
        , class Option2 =void
        , class Option3 =void
        >
struct GetContainer
{
   // extract node type through options->value_traits->node_traits->node
   typedef typename pack_options< avltree_defaults, Option1, Option2, Option3 >::type packed_options;
   typedef typename detail::get_value_traits< ValueType, typename packed_options::proto_value_traits>::type value_traits;
   typedef typename value_traits::node_traits::node node;

   typedef boost::intrusive::avl_set
      < ValueType
      , Option1
      , Option2
      , Option3
      , header_holder_type< pointer_holder< node > >
      > type;
};
};

template<class VoidPointer, bool constant_time_size, bool Default_Holder>
class test_main_template
{
   public:
   int operator()()
   {
      using namespace boost::intrusive;
      typedef testvalue<hooks<VoidPointer> , constant_time_size> value_type;

      test::test_generic_set < typename detail::get_base_value_traits
                  < value_type
                  , typename hooks<VoidPointer>::base_hook_type
                  >::type
                , GetContainer_With_Holder< Default_Holder >::template GetContainer
                >::test_all();
      test::test_generic_set < typename detail::get_member_value_traits
                  < member_hook< value_type
                               , typename hooks<VoidPointer>::member_hook_type
                               , &value_type::node_
                               >
                  >::type
                , GetContainer_With_Holder< Default_Holder >::template GetContainer
                >::test_all();
      test::test_generic_set < nonhook_node_member_value_traits< value_type,
                                                                 typename hooks<VoidPointer>::nonhook_node_member_type,
                                                                 &value_type::nhn_member_,
                                                                 safe_link
                                                               >,
                               GetContainer_With_Holder< Default_Holder >::template GetContainer
                             >::test_all();
      return 0;
   }
};

template<class VoidPointer, bool Default_Holder>
class test_main_template<VoidPointer, false, Default_Holder>
{
   public:
   int operator()()
   {
      using namespace boost::intrusive;
      typedef testvalue<hooks<VoidPointer> , false> value_type;

      test::test_generic_set < typename detail::get_base_value_traits
                  < value_type
                  , typename hooks<VoidPointer>::base_hook_type
                  >::type
                , GetContainer_With_Holder< Default_Holder >::template GetContainer
                >::test_all();

      test::test_generic_set < typename detail::get_member_value_traits
                  < member_hook< value_type
                               , typename hooks<VoidPointer>::member_hook_type
                               , &value_type::node_
                               >
                  >::type
                , GetContainer_With_Holder< Default_Holder >::template GetContainer
                >::test_all();

      test::test_generic_set < typename detail::get_base_value_traits
                  < value_type
                  , typename hooks<VoidPointer>::auto_base_hook_type
                  >::type
                , GetContainer_With_Holder< Default_Holder >::template GetContainer
                >::test_all();

      test::test_generic_set < typename detail::get_member_value_traits
                  < member_hook< value_type
                               , typename hooks<VoidPointer>::auto_member_hook_type
                               , &value_type::auto_node_
                               >
                  >::type
                , GetContainer_With_Holder< Default_Holder >::template GetContainer
                >::test_all();

      return 0;
   }
};

// container generator which ignores further parametrization, except for compare option
template < typename Value_Traits, bool ConstantTimeSize, typename HeaderHolder >
struct Get_Preset_Container
{
    template < class
             , class Option1 = void
             , class Option2 = void
             , class Option3 = void
             >
    struct GetContainer
    {
        // ignore further paramatrization except for the compare option
        // notably ignore the size option (use preset)
        typedef typename pack_options< avltree_defaults, Option1, Option2, Option3 >::type packed_options;
        typedef typename packed_options::compare compare_option;

        typedef boost::intrusive::avl_set< typename Value_Traits::value_type,
                                           value_traits< Value_Traits >,
                                           constant_time_size< ConstantTimeSize >,
                                           compare< compare_option >,
                                           header_holder_type< HeaderHolder >
                                         > type;
    };
};

template < bool ConstantTimeSize >
struct test_main_template_bptr
{
    void operator () ()
    {
        typedef BPtr_Value value_type;
        typedef BPtr_Value_Traits< AVLTree_BPtr_Node_Traits > value_traits;
        typedef bounded_allocator< value_type > allocator_type;

        allocator_type::init();
        test::test_generic_set< value_traits,
                                Get_Preset_Container< value_traits, ConstantTimeSize,
                                                      bounded_pointer_holder< value_type > >::template GetContainer
                              >::test_all();
        assert(allocator_type::is_clear());
        allocator_type::destroy();
    }
};

int main()
{
   // test (plain/smart pointers) x (nonconst/const size) x (void node allocator)
   test_main_template<void*, false, true>()();
   test_main_template<boost::intrusive::smart_ptr<void>, false, true>()();
   test_main_template<void*, true, true>()();
   test_main_template<boost::intrusive::smart_ptr<void>, true, true>()();
   // test (plain pointers) x (nonconst/const size) x (standard node allocator)
   test_main_template<void*, false, false>()();
   test_main_template<void*, true, false>()();
   // test (bounded pointers) x (nonconst/const size) x (special node allocator)
   //AVL with bptr failing in some platforms, to investigate.
   //test_main_template_bptr< true >()();
   //test_main_template_bptr< false >()();

   return boost::report_errors();
}
