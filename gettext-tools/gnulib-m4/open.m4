# open.m4 serial 8
dnl Copyright (C) 2007-2010 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_OPEN],
[
  AC_REQUIRE([AC_CANONICAL_HOST])
  case "$host_os" in
    mingw* | pw*)
      gl_REPLACE_OPEN
      ;;
    *)
      dnl open("foo/") should not create a file when the file name has a
      dnl trailing slash.  FreeBSD only has the problem on symlinks.
      AC_CHECK_FUNCS_ONCE([lstat])
      AC_CACHE_CHECK([whether open recognizes a trailing slash],
        [gl_cv_func_open_slash],
        [# Assume that if we have lstat, we can also check symlinks.
          if test $ac_cv_func_lstat = yes; then
            touch conftest.tmp
            ln -s conftest.tmp conftest.lnk
          fi
          AC_TRY_RUN([
#include <fcntl.h>
#if HAVE_UNISTD_H
# include <unistd.h>
#endif
int main ()
{
#if HAVE_LSTAT
  if (open ("conftest.lnk/", O_RDONLY) != -1) return 2;
#endif
  return open ("conftest.sl/", O_CREAT, 0600) >= 0;
}], [gl_cv_func_open_slash=yes], [gl_cv_func_open_slash=no],
            [
changequote(,)dnl
             case "$host_os" in
               freebsd*)        gl_cv_func_open_slash="guessing no" ;;
               solaris2.[0-9]*) gl_cv_func_open_slash="guessing no" ;;
               hpux*)           gl_cv_func_open_slash="guessing no" ;;
               *)               gl_cv_func_open_slash="guessing yes" ;;
             esac
changequote([,])dnl
            ])
          rm -f conftest.sl conftest.tmp conftest.lnk
        ])
      case "$gl_cv_func_open_slash" in
        *no)
          AC_DEFINE([OPEN_TRAILING_SLASH_BUG], [1],
            [Define to 1 if open() fails to recognize a trailing slash.])
          gl_REPLACE_OPEN
          ;;
      esac
      ;;
  esac
])

AC_DEFUN([gl_REPLACE_OPEN],
[
  AC_REQUIRE([gl_FCNTL_H_DEFAULTS])
  REPLACE_OPEN=1
  AC_LIBOBJ([open])
  gl_PREREQ_OPEN
])

# Prerequisites of lib/open.c.
AC_DEFUN([gl_PREREQ_OPEN],
[
  AC_REQUIRE([AC_C_INLINE])
  AC_REQUIRE([gl_PROMOTED_TYPE_MODE_T])
  :
])
