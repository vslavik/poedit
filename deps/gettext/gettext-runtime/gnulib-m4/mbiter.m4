# mbiter.m4 serial 5
dnl Copyright (C) 2005, 2008, 2009, 2010 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

dnl autoconf tests required for use of mbiter.h
dnl From Bruno Haible.

AC_DEFUN([gl_MBITER],
[
  AC_REQUIRE([AC_C_INLINE])
  AC_REQUIRE([AC_TYPE_MBSTATE_T])
  dnl The following line is that so the user can test HAVE_MBRTOWC before
  dnl #include "mbiter.h" or "mbuiter.h". It can be removed in 2010.
  AC_REQUIRE([AC_FUNC_MBRTOWC])
  :
])
