# iconv_open.m4 serial 7
dnl Copyright (C) 2007-2010 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_ICONV_OPEN],
[
  AC_REQUIRE([AM_ICONV])
  AC_REQUIRE([AC_CANONICAL_HOST])
  AC_REQUIRE([gl_ICONV_H_DEFAULTS])
  if test "$am_cv_func_iconv" = yes; then
    dnl Provide the <iconv.h> override, for the sake of the C++ aliases.
    gl_REPLACE_ICONV_H
    dnl Test whether iconv_open accepts standardized encoding names.
    dnl We know that GNU libiconv and GNU libc do.
    AC_EGREP_CPP([gnu_iconv], [
      #include <iconv.h>
      #if defined _LIBICONV_VERSION || defined __GLIBC__
       gnu_iconv
      #endif
      ], [gl_func_iconv_gnu=yes], [gl_func_iconv_gnu=no])
    if test $gl_func_iconv_gnu = no; then
      iconv_flavor=
      case "$host_os" in
        aix*)     iconv_flavor=ICONV_FLAVOR_AIX ;;
        irix*)    iconv_flavor=ICONV_FLAVOR_IRIX ;;
        hpux*)    iconv_flavor=ICONV_FLAVOR_HPUX ;;
        osf*)     iconv_flavor=ICONV_FLAVOR_OSF ;;
        solaris*) iconv_flavor=ICONV_FLAVOR_SOLARIS ;;
      esac
      if test -n "$iconv_flavor"; then
        AC_DEFINE_UNQUOTED([ICONV_FLAVOR], [$iconv_flavor],
          [Define to a symbolic name denoting the flavor of iconv_open()
           implementation.])
        gl_REPLACE_ICONV_OPEN
      fi
    fi
  fi
])

AC_DEFUN([gl_REPLACE_ICONV_OPEN],
[
  gl_REPLACE_ICONV_H
  REPLACE_ICONV_OPEN=1
  AC_LIBOBJ([iconv_open])
])

