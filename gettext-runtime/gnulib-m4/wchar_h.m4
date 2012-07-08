dnl A placeholder for ISO C99 <wchar.h>, for platforms that have issues.

dnl Copyright (C) 2007-2010 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

dnl Written by Eric Blake.

# wchar_h.m4 serial 33

AC_DEFUN([gl_WCHAR_H],
[
  AC_REQUIRE([gl_WCHAR_H_DEFAULTS])
  AC_REQUIRE([gl_WCHAR_H_INLINE_OK])
  dnl Prepare for creating substitute <wchar.h>.
  dnl Check for <wchar.h> (missing in Linux uClibc when built without wide
  dnl character support).
  dnl <wchar.h> is always overridden, because of GNULIB_POSIXCHECK.
  AC_CHECK_HEADERS_ONCE([wchar.h])
  gl_CHECK_NEXT_HEADERS([wchar.h])
  if test $ac_cv_header_wchar_h = yes; then
    HAVE_WCHAR_H=1
  else
    HAVE_WCHAR_H=0
  fi
  AC_SUBST([HAVE_WCHAR_H])

  AC_REQUIRE([gt_TYPE_WINT_T])
  if test $gt_cv_c_wint_t = yes; then
    HAVE_WINT_T=1
  else
    HAVE_WINT_T=0
  fi
  AC_SUBST([HAVE_WINT_T])

  dnl Check for declarations of anything we want to poison if the
  dnl corresponding gnulib module is not in use.
  gl_WARN_ON_USE_PREPARE([[
/* Some systems require additional headers.  */
#ifndef __GLIBC__
# include <stddef.h>
# include <stdio.h>
# include <time.h>
#endif
#include <wchar.h>
    ]], [btowc wctob mbsinit mbrtowc mbrlen mbsrtowcs mbsnrtowcs wcrtomb
    wcsrtombs wcsnrtombs wcwidth])
])

dnl Check whether <wchar.h> is usable at all.
AC_DEFUN([gl_WCHAR_H_INLINE_OK],
[
  dnl Test whether <wchar.h> suffers due to the transition from '__inline' to
  dnl 'gnu_inline'. See <http://sourceware.org/bugzilla/show_bug.cgi?id=4022>
  dnl and <http://gcc.gnu.org/bugzilla/show_bug.cgi?id=42440>. In summary,
  dnl glibc version 2.5 or older, together with gcc version 4.3 or newer and
  dnl the option -std=c99 or -std=gnu99, leads to a broken <wchar.h>.
  AC_CACHE_CHECK([whether <wchar.h> uses 'inline' correctly],
    [gl_cv_header_wchar_h_correct_inline],
    [gl_cv_header_wchar_h_correct_inline=yes
     AC_LANG_CONFTEST([
       AC_LANG_SOURCE([[#define wcstod renamed_wcstod
#include <wchar.h>
extern int zero (void);
int main () { return zero(); }
]])])
     if AC_TRY_EVAL([ac_compile]); then
       mv conftest.$ac_objext conftest1.$ac_objext
       AC_LANG_CONFTEST([
         AC_LANG_SOURCE([[#define wcstod renamed_wcstod
#include <wchar.h>
int zero (void) { return 0; }
]])])
       if AC_TRY_EVAL([ac_compile]); then
         mv conftest.$ac_objext conftest2.$ac_objext
         if $CC -o conftest$ac_exeext $CFLAGS $LDFLAGS conftest1.$ac_objext conftest2.$ac_objext $LIBS >&AS_MESSAGE_LOG_FD 2>&1; then
           :
         else
           gl_cv_header_wchar_h_correct_inline=no
         fi
       fi
     fi
     rm -f conftest1.$ac_objext conftest2.$ac_objext conftest$ac_exeext
    ])
  if test $gl_cv_header_wchar_h_correct_inline = no; then
    AC_MSG_ERROR([<wchar.h> cannot be used with this compiler ($CC $CFLAGS $CPPFLAGS).
This is a known interoperability problem of glibc <= 2.5 with gcc >= 4.3 in
C99 mode. You have four options:
  - Add the flag -fgnu89-inline to CC and reconfigure, or
  - Fix your include files, using parts of
    <http://sourceware.org/git/?p=glibc.git;a=commitdiff;h=b037a293a48718af30d706c2e18c929d0e69a621>, or
  - Use a gcc version older than 4.3, or
  - Don't use the flags -std=c99 or -std=gnu99.
Configuration aborted.])
  fi
])

