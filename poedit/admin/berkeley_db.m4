dnl
dnl @synopsis AX_BERKELEY_DB([MINIMUM-VERSION
dnl                          [, ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]]])
dnl
dnl This macro tries to find Berkeley DB. It honors MINIMUM-VERSION if given.
dnl 
dnl If libdb is found, DB_HEADER and DB_LIBS variables are set and 
dnl ACTION-IF-FOUND shell code is executed if specified. DB_HEADER is set to 
dnl location of db.h header in quotes (e.g. "db3/db.h") and AC_DEFINE_UNQUOTED
dnl is called on it, so that you can type 
dnl      #include DB_HEADER
dnl in your C/C++ code. DB_LIBS is set to linker flags needed to link against
dnl the library (e.g. -ldb3.1) and AC_SUBST is called on it.
dnl
dnl @version $Id$
dnl @author Vaclav Slavik <vslavik@fastmail.fm>
dnl
AC_DEFUN([AX_BERKELEY_DB],
[
  old_LIBS="$LIBS"

  minversion=ifelse([$1], ,,$1)
                
  DB_HEADER=""
  DB_LIBS=""

  if test -z $minversion ; then
      minvermajor=0
      minverminor=0
      minverpatch=0
      AC_MSG_CHECKING([for Berkeley DB])
  else
      minvermajor=`echo $minversion | cut -d. -f1`
      minverminor=`echo $minversion | cut -d. -f2`
      minverpatch=`echo $minversion | cut -d. -f3`
      minvermajor=${minvermajor:-0}
      minverminor=${minverminor:-0}
      minverpatch=${minverpatch:-0}
      AC_MSG_CHECKING([for Berkeley DB >= $minversion])
  fi

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
                        #if !((DB_VERSION_MAJOR > (${minvermajor}) || \
                              (DB_VERSION_MAJOR == (${minvermajor}) && \
                                    DB_VERSION_MINOR > (${minverminor})) || \
                              (DB_VERSION_MAJOR == (${minvermajor}) && \
                                    DB_VERSION_MINOR == (${minverminor}) && \
                                    DB_VERSION_PATCH >= (${minverpatch}))))
                            #error "too old version"
                        #endif
                    
                        DB *db;
                        db_create(&db, NULL, 0);
                    ])],
                [
                    AC_MSG_RESULT([header $db_hdr, library $db_lib])
                                
                    DB_HEADER="$db_hdr"
                    DB_LIBS="$db_lib"
                ])
        fi
    done
  done

  LIBS="$old_LIBS"

  if test -z $DB_HEADER ; then
    AC_MSG_RESULT([not found])
    ifelse([$3], , :, [$3])
  else
    AC_DEFINE_UNQUOTED(DB_HEADER, ["$DB_HEADER"])
    AC_SUBST(DB_LIBS)
    ifelse([$2], , :, [$2])
  fi
])
