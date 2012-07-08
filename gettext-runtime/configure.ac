dnl Configuration for the gettext-runtime directory of GNU gettext
dnl Copyright (C) 1995-1999, 2000-2009 Free Software Foundation, Inc.
dnl
dnl This program is free software: you can redistribute it and/or modify
dnl it under the terms of the GNU General Public License as published by
dnl the Free Software Foundation; either version 3 of the License, or
dnl (at your option) any later version.
dnl
dnl This program is distributed in the hope that it will be useful,
dnl but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
dnl GNU General Public License for more details.
dnl
dnl You should have received a copy of the GNU General Public License
dnl along with this program.  If not, see <http://www.gnu.org/licenses/>.

dnl Process this file with autoconf to produce a configure script.

AC_PREREQ([2.59])
AC_INIT
AC_CONFIG_SRCDIR([intl/dcigettext.c])
AC_CONFIG_AUX_DIR([../build-aux])
. $srcdir/../version.sh
gl_INIT_PACKAGE([gettext-runtime], [$VERSION_NUMBER])
AM_INIT_AUTOMAKE([silent-rules])
AM_CONFIG_HEADER([config.h])

dnl Installation directories.
test "$docdir" != '${datarootdir}/doc/${PACKAGE}' || docdir='${datarootdir}/doc/gettext'

dnl Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_YACC

gt_JAVA_CHOICE

gt_JAVACOMP([1.3], [1.1])
AC_CHECK_PROG([JAR], [jar], [jar])
if test -n "$HAVE_JAVACOMP" && test -n "$JAR" && test "$JAVA_CHOICE" != no; then
  BUILDJAVA=yes
else
  BUILDJAVA=no
fi
AC_SUBST([BUILDJAVA])

gt_CSHARPCOMP
if test -n "$HAVE_CSHARPCOMP" && test "$CSHARP_CHOICE" != no; then
  BUILDCSHARP=yes
else
  BUILDCSHARP=no
fi
AC_SUBST([BUILDCSHARP])

dnl Check for host type.
AC_CANONICAL_HOST

dnl Checks for compiler output filename suffixes.
AC_OBJEXT
AC_EXEEXT

dnl Make sure we see all GNU and Solaris extensions.
gl_EARLY

dnl Check for build configuration.

gl_WOE32_DLL

LT_INIT([win32-dll])

dnl Prepares the libtool configuration for handling of Windows resources, and
dnl sets the RC variable to a program that compiles Windows resource files.
LT_LANG([Windows Resource])

dnl On mingw and Cygwin, we can activate special Makefile rules which add
dnl version information to the shared libraries and executables.
case "$host_os" in
  mingw* | cygwin*) is_woe32=yes ;;
  *) is_woe32=no ;;
esac
AM_CONDITIONAL([WOE32], [test $is_woe32 = yes])

dnl Checks for libraries.

dnl These are the only lines required to internationalize the package.
dnl (OK, not quite, the AC_CONFIG_FILES has also some parts.)
AM_GNU_GETTEXT([use-libtool], [need-ngettext])

dnl Checks for header files, functions and declarations.
gl_INIT
AC_TYPE_SIZE_T
AC_CHECK_FUNCS([setlocale])

dnl Put some default definitions into config.h.
AH_BOTTOM([
/* On Windows, variables that may be in a DLL must be marked specially.  */
#if (defined _MSC_VER && defined _DLL) && !defined IN_RELOCWRAPPER
# define DLL_VARIABLE __declspec (dllimport)
#else
# define DLL_VARIABLE
#endif

/* Extra OS/2 (emx+gcc) defines.  */
#ifdef __EMX__
# include "intl/os2compat.h"
#endif
])

dnl Check for tools needed for formatting the documentation.
ac_aux_dir_abs=`cd $ac_aux_dir && pwd`
AC_PATH_PROG([PERL], [perl], [$ac_aux_dir_abs/missing perl])

dnl Check for tools needed for formatting man pages.
CROSS_COMPILING=$cross_compiling
AC_SUBST([CROSS_COMPILING])

dnl Checks for optional programs for the tests/lang-* tests.
gt_PROG_ANSI_CXX

dnl Checks for special libraries for the tests/test-lock test.
dnl On some systems, sched_yield is in librt, rather than in libpthread.
LIBSCHED=
if test $gl_threads_api = posix; then
  dnl Solaris has sched_yield in librt, not in libpthread or libc.
  AC_CHECK_LIB([rt], [sched_yield], [LIBSCHED=-lrt],
    [dnl Solaris 2.5.1, 2.6 has sched_yield in libposix4, not librt.
     AC_CHECK_LIB([posix4], [sched_yield], [LIBSCHED=-lposix4])])
fi
AC_SUBST([LIBSCHED])

dnl Determine whether to build libasprintf.
AC_ARG_ENABLE([libasprintf],
  [  --disable-libasprintf   do not build libasprintf],
  [if test "$enableval" != no; then
     SUBDIR_libasprintf=libasprintf
   else
     SUBDIR_libasprintf=
   fi
  ], [SUBDIR_libasprintf=libasprintf])
if test "$CXX" = ":"; then
  SUBDIR_libasprintf=
fi
AM_CONDITIONAL([ENABLE_LIBASPRINTF], [test -n "$SUBDIR_libasprintf"])

AC_CONFIG_SUBDIRS([libasprintf])

AC_CONFIG_FILES([Makefile])

AC_CONFIG_FILES([doc/Makefile])

AC_CONFIG_FILES([intl/Makefile])

AC_CONFIG_FILES([intl-java/Makefile])

AC_CONFIG_FILES([intl-csharp/Makefile])

AC_CONFIG_FILES([gnulib-lib/Makefile])

AC_CONFIG_FILES([src/Makefile])
AC_CONFIG_FILES([src/gettext.sh], [chmod a+x src/gettext.sh])

AC_CONFIG_FILES([po/Makefile.in])

AC_CONFIG_FILES([man/Makefile],
                [FIX_MAKEFILE_DISTRIB])
AC_CONFIG_FILES([man/x-to-1])

AC_CONFIG_FILES([m4/Makefile])

AC_CONFIG_FILES([tests/Makefile])

AC_OUTPUT
