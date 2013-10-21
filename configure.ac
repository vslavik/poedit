dnl Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)

AC_INIT([poedit], [1.6beta1], [poedit@googlegroups.com])
PACKAGE_SHORT_VERSION=1.6
AC_SUBST(PACKAGE_SHORT_VERSION)

AC_CONFIG_AUX_DIR([admin])
AC_CONFIG_SRCDIR([README])

AC_CANONICAL_BUILD
AC_CANONICAL_HOST

AM_INIT_AUTOMAKE([subdir-objects])
AM_MAINTAINER_MODE

m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])

WX_CONFIG_OPTIONS

AC_ARG_ENABLE(transmem,
              AS_HELP_STRING([--enable-transmem],
                             [Enable translation memory (default: yes)]),
              USE_TRANSMEM="$enableval", USE_TRANSMEM="yes")
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
AC_LANG_CPLUSPLUS


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
        equivalent variable and wxWidgets is version 2.8.0 or above,
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


AC_CHECK_HEADER([boost/function.hpp],
    [],
    [AC_MSG_ERROR([Boost is required to compile Poedit.])])

AC_CHECK_FUNCS([mkdtemp])


if test "$USE_SPELLCHECKING" != "no" ; then
    dnl we need GtkSpell and GTK+ 2.x for this, check if we're compatible
    PKG_CHECK_MODULES([GTKSPELL], [gtkspell-2.0],
        [
            AC_MSG_CHECKING([if wxWidgets toolkit uses GTK+ 2])
            AC_TRY_COMPILE([#include <wx/defs.h>],
                [
                    #ifndef __WXGTK20__
                        #error "not GTK+ 2"
                    #endif
                ],
                [
                    AC_MSG_RESULT([yes])
                    CXXFLAGS="$CXXFLAGS $GTKSPELL_CFLAGS"
                    LIBS="$LIBS $GTKSPELL_LIBS"
                    AC_DEFINE(USE_SPELLCHECKING)
                ],
                [
                
                    AC_MSG_RESULT([no])
                    USE_SPELLCHECKING="no"
                    AC_MSG_WARN([disabling spellchecking because GTK+ 2 is required by it])
                ])
        ],
        [
            USE_SPELLCHECKING="no"
            AC_MSG_WARN([disabling spellchecking because GtkSpell is missing])
        ])
fi



if $WX_CONFIG_WITH_ARGS --cxxflags | grep -q WXMSW; then
    use_windows=yes
    
    changequote(<<,>>)
    RC_WX_INCLUDES=`echo " $WX_CPPFLAGS" | sed -e 's/ -[^I][^ ]*//g' | sed -e 's/ -I\([^ ]*\)/ --include-dir \1/g'`
    changequote([,])

    AC_CHECK_TOOL(WINDRES, windres)
else
    use_windows=no
fi

AM_CONDITIONAL(USE_WINDOWS, test $use_windows = yes)

if test x$USE_TRANSMEM != xno ; then
    AX_BERKELEY_DB_CXX(4.7,
    [
        AC_DEFINE(USE_TRANSMEM)
        AC_DEFINE_UNQUOTED(DB_HEADER, ["$DB_HEADER"])
        LIBS="$LIBS $DB_LIBS"
   ],
   [
      USE_TRANSMEM=no
      AC_MSG_WARN([cannot find Berkeley DB >= 4.7, Poedit will build w/o translation memory feature])
   ])
fi


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
AC_SUBST(WINDRES)
AC_SUBST(WX_CONFIG_WITH_ARGS)
AC_SUBST(WX_LIBS)
AC_SUBST(RC_WX_INCLUDES)

AC_CONFIG_FILES([
         Makefile
         src/Makefile
         icons/Makefile
         locales/Makefile
         docs/Makefile
         ])

AC_OUTPUT

echo "
Configured $PACKAGE-$VERSION for $host

    Debugging information:      $USE_DEBUG
    Translation memory:         $USE_TRANSMEM
    Spellchecking:              $USE_SPELLCHECKING"
echo ""
