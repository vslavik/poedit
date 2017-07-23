dnl Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)

AC_INIT([poedit], [2.0.3], [help@poedit.net])
PACKAGE_SHORT_VERSION=2.0.3
AC_SUBST(PACKAGE_SHORT_VERSION)

AC_CONFIG_AUX_DIR([admin])
AC_CONFIG_SRCDIR([poedit.desktop])

AC_CANONICAL_BUILD
AC_CANONICAL_HOST

AM_INIT_AUTOMAKE([subdir-objects foreign])
AM_MAINTAINER_MODE

m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])

WX_CONFIG_OPTIONS

AC_ARG_ENABLE(debug,
              AS_HELP_STRING([--enable-debug],
                             [Enable debug build]),
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
AX_CXX_COMPILE_STDCXX([14], [], [mandatory])

WXLIBS_USED="xrc,stc,html,xml,adv,core,net"

case "$USE_DEBUG" in
yes)	DEBUG_FLAGS="-g -Wall -O0"
    ;;    
esac


AX_BOOST_BASE([1.60], [], [AC_MSG_ERROR([Boost libraries are required])])
AX_BOOST_SYSTEM
AX_BOOST_REGEX
AX_BOOST_THREAD
CXXFLAGS="$CXXFLAGS $BOOST_CPPFLAGS"


dnl Check for C++REST SDK used for online features
AC_ARG_WITH([cpprest],
    AS_HELP_STRING([--without-cpprest], [Ignore presence of C++ REST SDK and disable it]))

AS_IF([test "x$with_cpprest" != "xno"],
      [
      AX_BOOST_IOSTREAMS

      have_cpprest=no
      dnl C++11 check above modified CXXFLAGS, but AC_CHECK_HEADERS needs
      dnl it for this header too and it uses only the preprocessor in one
      dnl of its two phases:
      old_CPPFLAGS="$CPPFLAGS"
      CPPFLAGS="$CPPFLAGS $CXXFLAGS $BOOST_CPPFLAGS"
          AC_CHECK_HEADERS([cpprest/http_client.h],
              [
                  AC_MSG_CHECKING([for libcpprest >= 2.5])
                  old_LIBS="$LIBS"
                  LIBS="-lcpprest $BOOST_SYSTEM_LIB $BOOST_THREAD_LIB -lssl -lcrypto $LIBS"
                  AC_LINK_IFELSE([AC_LANG_PROGRAM(
                      [
                          #include <cpprest/version.h>
                          #include <cpprest/http_client.h>
                      ],
                      [
                          #if CPPREST_VERSION < 200500
                            #error "cpprest >= 2.5 required"
                          #endif
                          web::http::client::http_client c(U("https://poedit.net"));
                      ])],
                      [have_cpprest=yes])
                  LIBS="$old_LIBS"
                  AC_MSG_RESULT([$have_cpprest])
              ])
      CPPFLAGS="$old_CPPFLAGS"
      ],
      [have_cpprest=no])

AS_IF([test "x$have_cpprest" = "xyes"],
      [
          AC_DEFINE([HAVE_HTTP_CLIENT])
          AC_DEFINE([HAVE_PPL])
          CPPREST_LIBS="-lcpprest $BOOST_IOSTREAMS_LIB $BOOST_THREAD_LIB $BOOST_SYSTEM_LIB -lssl -lcrypto"
          AC_SUBST(CPPREST_LIBS)
          PKG_CHECK_MODULES([LIBSECRET], [libsecret-1], [
              CXXFLAGS="$CXXFLAGS $LIBSECRET_CFLAGS"
              AC_SUBST(LIBSECRET_LIBS)
          ])
      ],
      [
          AS_IF([test "x$with_cpprest" = "xyes"],
                [AC_MSG_ERROR([C++ REST SDK requested but not found])])
      ])
AM_CONDITIONAL([HAVE_CPPREST], [test "x$have_cpprest" != "xno"])

