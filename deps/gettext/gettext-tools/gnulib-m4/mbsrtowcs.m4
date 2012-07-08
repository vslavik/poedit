# mbsrtowcs.m4 serial 6
dnl Copyright (C) 2008-2010 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_MBSRTOWCS],
[
  AC_REQUIRE([gl_WCHAR_H_DEFAULTS])

  AC_REQUIRE([AC_TYPE_MBSTATE_T])
  gl_MBSTATE_T_BROKEN

  AC_CHECK_FUNCS_ONCE([mbsrtowcs])
  if test $ac_cv_func_mbsrtowcs = no; then
    HAVE_MBSRTOWCS=0
  else
    if test $REPLACE_MBSTATE_T = 1; then
      REPLACE_MBSRTOWCS=1
    else
      gl_MBSRTOWCS_WORKS
      case "$gl_cv_func_mbsrtowcs_works" in
        *yes) ;;
        *) REPLACE_MBSRTOWCS=1 ;;
      esac
    fi
  fi
  if test $HAVE_MBSRTOWCS = 0 || test $REPLACE_MBSRTOWCS = 1; then
    gl_REPLACE_WCHAR_H
    AC_LIBOBJ([mbsrtowcs])
    AC_LIBOBJ([mbsrtowcs-state])
    gl_PREREQ_MBSRTOWCS
  fi
])

dnl Test whether mbsrtowcs works.
dnl Result is gl_cv_func_mbsrtowcs_works.

AC_DEFUN([gl_MBSRTOWCS_WORKS],
[
  AC_REQUIRE([AC_PROG_CC])
  AC_REQUIRE([gt_LOCALE_FR_UTF8])
  AC_REQUIRE([gt_LOCALE_JA])
  AC_REQUIRE([gt_LOCALE_ZH_CN])
  AC_REQUIRE([AC_CANONICAL_HOST]) dnl for cross-compiles
  AC_CACHE_CHECK([whether mbsrtowcs works],
    [gl_cv_func_mbsrtowcs_works],
    [
      dnl Initial guess, used when cross-compiling or when no suitable locale
      dnl is present.
changequote(,)dnl
      case "$host_os" in
                          # Guess no on HP-UX and Solaris.
        hpux* | solaris*) gl_cv_func_mbsrtowcs_works="guessing no" ;;
                          # Guess yes otherwise.
        *)                gl_cv_func_mbsrtowcs_works="guessing yes" ;;
      esac
changequote([,])dnl
      if test $LOCALE_FR_UTF8 != none || test $LOCALE_JA != none || test $LOCALE_ZH_CN != none; then
        AC_TRY_RUN([
#include <locale.h>
#include <string.h>
#include <wchar.h>
int main ()
{
  /* Test whether the function works when started with a conversion state
     in non-initial state.  This fails on HP-UX 11.11 and Solaris 10.  */
  if (setlocale (LC_ALL, "$LOCALE_FR_UTF8") != NULL)
    {
      const char input[] = "B\303\274\303\237er";
      mbstate_t state;

      memset (&state, '\0', sizeof (mbstate_t));
      if (mbrtowc (NULL, input + 1, 1, &state) == (size_t)(-2))
        if (!mbsinit (&state))
          {
            const char *src = input + 2;
            if (mbsrtowcs (NULL, &src, 10, &state) != 4)
              return 1;
          }
    }
  if (setlocale (LC_ALL, "$LOCALE_JA") != NULL)
    {
      const char input[] = "<\306\374\313\334\270\354>";
      mbstate_t state;

      memset (&state, '\0', sizeof (mbstate_t));
      if (mbrtowc (NULL, input + 3, 1, &state) == (size_t)(-2))
        if (!mbsinit (&state))
          {
            const char *src = input + 4;
            if (mbsrtowcs (NULL, &src, 10, &state) != 3)
              return 1;
          }
    }
  if (setlocale (LC_ALL, "$LOCALE_ZH_CN") != NULL)
    {
      const char input[] = "B\250\271\201\060\211\070er";
      mbstate_t state;

      memset (&state, '\0', sizeof (mbstate_t));
      if (mbrtowc (NULL, input + 1, 1, &state) == (size_t)(-2))
        if (!mbsinit (&state))
          {
            const char *src = input + 2;
            if (mbsrtowcs (NULL, &src, 10, &state) != 4)
              return 1;
          }
    }
  return 0;
}],
          [gl_cv_func_mbsrtowcs_works=yes],
          [gl_cv_func_mbsrtowcs_works=no],
          [:])
      fi
    ])
])

# Prerequisites of lib/mbsrtowcs.c.
AC_DEFUN([gl_PREREQ_MBSRTOWCS], [
  :
])
