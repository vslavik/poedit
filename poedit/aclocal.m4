dnl aclocal.m4 generated automatically by aclocal 1.4

dnl Copyright (C) 1994, 1995-8, 1999 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

dnl This program is distributed in the hope that it will be useful,
dnl but WITHOUT ANY WARRANTY, to the extent permitted by law; without
dnl even the implied warranty of MERCHANTABILITY or FITNESS FOR A
dnl PARTICULAR PURPOSE.

dnl ---------------------------------------------------------------------------
dnl WX_PATH_WXCONFIG(VERSION, [ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]])
dnl
dnl Test for wxWindows, and define WX_CFLAGS and WX_LIBS. Set WX_CONFIG
dnl environment variable to override the default name of the wx-config script
dnl to use.
dnl ---------------------------------------------------------------------------

AC_DEFUN(WX_PATH_WXCONFIG,
[
dnl 
dnl Get the cflags and libraries from the wx-config script
dnl
AC_ARG_WITH(wx-prefix, [  --with-wx-prefix=PREFIX Prefix where wxWindows is installed (optional)],
            wx_config_prefix="$withval", wx_config_prefix="")
AC_ARG_WITH(wx-exec-prefix,[  --with-wx-exec-prefix=PREFIX Exec prefix where wxWindows is installed (optional)],
            wx_config_exec_prefix="$withval", wx_config_exec_prefix="")
AC_ARG_WITH(wx-config, [  --with-wx-config=CONFIG Name of wx-config script to use (optional)],
            wx_config="$withval", wx_config="")

  dnl deal with optional prefixes
  if test x$wx_config_exec_prefix != x ; then
     wx_config_args="$wx_config_args --exec-prefix=$wx_config_exec_prefix"
     if test x${WX_CONFIG+set} != xset ; then
        WX_CONFIG=$wx_config_exec_prefix/bin/wx-config
     fi
  fi
  if test x$wx_config_prefix != x ; then
     wx_config_args="$wx_config_args --prefix=$wx_config_prefix"
     if test x${WX_CONFIG+set} != xset ; then
        WX_CONFIG=$wx_config_prefix/bin/wx-config
     fi
  fi

  dnl deal with optional wx-config
  if test x$wx_config != x ; then
     WX_CONFIG=$wx_config
  fi

  if test x${WX_CONFIG+set} != xset ; then
     WX_CONFIG=wx-config
  fi
  AC_PATH_PROG(WX_CONFIG, $WX_CONFIG, no)

  min_wx_version=ifelse([$1], ,2.2.1,$1)
  AC_MSG_CHECKING(for wxWindows version >= $min_wx_version)
  no_wx=""
  if test "$WX_CONFIG" = "no" ; then
    no_wx=yes
  else
    WX_CFLAGS=`$WX_CONFIG $wx_config_args --cflags`
    WX_LIBS=`$WX_CONFIG $wx_config_args --libs`
    wx_config_major_version=`$WX_CONFIG $wx_config_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\1/'`
    wx_config_minor_version=`$WX_CONFIG $wx_config_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\2/'`
    wx_config_micro_version=`$WX_CONFIG $wx_config_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\3/'`
  fi

  if test "x$no_wx" = x ; then
     AC_MSG_RESULT(yes (version $wx_config_major_version.$wx_config_minor_version.$wx_config_micro_version))
     ifelse([$2], , :, [$2])     
  else
     AC_MSG_RESULT(no)
     WX_CFLAGS=""
     WX_LIBS=""
     ifelse([$3], , :, [$3])
  fi
  AC_SUBST(WX_CFLAGS)
  AC_SUBST(WX_LIBS)
])






dnl ---------------------------------------------------------------------------
dnl FIND_GNOME
dnl
dnl Finds GNOME (if present) and fills in GNOME_DATA_DIR and USE_GNOME conditional
dnl ---------------------------------------------------------------------------

AC_DEFUN(FIND_GNOME,
[
  AC_MSG_CHECKING(for GNOME data directory)
  GNOME_DATA_DIR=`gnome-config --datadir 2>/dev/null`
  if test x$GNOME_DATA_DIR = x ; then
     AC_MSG_RESULT(no)
  else
     AC_MSG_RESULT($GNOME_DATA_DIR)
  fi
  AC_SUBST(GNOME_DATA_DIR)
  AM_CONDITIONAL(USE_GNOME, test x$GNOME_DATA_DIR != x)
])


dnl ---------------------------------------------------------------------------
dnl FIND_KDE
dnl
dnl Finds KDE (if present) and fills in KDE_DATA_DIR and USE_KDE conditional
dnl ---------------------------------------------------------------------------

AC_DEFUN(FIND_KDE,
[
  AC_MSG_CHECKING(for KDE data directory)
  if test x$KDEDIR = x ; then
     AC_MSG_RESULT(no)
     KDE_DATA_DIR=""
  else
     KDE_DATA_DIR=$KDEDIR/share
     AC_MSG_RESULT($KDE_DATA_DIR)
  fi
  AC_SUBST(KDE_DATA_DIR)
  AM_CONDITIONAL(USE_KDE, test x$KDEDIR != x)
])



dnl ---------------------------------------------------------------------------
dnl FIND_BERKELEY_DB
dnl
dnl Finds Berkeley DB (if present) and modifies LIBS and CXXFLAGS accordingly
dnl ---------------------------------------------------------------------------