dnl Unconditionally enables the replacement of <wchar.h>.
AC_DEFUN([gl_REPLACE_WCHAR_H],
[
  dnl This is a no-op, because <wchar.h> is always overridden.
  :
])

AC_DEFUN([gl_WCHAR_MODULE_INDICATOR],
[
  dnl Use AC_REQUIRE here, so that the default settings are expanded once only.
  AC_REQUIRE([gl_WCHAR_H_DEFAULTS])
  gl_MODULE_INDICATOR_SET_VARIABLE([$1])
  dnl Define it also as a C macro, for the benefit of the unit tests.
  gl_MODULE_INDICATOR_FOR_TESTS([$1])
])

AC_DEFUN([gl_WCHAR_H_DEFAULTS],
[
  GNULIB_BTOWC=0;      AC_SUBST([GNULIB_BTOWC])
  GNULIB_WCTOB=0;      AC_SUBST([GNULIB_WCTOB])
  GNULIB_MBSINIT=0;    AC_SUBST([GNULIB_MBSINIT])
  GNULIB_MBRTOWC=0;    AC_SUBST([GNULIB_MBRTOWC])
  GNULIB_MBRLEN=0;     AC_SUBST([GNULIB_MBRLEN])
  GNULIB_MBSRTOWCS=0;  AC_SUBST([GNULIB_MBSRTOWCS])
  GNULIB_MBSNRTOWCS=0; AC_SUBST([GNULIB_MBSNRTOWCS])
  GNULIB_WCRTOMB=0;    AC_SUBST([GNULIB_WCRTOMB])
  GNULIB_WCSRTOMBS=0;  AC_SUBST([GNULIB_WCSRTOMBS])
  GNULIB_WCSNRTOMBS=0; AC_SUBST([GNULIB_WCSNRTOMBS])
  GNULIB_WCWIDTH=0;    AC_SUBST([GNULIB_WCWIDTH])
  dnl Assume proper GNU behavior unless another module says otherwise.
  HAVE_BTOWC=1;         AC_SUBST([HAVE_BTOWC])
  HAVE_MBSINIT=1;       AC_SUBST([HAVE_MBSINIT])
  HAVE_MBRTOWC=1;       AC_SUBST([HAVE_MBRTOWC])
  HAVE_MBRLEN=1;        AC_SUBST([HAVE_MBRLEN])
  HAVE_MBSRTOWCS=1;     AC_SUBST([HAVE_MBSRTOWCS])
  HAVE_MBSNRTOWCS=1;    AC_SUBST([HAVE_MBSNRTOWCS])
  HAVE_WCRTOMB=1;       AC_SUBST([HAVE_WCRTOMB])
  HAVE_WCSRTOMBS=1;     AC_SUBST([HAVE_WCSRTOMBS])
  HAVE_WCSNRTOMBS=1;    AC_SUBST([HAVE_WCSNRTOMBS])
  HAVE_DECL_WCTOB=1;    AC_SUBST([HAVE_DECL_WCTOB])
  HAVE_DECL_WCWIDTH=1;  AC_SUBST([HAVE_DECL_WCWIDTH])
  REPLACE_MBSTATE_T=0;  AC_SUBST([REPLACE_MBSTATE_T])
  REPLACE_BTOWC=0;      AC_SUBST([REPLACE_BTOWC])
  REPLACE_WCTOB=0;      AC_SUBST([REPLACE_WCTOB])
  REPLACE_MBSINIT=0;    AC_SUBST([REPLACE_MBSINIT])
  REPLACE_MBRTOWC=0;    AC_SUBST([REPLACE_MBRTOWC])
  REPLACE_MBRLEN=0;     AC_SUBST([REPLACE_MBRLEN])
  REPLACE_MBSRTOWCS=0;  AC_SUBST([REPLACE_MBSRTOWCS])
  REPLACE_MBSNRTOWCS=0; AC_SUBST([REPLACE_MBSNRTOWCS])
  REPLACE_WCRTOMB=0;    AC_SUBST([REPLACE_WCRTOMB])
  REPLACE_WCSRTOMBS=0;  AC_SUBST([REPLACE_WCSRTOMBS])
  REPLACE_WCSNRTOMBS=0; AC_SUBST([REPLACE_WCSNRTOMBS])
  REPLACE_WCWIDTH=0;    AC_SUBST([REPLACE_WCWIDTH])
])
