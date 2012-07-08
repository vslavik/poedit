# setenv.m4 serial 16
dnl Copyright (C) 2001-2004, 2006-2010 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_SETENV],
[
  AC_REQUIRE([gl_FUNC_SETENV_SEPARATE])
  if test $HAVE_SETENV$REPLACE_SETENV != 10; then
    AC_LIBOBJ([setenv])
  fi
])

# Like gl_FUNC_SETENV, except prepare for separate compilation (no AC_LIBOBJ).
AC_DEFUN([gl_FUNC_SETENV_SEPARATE],
[
  AC_REQUIRE([gl_STDLIB_H_DEFAULTS])
  AC_CHECK_FUNCS_ONCE([setenv])
  if test $ac_cv_func_setenv = no; then
    HAVE_SETENV=0
  else
    AC_CACHE_CHECK([whether setenv validates arguments],
      [gl_cv_func_setenv_works],
      [AC_RUN_IFELSE([AC_LANG_PROGRAM([[
       #include <stdlib.h>
       #include <errno.h>
       #include <string.h>
      ]], [[
       if (setenv ("", "", 0) != -1) return 1;
       if (errno != EINVAL) return 2;
       if (setenv ("a", "=", 1) != 0) return 3;
       if (strcmp (getenv ("a"), "=") != 0) return 4;
      ]])],
      [gl_cv_func_setenv_works=yes], [gl_cv_func_setenv_works=no],
      [gl_cv_func_setenv_works="guessing no"])])
    if test "$gl_cv_func_setenv_works" != yes; then
      REPLACE_SETENV=1
      AC_LIBOBJ([setenv])
    fi
  fi
  gl_PREREQ_SETENV
])

AC_DEFUN([gl_FUNC_UNSETENV],
[
  AC_REQUIRE([gl_STDLIB_H_DEFAULTS])
  AC_CHECK_FUNCS([unsetenv])
  if test $ac_cv_func_unsetenv = no; then
    HAVE_UNSETENV=0
    AC_LIBOBJ([unsetenv])
    gl_PREREQ_UNSETENV
  else
    dnl Some BSDs return void, failing to do error checking.
    AC_CACHE_CHECK([for unsetenv() return type], [gt_cv_func_unsetenv_ret],
      [AC_TRY_COMPILE([#include <stdlib.h>
extern
#ifdef __cplusplus
"C"
#endif
#if defined(__STDC__) || defined(__cplusplus)
int unsetenv (const char *name);
#else
int unsetenv();
#endif
], , gt_cv_func_unsetenv_ret='int', gt_cv_func_unsetenv_ret='void')])
    if test $gt_cv_func_unsetenv_ret = 'void'; then
      AC_DEFINE([VOID_UNSETENV], [1], [Define to 1 if unsetenv returns void
       instead of int.])
      REPLACE_UNSETENV=1
      AC_LIBOBJ([unsetenv])
    fi

    dnl Solaris 10 unsetenv does not remove all copies of a name.
    AC_CACHE_CHECK([whether unsetenv works on duplicates],
      [gl_cv_func_unsetenv_works],
      [AC_RUN_IFELSE([AC_LANG_PROGRAM([[
       #include <stdlib.h>
      ]], [[
       char entry[] = "b=2";
       if (putenv ((char *) "a=1")) return 1;
       if (putenv (entry)) return 2;
       entry[0] = 'a';
       unsetenv ("a");
       if (getenv ("a")) return 3;
      ]])],
      [gl_cv_func_unsetenv_works=yes], [gl_cv_func_unsetenv_works=no],
      [gl_cv_func_unsetenv_works="guessing no"])])
    if test "$gl_cv_func_unsetenv_works" != yes; then
      REPLACE_UNSETENV=1
      AC_LIBOBJ([unsetenv])
    fi
  fi
])

# Prerequisites of lib/setenv.c.
AC_DEFUN([gl_PREREQ_SETENV],
[
  AC_REQUIRE([AC_FUNC_ALLOCA])
  AC_REQUIRE([gl_ENVIRON])
  AC_CHECK_HEADERS_ONCE([unistd.h])
  AC_CHECK_HEADERS([search.h])
  AC_CHECK_FUNCS([tsearch])
])

# Prerequisites of lib/unsetenv.c.
AC_DEFUN([gl_PREREQ_UNSETENV],
[
  AC_REQUIRE([gl_ENVIRON])
  AC_CHECK_HEADERS_ONCE([unistd.h])
])
