# mkdtemp.m4 serial 6
dnl Copyright (C) 2001-2003, 2006-2007, 2009-2010 Free Software Foundation,
dnl Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gt_FUNC_MKDTEMP],
[
  AC_REQUIRE([gl_STDLIB_H_DEFAULTS])
  AC_REPLACE_FUNCS([mkdtemp])
  if test $ac_cv_func_mkdtemp = no; then
    HAVE_MKDTEMP=0
    gl_PREREQ_MKDTEMP
  fi
])

# Prerequisites of lib/mkdtemp.c
AC_DEFUN([gl_PREREQ_MKDTEMP],
[:
])
