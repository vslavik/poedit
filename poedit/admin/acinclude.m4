

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
  USE_TRANSMEM=yes

  dnl NB: can't look for db_create function because some builds of db-4.x
  dnl     have safe function names such as db_create_4000

  old_LIBS="$LIBS"
                
  DB_HEADER=""
  DB_LIBS=""

  AC_MSG_CHECKING([for Berkeley DB])

  for version in "" 5.0 4.9 4.8 4.7 4.6 4.5 4.4 4.3 4.2 4.1 4.0 3.6 3.5 3.4 3.3 3.2 3.1 ; do

    if test -z $version ; then
        db_lib="-ldb"
        try_headers="db.h"
    else
        db_lib="-ldb-$version"
        try_headers="db$version/db.h db`echo $version | sed -e 's,\..*,,g'`/db.h"
    fi
        
    LIBS="$old_LIBS $db_lib"
 
    for db_hdr in $try_headers ; do
        if test -z $DB_HEADER ; then
            AC_LINK_IFELSE(
                [AC_LANG_PROGRAM(
                    [
                        #include <${db_hdr}>
                    ],
                    [
                        DB *db;
                        db_create(&db, NULL, 0);
                    ])],
                [
                    DB_HEADER="$db_hdr"
                    DB_LIBS="$db_lib"
                    AC_MSG_RESULT([header $DB_HEADER, library $DB_LIBS])
                ])
        fi
    done
  done

  LIBS="$old_LIBS"

  if test -z $DB_HEADER ; then
    AC_MSG_RESULT([not found])
    
    USE_TRANSMEM=no
    AC_MSG_WARN([cannot find Berkeley DB >= 3.1, poEdit will build w/o translation memory feature])
  fi
  
  if test x$USE_TRANSMEM != xno ; then
      #CXXFLAGS="$CXXFLAGS -DUSE_TRANSMEM -DDB_HEADER=\\\"$DB_HEADER\\\""
      AC_DEFINE(USE_TRANSMEM)
      AC_DEFINE_UNQUOTED(DB_HEADER, ["$DB_HEADER"])
      LIBS="$LIBS $DB_LIBS"
  fi

   if test x$USE_TRANSMEM != xno ; then
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