AC_DEFUN(FIND_BERKELEY_DB,
[
  USE_TRANSMEM=1
  AC_CHECK_LIB(db-3.1, db_create,
     [
         DB_LIB=-ldb-3.1
     ],
     [
         AC_CHECK_LIB(db, db_create,
         [
             DB_LIB=-ldb
         ],
         [
             USE_TRANSMEM=0
             AC_MSG_WARN([

*** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** 

 Cannot find Berkeley DB 3.1, poEdit will build w/o translation memory feature!

*** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** 

             ])
         ])
     ])

  AC_CHECK_HEADER(db3/db.h,
     [
        CXXFLAGS="$CXXFLAGS -DUSE_REDHAT_DB3"
     ],
     [
        AC_CHECK_HEADER(db.h, [],
        [
           USE_TRANSMEM=0
           AC_MSG_RESULT(not found)
           AC_MSG_WARN([

*** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** 

 Cannot find Berkeley DB 3.1, poEdit will build w/o translation memory feature!

*** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** 

           ])
        ])
     ])

         
  if test x$USE_TRANSMEM != x ; then
      CXXFLAGS="$CXXFLAGS -DUSE_TRANSMEM"
  fi
  LIBS="$LIBS $DB_LIB"
  CXXFLAGS="$CXXFLAGS $DB_INCLUDE"
])


# Define a conditional.

AC_DEFUN(AM_CONDITIONAL,
[AC_SUBST($1_TRUE)
AC_SUBST($1_FALSE)
if $2; then
  $1_TRUE=
  $1_FALSE='#'
else
  $1_TRUE='#'
  $1_FALSE=
fi])

# Do all the work for Automake.  This macro actually does too much --
# some checks are only needed if your package does certain things.
# But this isn't really a big deal.

# serial 1

dnl Usage:
dnl AM_INIT_AUTOMAKE(package,version, [no-define])

AC_DEFUN(AM_INIT_AUTOMAKE,
[AC_REQUIRE([AC_PROG_INSTALL])
PACKAGE=[$1]
AC_SUBST(PACKAGE)
VERSION=[$2]
AC_SUBST(VERSION)
dnl test to see if srcdir already configured
if test "`cd $srcdir && pwd`" != "`pwd`" && test -f $srcdir/config.status; then
  AC_MSG_ERROR([source directory already configured; run "make distclean" there first])
fi
ifelse([$3],,
AC_DEFINE_UNQUOTED(PACKAGE, "$PACKAGE", [Name of package])
AC_DEFINE_UNQUOTED(VERSION, "$VERSION", [Version number of package]))
AC_REQUIRE([AM_SANITY_CHECK])
AC_REQUIRE([AC_ARG_PROGRAM])
dnl FIXME This is truly gross.
missing_dir=`cd $ac_aux_dir && pwd`
AM_MISSING_PROG(ACLOCAL, aclocal, $missing_dir)
AM_MISSING_PROG(AUTOCONF, autoconf, $missing_dir)
AM_MISSING_PROG(AUTOMAKE, automake, $missing_dir)
AM_MISSING_PROG(AUTOHEADER, autoheader, $missing_dir)
AM_MISSING_PROG(MAKEINFO, makeinfo, $missing_dir)
AC_REQUIRE([AC_PROG_MAKE_SET])])

#
# Check to make sure that the build environment is sane.
#

AC_DEFUN(AM_SANITY_CHECK,
[AC_MSG_CHECKING([whether build environment is sane])
# Just in case
sleep 1
echo timestamp > conftestfile
# Do `set' in a subshell so we don't clobber the current shell's
# arguments.  Must try -L first in case configure is actually a
# symlink; some systems play weird games with the mod time of symlinks
# (eg FreeBSD returns the mod time of the symlink's containing
# directory).
if (
   set X `ls -Lt $srcdir/configure conftestfile 2> /dev/null`
   if test "[$]*" = "X"; then
      # -L didn't work.
      set X `ls -t $srcdir/configure conftestfile`
   fi
   if test "[$]*" != "X $srcdir/configure conftestfile" \
      && test "[$]*" != "X conftestfile $srcdir/configure"; then

      # If neither matched, then we have a broken ls.  This can happen
      # if, for instance, CONFIG_SHELL is bash and it inherits a
      # broken ls alias from the environment.  This has actually
      # happened.  Such a system could not be considered "sane".
      AC_MSG_ERROR([ls -t appears to fail.  Make sure there is not a broken
alias in your environment])
   fi

   test "[$]2" = conftestfile
   )
then
   # Ok.
   :
else
   AC_MSG_ERROR([newly created file is older than distributed files!
Check your system clock])
fi
rm -f conftest*
AC_MSG_RESULT(yes)])

dnl AM_MISSING_PROG(NAME, PROGRAM, DIRECTORY)
dnl The program must properly implement --version.
AC_DEFUN(AM_MISSING_PROG,
[AC_MSG_CHECKING(for working $2)
# Run test in a subshell; some versions of sh will print an error if
# an executable is not found, even if stderr is redirected.
# Redirect stdin to placate older versions of autoconf.  Sigh.
if ($2 --version) < /dev/null > /dev/null 2>&1; then
   $1=$2
   AC_MSG_RESULT(found)
else
   $1="$3/missing $2"
   AC_MSG_RESULT(missing)
fi
AC_SUBST($1)])

