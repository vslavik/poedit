# /* **************************************************************************
#  *                                                                          *
#  *     (C) Copyright Paul Mensonides 2002.
#  *     Distributed under the Boost Software License, Version 1.0. (See
#  *     accompanying file LICENSE_1_0.txt or copy at
#  *     http://www.boost.org/LICENSE_1_0.txt)
#  *                                                                          *
#  ************************************************************************** */
#
# /* Revised by Edward Diener (2011) */
#
# /* See http://www.boost.org for most recent version. */
#
# include <boost/preprocessor/arithmetic/add.hpp>
# include <boost/preprocessor/arithmetic/sub.hpp>
# include <boost/preprocessor/cat.hpp>
# include <boost/preprocessor/comparison/equal.hpp>
# include <boost/preprocessor/comparison/less.hpp>
# include <boost/preprocessor/control/iif.hpp>
# include <boost/preprocessor/facilities/is_empty.hpp>
# include <boost/preprocessor/seq.hpp>
# include <boost/preprocessor/array/elem.hpp>
# include <boost/preprocessor/array/size.hpp>
# include <boost/preprocessor/tuple/elem.hpp>
# include <boost/preprocessor/tuple/size.hpp>
# include <boost/preprocessor/list/at.hpp>
# include <boost/preprocessor/list/size.hpp>
# include <boost/preprocessor/variadic/elem.hpp>
# include <boost/preprocessor/variadic/size.hpp>
# include <libs/preprocessor/test/test.h>

# define SEQ_NONE ()
# define SEQ (4)(1)(5)(2)
# define SEQVAR (4,5,8,3,61)(1,0)(5,22,43)(2)(17,45,33)

# define REVERSAL(s, x, y) BOOST_PP_SUB(y, x)
# define SUB_S(s, x, y) BOOST_PP_SUB(x, y)
# define ADD_S(s, x, y) BOOST_PP_ADD(x, y)
# define CAT_S(s, x, y) BOOST_PP_CAT(x, BOOST_PP_IS_EMPTY(y))

BEGIN BOOST_PP_IS_EMPTY(BOOST_PP_SEQ_HEAD(SEQ_NONE)) == 1 END
BEGIN BOOST_PP_SEQ_HEAD(SEQ) == 4 END

BEGIN BOOST_PP_SEQ_FOLD_LEFT(CAT_S, 1, SEQ_NONE) == 11 END
BEGIN BOOST_PP_SEQ_FOLD_LEFT(SUB_S, 22, SEQ) == 10 END
BEGIN BOOST_PP_SEQ_FOLD_RIGHT(CAT_S, 2, SEQ_NONE) == 21 END
BEGIN BOOST_PP_SEQ_FOLD_RIGHT(ADD_S, 0, SEQ) == 12 END
BEGIN BOOST_PP_SEQ_FOLD_RIGHT(REVERSAL, 0, SEQ) == 4 END

BEGIN BOOST_PP_IS_EMPTY(BOOST_PP_SEQ_CAT(BOOST_PP_SEQ_REVERSE(SEQ_NONE))) == 1 END
BEGIN BOOST_PP_SEQ_CAT(BOOST_PP_SEQ_REVERSE(SEQ)) == 2514 END

BEGIN BOOST_PP_SEQ_CAT(BOOST_PP_SEQ_REST_N(2, SEQ)) == 52 END
BEGIN BOOST_PP_IS_EMPTY(BOOST_PP_SEQ_CAT(BOOST_PP_SEQ_FIRST_N(1, SEQ_NONE))) == 1 END
BEGIN BOOST_PP_SEQ_CAT(BOOST_PP_SEQ_FIRST_N(2, SEQ)) == 41 END

BEGIN BOOST_PP_IS_EMPTY(BOOST_PP_SEQ_ELEM(0, SEQ_NONE)) == 1 END
BEGIN BOOST_PP_SEQ_SIZE(SEQ_NONE) == 1 END
BEGIN BOOST_PP_SEQ_ELEM(2, SEQ) == 5 END
BEGIN BOOST_PP_SEQ_SIZE(SEQ) == 4 END

BEGIN BOOST_PP_SEQ_CAT(BOOST_PP_SEQ_TRANSFORM(CAT_S, 13, SEQ_NONE)) == 131 END
BEGIN BOOST_PP_SEQ_CAT(BOOST_PP_SEQ_TRANSFORM(ADD_S, 2, SEQ)) == 6374 END
BEGIN BOOST_PP_SEQ_CAT(BOOST_PP_SEQ_TAIL(SEQ) SEQ) == 1524152 END

# define F1(r, state, x) + x + state
# define FI2(r, state, i, x) BOOST_PP_IIF(BOOST_PP_EQUAL(i,2),+ x + x + state,+ x + state)
BEGIN BOOST_PP_SEQ_FOR_EACH(F1, 1, SEQ) == 16 END
BEGIN BOOST_PP_SEQ_FOR_EACH_I(FI2, 1, SEQ) == 21 END

BEGIN BOOST_PP_TUPLE_ELEM(4, 3, BOOST_PP_SEQ_TO_TUPLE(SEQ)) == 2 END
BEGIN BOOST_PP_IS_EMPTY(BOOST_PP_TUPLE_ELEM(1, 0, BOOST_PP_SEQ_TO_TUPLE(SEQ_NONE))) == 1 END

#if BOOST_PP_VARIADICS

BEGIN BOOST_PP_TUPLE_SIZE(BOOST_PP_SEQ_TO_TUPLE(SEQ_NONE)) == 1 END

#endif

BEGIN BOOST_PP_ARRAY_ELEM(3, BOOST_PP_SEQ_TO_ARRAY(SEQ)) == 2 END

