dnl Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)

AC_INIT([poedit], [1.6.11], [poedit@googlegroups.com])
PACKAGE_SHORT_VERSION=1.6.11
AC_SUBST(PACKAGE_SHORT_VERSION)

AC_CONFIG_AUX_DIR([admin])
AC_CONFIG_SRCDIR([README])

AC_CANONICAL_BUILD
AC_CANONICAL_HOST

AM_INIT_AUTOMAKE([subdir-objects foreign])
AM_MAINTAINER_MODE

m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])

WX_CONFIG_OPTIONS

AC_ARG_ENABLE(spellchecking,
              AS_HELP_STRING([--enable-spellchecking],
                             [Enable spell checking (default: yes)]),
              USE_SPELLCHECKING="$enableval", USE_SPELLCHECKING="yes")
AC_ARG_ENABLE(debug,
              AS_HELP_STRING([--enable-debug],
                             [Enable debugging information]),
              USE_DEBUG="$enableval", USE_DEBUG="no")

AC_MSG_CHECKING(for install location)
case "$prefix" in
NONE)	AC_CACHE_VAL(m_cv_prefix,m_cv_prefix=$ac_default_prefix);;
*)	m_cv_prefix=$prefix ;;
esac
AC_MSG_RESULT($m_cv_prefix)
case "$m_cv_prefix" in /*) ;; *)
	AC_MSG_WARN([--prefix=$prefix must be an absolute path name, using $ac_default_prefix])
	m_cv_prefix=$ac_default_prefix
esac
prefix=$m_cv_prefix


dnl Checks for programs.
AC_PROG_AWK
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_RANLIB
AC_PROG_CC
AC_PROG_CXX
AC_PROG_CXXCPP

AC_LANG([C++])
AX_CXX_COMPILE_STDCXX_11([], [mandatory])


case "$USE_DEBUG" in
yes)	DEBUG_FLAGS="-g -Wall -O0"
    ;;    
esac

WX_CONFIG_CHECK([3.0.0],
                [WXFOUND=1], [WXFOUND=0],
                [xrc,stc,html,xml,adv,core], [--unicode])

if test "$WXFOUND" != 1; then
    AC_MSG_ERROR([
        Please check that wx-config is in path, the directory
        where wxWidgets libraries are installed (returned by
        'wx-config --unicode --libs' command) is in LD_LIBRARY_PATH or
        equivalent variable and wxWidgets is version 3.0.0 or above,
        with Unicode build available.
    ])
fi

dnl Check if wxWidgets includes XRC library and if it does, don't build it
dnl ourselves:
AC_MSG_CHECKING([if wxWidgets includes XRC])
saved_CXXFLAGS="$CXXFLAGS"
CXXFLAGS="$CXXFLAGS $WX_CXXFLAGS"
AC_TRY_COMPILE([#include <wx/defs.h>],
               [
                #if !defined(wxUSE_XRC) || !wxUSE_XRC
                    #error "XRC not compiled in"
                #endif
               ],
               [
                AC_MSG_RESULT([yes])
               ],
               [
                AC_MSG_RESULT([no])
                AC_MSG_ERROR([XRC is required to build poedit!])
               ])
CXXFLAGS="$saved_CXXFLAGS"

CXXFLAGS="$CXXFLAGS $WX_CXXFLAGS"


WXRC_CHECK([], [AC_MSG_ERROR([wxrc is needed to compile Poedit.])])


AC_CHECK_FUNCS([mkdtemp])


PKG_CHECK_MODULES([ICU], [icu-uc icu-i18n],
        [
            CXXFLAGS="$CXXFLAGS $ICU_CFLAGS"
            LIBS="$LIBS $ICU_LIBS"
        ],
        [
            AC_MSG_ERROR([missing ICU library])
        ])


if test "$USE_SPELLCHECKING" != "no" ; then
    dnl we need GtkSpell and GTK+ >= 2 for this, check if we're compatible
    AC_MSG_CHECKING([if wxWidgets toolkit uses GTK+ 3])
    AC_TRY_COMPILE([#include <wx/defs.h>],
        [
            #ifndef __WXGTK3__
                #error "not GTK+ 3"
            #endif
        ],
        [
            AC_MSG_RESULT([yes])
            gtkspell_packages="gtkspell3-3.0 gtk+-3.0"
        ],
        [
            AC_MSG_RESULT([no])
            AC_MSG_CHECKING([if wxWidgets toolkit uses GTK+ 2])
            AC_TRY_COMPILE([#include <wx/defs.h>],
                [
                    #ifndef __WXGTK20__
                        #error "not GTK+ 2"
                    #endif
                ],
                [
                    AC_MSG_RESULT([yes])
                    gtkspell_packages="gtkspell-2.0 gtk+-2.0"
                ],
                [
                    AC_MSG_RESULT([no])
                    USE_SPELLCHECKING="no"
                    AC_MSG_WARN([disabling spellchecking because GTK+ is required by it])
                ])
        ])

    if test "$USE_SPELLCHECKING" != "no" ; then
        PKG_CHECK_MODULES([GTKSPELL], [$gtkspell_packages],
            [
                CXXFLAGS="$CXXFLAGS $GTKSPELL_CFLAGS"
                LIBS="$LIBS $GTKSPELL_LIBS"
                AC_DEFINE(USE_SPELLCHECKING)
            ],
            [
                USE_SPELLCHECKING="no"
                AC_MSG_WARN([disabling spellchecking because GtkSpell is missing])
            ])
    fi
fi


AX_BERKELEY_DB_CXX(4.7,
[
    AC_DEFINE_UNQUOTED(DB_HEADER, ["$DB_HEADER"])
    AC_SUBST(DB_LIBS)
],
[
  AC_MSG_ERROR([cannot find required Berkeley DB >= 4.7])
])

PKG_CHECK_MODULES([LUCENE], [liblucene++ >= 3.0.5],
        [
            CXXFLAGS="$CXXFLAGS $LUCENE_CFLAGS"
            AC_SUBST(LUCENE_LIBS)
        ],
        [
            AC_MSG_ERROR([missing Lucene++ library])
        ])

PKG_CHECK_MODULES([EXPAT], [expat],
        [
            CXXFLAGS="$CXXFLAGS $EXPAT_CFLAGS"
            AC_SUBST(EXPAT_LIBS)
        ],
        [
            AC_MSG_ERROR([missing Expat library])
        ])


APPCAST_SUFFIX=""
if echo "$VERSION" | grep -q '\(beta\|rc\)' ; then
    APPCAST_SUFFIX="/beta"
fi
AC_SUBST(APPCAST_SUFFIX)


if test "x$GXX" == "xyes"; then
    AX_CXX_CHECK_FLAG([-Wall],[],[], [CXXFLAGS="$CXXFLAGS -Wall"])
    AX_CXX_CHECK_FLAG([-Wextra],[],[], [CXXFLAGS="$CXXFLAGS -Wextra"])
fi

CXXFLAGS="$CXXFLAGS $DEBUG_FLAGS \"-DPOEDIT_PREFIX=\\\"$prefix\\\"\""



AC_SUBST(LDFLAGS)
AC_SUBST(CFLAGS)
AC_SUBST(CXXFLAGS)
AC_SUBST(WX_CONFIG_WITH_ARGS)
AC_SUBST(WX_LIBS)

AC_CONFIG_FILES([
         Makefile
         src/Makefile
         icons/Makefile
         artwork/Makefile
         locales/Makefile
         docs/Makefile
         ])

AC_OUTPUT

echo "
Configured $PACKAGE-$VERSION for $host

    Debugging information:      $USE_DEBUG
    Spellchecking:              $USE_SPELLCHECKING"
echo ""