AC_CHECK_HEADERS([json.hpp])

WX_CONFIG_CHECK([3.0.3], [WXFOUND=1], [WXFOUND=0], [$WXLIBS_USED], [--unicode])

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
                gtkspell_packages="gtkspell-2.0 gtk+-2.0 >= 2.20"
            ],
            [
                AC_MSG_RESULT([no])
                AC_MSG_ERROR([GTK+ build of wxWidgets is required])
            ])
    ])

PKG_CHECK_MODULES([GTKSPELL], [$gtkspell_packages],
    [
        CXXFLAGS="$CXXFLAGS $GTKSPELL_CFLAGS"
        LIBS="$LIBS $GTKSPELL_LIBS"
    ],
    [
        AC_MSG_ERROR([missing GtkSpell library])
    ])


PKG_CHECK_MODULES([LUCENE], [liblucene++ >= 3.0.5],
        [
            CXXFLAGS="$CXXFLAGS $LUCENE_CFLAGS"
            AC_SUBST(LUCENE_LIBS)
        ],
        [
            AC_MSG_ERROR([missing Lucene++ library])
        ])


dnl Check for Compact Language Detector 2
dnl (used for better language detection and for non-English source languages)
AC_ARG_WITH([cld2],
    AS_HELP_STRING([--without-cld2], [Ignore presence of cld2 and disable it]))

AS_IF([test "x$with_cld2" != "xno"],
      [
          have_cld2=no
          AC_CHECK_HEADERS([cld2/public/compact_lang_det.h],
              [
                  AC_MSG_CHECKING([for libcld2])
                  old_LIBS="$LIBS"
                  LIBS="-lcld2 $LIBS"
                  AC_LINK_IFELSE([AC_LANG_PROGRAM(
                      [
                          #include <stdio.h>
                          #include <cld2/public/compact_lang_det.h>
                      ],
                      [
                          CLD2::isDataDynamic();
                      ])],
                      [have_cld2=yes])
                  LIBS="$old_LIBS"
                  AC_MSG_RESULT([$have_cld2])
              ])
      ],
      [have_cld2=no])

AS_IF([test "x$have_cld2" = "xyes"],
      [
          AC_DEFINE([HAVE_CLD2])
          CLD2_LIBS="-lcld2"
          AC_SUBST(CLD2_LIBS)
      ],
      [
          AS_IF([test "x$with_cld2" = "xyes"],
                [AC_MSG_ERROR([cld2 requested but not found])])
      ])




if test "x$GXX" == "xyes"; then
    AX_CXX_CHECK_FLAG([-Wall],[],[], [CXXFLAGS="$CXXFLAGS -Wall"])
    AX_CXX_CHECK_FLAG([-Wextra],[],[], [CXXFLAGS="$CXXFLAGS -Wextra"])
fi

CXXFLAGS="$CXXFLAGS $DEBUG_FLAGS -DwxNO_UNSAFE_WXSTRING_CONV=1 \"-DPOEDIT_PREFIX=\\\"$prefix\\\"\""



AC_SUBST(LDFLAGS)
AC_SUBST(CFLAGS)
AC_SUBST(CXXFLAGS)
AC_SUBST(WX_CONFIG_WITH_ARGS)
AC_SUBST(WX_LIBS)

AC_CONFIG_FILES([
         Makefile
         src/Makefile
         artwork/Makefile
         locales/Makefile
         docs/Makefile
         ])

AC_OUTPUT

echo "
Configured $PACKAGE-$VERSION for $host

Enabled features:

    * debug build:                    $USE_DEBUG
    * language detection:             $have_cld2
    * crowdin integration:            $have_cpprest
"

if test "x$have_cld2" != "xyes" -o "x$have_cpprest" != "xyes" ; then
    echo "

    !!! WARNING !!!

    Your are building a limited version of Poedit without some important
    features (see above). This makes Poedit harder to use and is strongly
    advised against.

    !!! WARNING !!!

    "
fi

