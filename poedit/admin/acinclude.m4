

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


