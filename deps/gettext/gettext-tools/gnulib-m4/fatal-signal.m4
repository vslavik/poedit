# fatal-signal.m4 serial 8
dnl Copyright (C) 2003-2004, 2006, 2008-2010 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FATAL_SIGNAL],
[
  AC_REQUIRE([AC_C_INLINE])
  AC_REQUIRE([gt_TYPE_SIG_ATOMIC_T])
  AC_CHECK_HEADERS_ONCE([unistd.h])
  gl_PREREQ_SIG_HANDLER_H
])
