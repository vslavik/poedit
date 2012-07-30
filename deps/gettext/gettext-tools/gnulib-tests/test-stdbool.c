/* Test of <stdbool.h> substitute.
   Copyright (C) 2002-2007, 2009-2010 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* Written by Bruno Haible <bruno@clisp.org>, 2007.  */

#include <config.h>

#include <stdbool.h>

#ifndef bool
 "error: bool is not defined"
#endif
#ifndef false
 "error: false is not defined"
#endif
#if false
 "error: false is not 0"
#endif
#ifndef true
 "error: true is not defined"
#endif
#if true != 1
 "error: true is not 1"
#endif
#ifndef __bool_true_false_are_defined
 "error: __bool_true_false_are_defined is not defined"
#endif

#if 0 /* Cannot be guaranteed with gnulib's <stdbool.h>.  */
struct s { _Bool s: 1; _Bool t; } s;
#endif

char a[true == 1 ? 1 : -1];
char b[false == 0 ? 1 : -1];
char c[__bool_true_false_are_defined == 1 ? 1 : -1];
#if 0 /* Cannot be guaranteed with gnulib's <stdbool.h>, at least,
not for all compilers.  */
char d[(bool) 0.5 == true ? 1 : -1];
bool e = &s;
char f[(_Bool) 0.0 == false ? 1 : -1];
#endif
char g[true];
char h[sizeof (_Bool)];
#if 0 /* See above.  */
char i[sizeof s.t];
#endif
enum { j = false, k = true, l = false * true, m = true * 256 };
_Bool n[m];
char o[sizeof n == m * sizeof n[0] ? 1 : -1];
char p[-1 - (_Bool) 0 < 0 && -1 - (bool) 0 < 0 ? 1 : -1];
#if 0 /* Cannot be guaranteed with gnulib's <stdbool.h>.  */
#if defined __xlc__ || defined __GNUC__
 /* Catch a bug in IBM AIX xlc compiler version 6.0.0.0
    reported by James Lemley on 2005-10-05; see
    http://lists.gnu.org/archive/html/bug-coreutils/2005-10/msg00086.html
    This test is not quite right, since xlc is allowed to
    reject this program, as the initializer for xlcbug is
    not one of the forms that C requires support for.
    However, doing the test right would require a run-time
    test, and that would make cross-compilation harder.
    Let us hope that IBM fixes the xlc bug, and also adds
    support for this kind of constant expression.  In the
    meantime, this test will reject xlc, which is OK, since
    our stdbool.h substitute should suffice.  We also test
    this with GCC, where it should work, to detect more
    quickly whether someone messes up the test in the
    future.  */
 char digs[] = "0123456789";
 int xlcbug = 1 / (&(digs + 5)[-2 + (bool) 1] == &digs[4] ? 1 : -1);
#endif
#endif
/* Catch a bug in an HP-UX C compiler.  See
   http://gcc.gnu.org/ml/gcc-patches/2003-12/msg02303.html
   http://lists.gnu.org/archive/html/bug-coreutils/2005-11/msg00161.html
 */
_Bool q = true;
_Bool *pq = &q;

int
main ()
{
  return 0;
}