AC_DEFUN([gl_FUNC_ICONV_OPEN_UTF],
[
  AC_REQUIRE([gl_FUNC_ICONV_OPEN])
  AC_REQUIRE([AC_CANONICAL_HOST]) dnl for cross-compiles
  AC_REQUIRE([gl_ICONV_H_DEFAULTS])
  if test "$am_cv_func_iconv" = yes; then
    if test -n "$am_cv_proto_iconv_arg1"; then
      ICONV_CONST="const"
    else
      ICONV_CONST=
    fi
    AC_SUBST([ICONV_CONST])
    AC_CACHE_CHECK([whether iconv supports conversion between UTF-8 and UTF-{16,32}{BE,LE}],
      [gl_cv_func_iconv_supports_utf],
      [
        save_LIBS="$LIBS"
        LIBS="$LIBS $LIBICONV"
        AC_TRY_RUN([
#include <iconv.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define ASSERT(expr) if (!(expr)) return 1;
int main ()
{
  /* Test conversion from UTF-8 to UTF-16BE with no errors.  */
  {
    static const char input[] =
      "Japanese (\346\227\245\346\234\254\350\252\236) [\360\235\224\215\360\235\224\236\360\235\224\255]";
    static const char expected[] =
      "\000J\000a\000p\000a\000n\000e\000s\000e\000 \000(\145\345\147\054\212\236\000)\000 \000[\330\065\335\015\330\065\335\036\330\065\335\055\000]";
    iconv_t cd;
    char buf[100];
    const char *inptr;
    size_t inbytesleft;
    char *outptr;
    size_t outbytesleft;
    size_t res;
    cd = iconv_open ("UTF-16BE", "UTF-8");
    ASSERT (cd != (iconv_t)(-1));
    inptr = input;
    inbytesleft = sizeof (input) - 1;
    outptr = buf;
    outbytesleft = sizeof (buf);
    res = iconv (cd,
                 (ICONV_CONST char **) &inptr, &inbytesleft,
                 &outptr, &outbytesleft);
    ASSERT (res == 0 && inbytesleft == 0);
    ASSERT (outptr == buf + (sizeof (expected) - 1));
    ASSERT (memcmp (buf, expected, sizeof (expected) - 1) == 0);
    ASSERT (iconv_close (cd) == 0);
  }
  /* Test conversion from UTF-8 to UTF-16LE with no errors.  */
  {
    static const char input[] =
      "Japanese (\346\227\245\346\234\254\350\252\236) [\360\235\224\215\360\235\224\236\360\235\224\255]";
    static const char expected[] =
      "J\000a\000p\000a\000n\000e\000s\000e\000 \000(\000\345\145\054\147\236\212)\000 \000[\000\065\330\015\335\065\330\036\335\065\330\055\335]\000";
    iconv_t cd;
    char buf[100];
    const char *inptr;
    size_t inbytesleft;
    char *outptr;
    size_t outbytesleft;
    size_t res;
    cd = iconv_open ("UTF-16LE", "UTF-8");
    ASSERT (cd != (iconv_t)(-1));
    inptr = input;
    inbytesleft = sizeof (input) - 1;
    outptr = buf;
    outbytesleft = sizeof (buf);
    res = iconv (cd,
                 (ICONV_CONST char **) &inptr, &inbytesleft,
                 &outptr, &outbytesleft);
    ASSERT (res == 0 && inbytesleft == 0);
    ASSERT (outptr == buf + (sizeof (expected) - 1));
    ASSERT (memcmp (buf, expected, sizeof (expected) - 1) == 0);
    ASSERT (iconv_close (cd) == 0);
  }
  /* Test conversion from UTF-8 to UTF-32BE with no errors.  */
  {
    static const char input[] =
      "Japanese (\346\227\245\346\234\254\350\252\236) [\360\235\224\215\360\235\224\236\360\235\224\255]";
    static const char expected[] =
      "\000\000\000J\000\000\000a\000\000\000p\000\000\000a\000\000\000n\000\000\000e\000\000\000s\000\000\000e\000\000\000 \000\000\000(\000\000\145\345\000\000\147\054\000\000\212\236\000\000\000)\000\000\000 \000\000\000[\000\001\325\015\000\001\325\036\000\001\325\055\000\000\000]";
    iconv_t cd;
    char buf[100];
    const char *inptr;
    size_t inbytesleft;
    char *outptr;
    size_t outbytesleft;
    size_t res;
    cd = iconv_open ("UTF-32BE", "UTF-8");
    ASSERT (cd != (iconv_t)(-1));
    inptr = input;
    inbytesleft = sizeof (input) - 1;
    outptr = buf;
    outbytesleft = sizeof (buf);
    res = iconv (cd,
                 (ICONV_CONST char **) &inptr, &inbytesleft,
                 &outptr, &outbytesleft);
    ASSERT (res == 0 && inbytesleft == 0);
    ASSERT (outptr == buf + (sizeof (expected) - 1));
    ASSERT (memcmp (buf, expected, sizeof (expected) - 1) == 0);
    ASSERT (iconv_close (cd) == 0);
  }
  /* Test conversion from UTF-8 to UTF-32LE with no errors.  */
  {
    static const char input[] =
      "Japanese (\346\227\245\346\234\254\350\252\236) [\360\235\224\215\360\235\224\236\360\235\224\255]";
    static const char expected[] =
      "J\000\000\000a\000\000\000p\000\000\000a\000\000\000n\000\000\000e\000\000\000s\000\000\000e\000\000\000 \000\000\000(\000\000\000\345\145\000\000\054\147\000\000\236\212\000\000)\000\000\000 \000\000\000[\000\000\000\015\325\001\000\036\325\001\000\055\325\001\000]\000\000\000";
    iconv_t cd;
    char buf[100];
    const char *inptr;
    size_t inbytesleft;
    char *outptr;
    size_t outbytesleft;
    size_t res;
    cd = iconv_open ("UTF-32LE", "UTF-8");
    ASSERT (cd != (iconv_t)(-1));
    inptr = input;
    inbytesleft = sizeof (input) - 1;
    outptr = buf;
    outbytesleft = sizeof (buf);
    res = iconv (cd,
                 (ICONV_CONST char **) &inptr, &inbytesleft,
                 &outptr, &outbytesleft);
    ASSERT (res == 0 && inbytesleft == 0);
    ASSERT (outptr == buf + (sizeof (expected) - 1));
    ASSERT (memcmp (buf, expected, sizeof (expected) - 1) == 0);
    ASSERT (iconv_close (cd) == 0);
  }
  /* Test conversion from UTF-16BE to UTF-8 with no errors.
     This test fails on NetBSD 3.0.  */
  {
    static const char input[] =
      "\000J\000a\000p\000a\000n\000e\000s\000e\000 \000(\145\345\147\054\212\236\000)\000 \000[\330\065\335\015\330\065\335\036\330\065\335\055\000]";
    static const char expected[] =
      "Japanese (\346\227\245\346\234\254\350\252\236) [\360\235\224\215\360\235\224\236\360\235\224\255]";
    iconv_t cd;
    char buf[100];
    const char *inptr;
    size_t inbytesleft;
    char *outptr;
    size_t outbytesleft;
    size_t res;
    cd = iconv_open ("UTF-8", "UTF-16BE");
    ASSERT (cd != (iconv_t)(-1));
    inptr = input;
    inbytesleft = sizeof (input) - 1;
    outptr = buf;
    outbytesleft = sizeof (buf);
    res = iconv (cd,
                 (ICONV_CONST char **) &inptr, &inbytesleft,
                 &outptr, &outbytesleft);
    ASSERT (res == 0 && inbytesleft == 0);
    ASSERT (outptr == buf + (sizeof (expected) - 1));
    ASSERT (memcmp (buf, expected, sizeof (expected) - 1) == 0);
    ASSERT (iconv_close (cd) == 0);
  }
  return 0;
}], [gl_cv_func_iconv_supports_utf=yes], [gl_cv_func_iconv_supports_utf=no],
          [
           dnl We know that GNU libiconv, GNU libc, and Solaris >= 9 do.
           dnl OSF/1 5.1 has these encodings, but inserts a BOM in the "to"
           dnl direction.
           gl_cv_func_iconv_supports_utf=no
           if test $gl_func_iconv_gnu = yes; then
             gl_cv_func_iconv_supports_utf=yes
           else
changequote(,)dnl
             case "$host_os" in
               solaris2.9 | solaris2.1[0-9]) gl_cv_func_iconv_supports_utf=yes ;;
             esac
changequote([,])dnl
           fi
          ])
        LIBS="$save_LIBS"
      ])
    if test $gl_cv_func_iconv_supports_utf = no; then
      REPLACE_ICONV_UTF=1
      AC_DEFINE([REPLACE_ICONV_UTF], [1],
        [Define if the iconv() functions are enhanced to handle the UTF-{16,32}{BE,LE} encodings.])
      REPLACE_ICONV=1
      gl_REPLACE_ICONV_OPEN
      AC_LIBOBJ([iconv])
      AC_LIBOBJ([iconv_close])
    fi
  fi
])
