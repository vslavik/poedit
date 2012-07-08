# rmdir.m4 serial 8
dnl Copyright (C) 2002, 2005, 2009-2010 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_RMDIR],
[
  AC_REQUIRE([gl_AC_DOS])
  AC_REQUIRE([gl_UNISTD_H_DEFAULTS])
  dnl Detect cygwin 1.5.x bug.
  AC_CACHE_CHECK([whether rmdir works], [gl_cv_func_rmdir_works],
    [mkdir conftest.dir
     touch conftest.file
     AC_RUN_IFELSE(
       [AC_LANG_PROGRAM(
         [[#include <stdio.h>
           #include <errno.h>
           #include <unistd.h>
]], [[return !rmdir ("conftest.file/") || errno != ENOTDIR
       || !rmdir ("conftest.dir/./");]])],
       [gl_cv_func_rmdir_works=yes], [gl_cv_func_rmdir_works=no],
       [gl_cv_func_rmdir_works="guessing no"])
     rm -rf conftest.dir conftest.file])
  if test x"$gl_cv_func_rmdir_works" != xyes; then
    REPLACE_RMDIR=1
    AC_LIBOBJ([rmdir])
  fi
])
