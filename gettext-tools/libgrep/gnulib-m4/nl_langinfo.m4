# nl_langinfo.m4 serial 3
dnl Copyright (C) 2009, 2010 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_NL_LANGINFO],
[
  AC_REQUIRE([gl_LANGINFO_H_DEFAULTS])
  AC_REQUIRE([gl_LANGINFO_H])
  AC_CHECK_FUNCS_ONCE([nl_langinfo])
  if test $ac_cv_func_nl_langinfo = yes; then
    if test $HAVE_LANGINFO_CODESET = 1 && test $HAVE_LANGINFO_ERA = 1; then
      :
    else
      REPLACE_NL_LANGINFO=1
      AC_DEFINE([REPLACE_NL_LANGINFO], [1],
        [Define if nl_langinfo exists but is overridden by gnulib.])
      AC_LIBOBJ([nl_langinfo])
    fi
  else
    HAVE_NL_LANGINFO=0
    AC_LIBOBJ([nl_langinfo])
  fi
])
