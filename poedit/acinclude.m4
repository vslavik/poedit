

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

  DB_VERSION=0
  AC_SEARCH_LIBS(db_create, db db-4.5 db-4.4 db-4.3 db-4.2 db-4.1 db-4.0 db-3.6 db-3.5 db-3.4 db-3.3 db-3.2 db-3.1,
         [
             if echo $LIBS | grep -q '\-ldb-4\.' ; then
                 DB_HEADER_DIR="db4/"
             elif echo $LIBS | grep -q '\-ldb-3\.' ; then
                 DB_HEADER_DIR="db3/"
             fi
         ],
         [
             USE_TRANSMEM=0
             AC_MSG_WARN([
*** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** 
Cannot find Berkeley DB >= 3.1, poEdit will build w/o translation memory feature!
*** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** 
             ])
         ])

  if test x$USE_TRANSMEM != x0 ; then
      AC_CHECK_HEADER(${DB_HEADER_DIR}db.h,
            [DB_HEADER="${DB_HEADER_DIR}db.h"],
            [
              AC_CHECK_HEADER(db.h, [DB_HEADER=db.h],
              [
                  USE_TRANSMEM=0
                  AC_MSG_RESULT(not found)
                  AC_MSG_WARN([
*** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** 
Cannot find Berkeley DB >= 3.1, poEdit will build w/o translation memory feature!
*** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** 
                  ])
              ])
            ])

      if test x$USE_TRANSMEM != x0 ; then
          CXXFLAGS="$CXXFLAGS -DUSE_TRANSMEM -DDB_HEADER=\\\"$DB_HEADER\\\""
      fi
   fi

   if test x$USE_TRANSMEM != x0 ; then
      AC_MSG_CHECKING([if db_open requires transaction argument])
      AC_TRY_COMPILE([#include "$DB_HEADER"],
        [
            DB *db;
            DBTYPE type;
            db->open(db, "foo", NULL, type, DB_CREATE, 0);
        ],
        [
            AC_MSG_RESULT(no)
        ],
        [
            AC_MSG_RESULT(yes)
            CXXFLAGS="$CXXFLAGS -DDB_OPEN_WITH_TRANSACTION"
        ])
   fi
])

