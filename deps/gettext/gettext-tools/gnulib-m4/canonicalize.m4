# canonicalize.m4 serial 16

dnl Copyright (C) 2003-2007, 2009-2010 Free Software Foundation, Inc.

dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

# Provides canonicalize_file_name and canonicalize_filename_mode, but does
# not provide or fix realpath.
AC_DEFUN([gl_FUNC_CANONICALIZE_FILENAME_MODE],
[
  AC_LIBOBJ([canonicalize])

  AC_REQUIRE([gl_USE_SYSTEM_EXTENSIONS])
  AC_CHECK_FUNCS_ONCE([canonicalize_file_name])
  AC_REQUIRE([gl_DOUBLE_SLASH_ROOT])
  AC_REQUIRE([gl_FUNC_REALPATH_WORKS])
  if test $ac_cv_func_canonicalize_file_name = no; then
    HAVE_CANONICALIZE_FILE_NAME=0
  elif test "$gl_cv_func_realpath_works" != yes; then
    REPLACE_CANONICALIZE_FILE_NAME=1
  fi
])

# Provides canonicalize_file_name and realpath.
AC_DEFUN([gl_CANONICALIZE_LGPL],
[
  AC_REQUIRE([gl_CANONICALIZE_LGPL_SEPARATE])
  if test $ac_cv_func_canonicalize_file_name = no; then
    HAVE_CANONICALIZE_FILE_NAME=0
    AC_LIBOBJ([canonicalize-lgpl])
    if test $ac_cv_func_realpath = no; then
      HAVE_REALPATH=0
    elif test "$gl_cv_func_realpath_works" != yes; then
      REPLACE_REALPATH=1
    fi
  elif test "$gl_cv_func_realpath_works" != yes; then
    AC_LIBOBJ([canonicalize-lgpl])
    REPLACE_REALPATH=1
    REPLACE_CANONICALIZE_FILE_NAME=1
  fi
])

# Like gl_CANONICALIZE_LGPL, except prepare for separate compilation
# (no AC_LIBOBJ).
AC_DEFUN([gl_CANONICALIZE_LGPL_SEPARATE],
[
  AC_REQUIRE([gl_USE_SYSTEM_EXTENSIONS])
  AC_CHECK_FUNCS_ONCE([canonicalize_file_name getcwd readlink])
  AC_REQUIRE([gl_DOUBLE_SLASH_ROOT])
  AC_REQUIRE([gl_FUNC_REALPATH_WORKS])
  AC_CHECK_HEADERS_ONCE([sys/param.h])
])

# Check whether realpath works.  Assume that if a platform has both
# realpath and canonicalize_file_name, but the former is broken, then
# so is the latter.
AC_DEFUN([gl_FUNC_REALPATH_WORKS],
[
  AC_CHECK_FUNCS_ONCE([realpath])
  AC_CACHE_CHECK([whether realpath works], [gl_cv_func_realpath_works], [
    touch conftest.a
    AC_RUN_IFELSE([
      AC_LANG_PROGRAM([[
        #include <stdlib.h>
      ]], [[
        char *name1 = realpath ("conftest.a", NULL);
        char *name2 = realpath ("conftest.b/../conftest.a", NULL);
        char *name3 = realpath ("conftest.a/", NULL);
        return !(name1 && *name1 == '/' && !name2 && !name3);
      ]])
    ], [gl_cv_func_realpath_works=yes], [gl_cv_func_realpath_works=no],
       [gl_cv_func_realpath_works="guessing no"])
  ])
  if test "$gl_cv_func_realpath_works" = yes; then
    AC_DEFINE([FUNC_REALPATH_WORKS], [1], [Define to 1 if realpath()
      can malloc memory, always gives an absolute path, and handles
      trailing slash correctly.])
  fi
])