BEGIN BOOST_PP_IS_EMPTY(BOOST_PP_ARRAY_ELEM(0, BOOST_PP_SEQ_TO_ARRAY(SEQ_NONE))) == 1 END
BEGIN BOOST_PP_ARRAY_SIZE(BOOST_PP_SEQ_TO_ARRAY(SEQ_NONE)) == 1 END

# define LESS_S(s, x, y) BOOST_PP_LESS(x, y)
BEGIN BOOST_PP_SEQ_CAT(BOOST_PP_SEQ_FILTER(LESS_S, 3, SEQ)) == 45 END

BEGIN BOOST_PP_SEQ_CAT(BOOST_PP_SEQ_INSERT(SEQ_NONE, 0, 7)) == 7 END
BEGIN BOOST_PP_SEQ_CAT(BOOST_PP_SEQ_INSERT(SEQ, 0, 3)) == 34152 END
BEGIN BOOST_PP_SEQ_CAT(BOOST_PP_SEQ_INSERT(SEQ, 2, 3)) == 41352 END
BEGIN BOOST_PP_SEQ_CAT(BOOST_PP_SEQ_INSERT(SEQ, 4, 3)) == 41523 END

BEGIN BOOST_PP_SEQ_CAT(BOOST_PP_SEQ_POP_BACK(SEQ)) == 415 END
BEGIN BOOST_PP_SEQ_CAT(BOOST_PP_SEQ_POP_FRONT(SEQ)) == 152 END

BEGIN BOOST_PP_SEQ_CAT(BOOST_PP_SEQ_PUSH_FRONT(SEQ_NONE, 145))  == 145 END
BEGIN BOOST_PP_SEQ_CAT(BOOST_PP_SEQ_PUSH_FRONT(SEQ, 3))  == 34152 END
BEGIN BOOST_PP_SEQ_CAT(BOOST_PP_SEQ_PUSH_BACK(SEQ_NONE, 79))  == 79 END
BEGIN BOOST_PP_SEQ_CAT(BOOST_PP_SEQ_PUSH_BACK(SEQ, 3))  == 41523 END

BEGIN BOOST_PP_SEQ_CAT(BOOST_PP_SEQ_REMOVE(SEQ, 0))  == 152 END
BEGIN BOOST_PP_SEQ_CAT(BOOST_PP_SEQ_REMOVE(SEQ, 2))  == 412 END
BEGIN BOOST_PP_SEQ_CAT(BOOST_PP_SEQ_REMOVE(SEQ, 3))  == 415 END

BEGIN BOOST_PP_SEQ_CAT(BOOST_PP_SEQ_REPLACE(SEQ_NONE, 0, 22))  == 22 END
BEGIN BOOST_PP_SEQ_CAT(BOOST_PP_SEQ_REPLACE(SEQ, 0, 3))  == 3152 END
BEGIN BOOST_PP_SEQ_CAT(BOOST_PP_SEQ_REPLACE(SEQ, 1, 3))  == 4352 END
BEGIN BOOST_PP_SEQ_CAT(BOOST_PP_SEQ_REPLACE(SEQ, 3, 3))  == 4153 END

BEGIN BOOST_PP_SEQ_CAT(BOOST_PP_SEQ_SUBSEQ(SEQ, 0, 4))  == 4152 END
BEGIN BOOST_PP_SEQ_CAT(BOOST_PP_SEQ_SUBSEQ(SEQ, 0, 2))  == 41 END
BEGIN BOOST_PP_SEQ_CAT(BOOST_PP_SEQ_SUBSEQ(SEQ, 1, 2))  == 15 END
BEGIN BOOST_PP_SEQ_CAT(BOOST_PP_SEQ_SUBSEQ(SEQ, 2, 2))  == 52 END

# define F2(r, x) + BOOST_PP_SEQ_ELEM(0, x) + 2 - BOOST_PP_SEQ_ELEM(1, x)

#define ADD_NIL(x) x(nil)

BEGIN BOOST_PP_SEQ_FOR_EACH_PRODUCT(F2, ((1)(0)) ((2)(3))) == 0 END

# define L1 (0)(x)
# define L2 (a)(1)(b)(2)
# define L3 (c)(3)(d)

# define LL (L1)(L2)(L3)

#define SEQ_APPEND(s, state, elem) state elem
BEGIN BOOST_PP_SEQ_CAT(BOOST_PP_SEQ_TAIL(BOOST_PP_SEQ_FOLD_LEFT(SEQ_APPEND, (~), LL))) == 0x0a1b2c3d END
BEGIN BOOST_PP_SEQ_SIZE(BOOST_PP_SEQ_TAIL(BOOST_PP_SEQ_FOLD_LEFT(SEQ_APPEND, (~), LL))) == 9 END

BEGIN BOOST_PP_LIST_AT(BOOST_PP_SEQ_TO_LIST(SEQ), 2) == 5 END
BEGIN BOOST_PP_IS_EMPTY(BOOST_PP_LIST_AT(BOOST_PP_SEQ_TO_LIST(SEQ_NONE),0)) == 1 END
BEGIN BOOST_PP_LIST_SIZE(BOOST_PP_SEQ_TO_LIST(SEQ_NONE)) == 1 END

#if BOOST_PP_VARIADICS

BEGIN BOOST_PP_VARIADIC_SIZE(BOOST_PP_SEQ_ENUM(SEQ_NONE)) == 1 END
BEGIN BOOST_PP_VARIADIC_ELEM(0,BOOST_PP_SEQ_ENUM(SEQ)) == 4 END
BEGIN BOOST_PP_TUPLE_ELEM(2,BOOST_PP_SEQ_ELEM(0,BOOST_PP_VARIADIC_SEQ_TO_SEQ(SEQVAR))) == 8 END

#endif
