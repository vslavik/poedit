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
    WX_CFLAGS=`$WX_CONFIG $wx_config_args --cxxflags`
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
  AC_PATH_PROG(KDE_CONFIG, kde-config)
  if test x$KDE_CONFIG = x ; then
      AC_MSG_CHECKING(for KDEDIR)
      if test x$KDEDIR = x ; then
          AC_MSG_RESULT(no)
          KDE_DATA_DIR=""
      else
          KDE_DATA_DIR=$KDEDIR/share
          AC_MSG_RESULT($KDE_DATA_DIR)
      fi
  else
      KDE_DATA_DIR=`kde-config --prefix`/share
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

  if test x$USE_TRANSMEM != x0 ; then
      CXXFLAGS="$CXXFLAGS -DUSE_TRANSMEM"
  fi
  LIBS="$LIBS $DB_LIB"
  CXXFLAGS="$CXXFLAGS $DB_INCLUDE"
])

