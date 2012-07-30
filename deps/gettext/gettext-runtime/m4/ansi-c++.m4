# ansi-c++.m4 serial 1 (gettext-0.12)
dnl Copyright (C) 2002-2003 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

dnl From Bruno Haible.

# Sets CXX to the name of a sufficiently ANSI C++ compliant compiler,
# or to ":" if none is found.

AC_DEFUN([gt_PROG_ANSI_CXX],
[
AC_CHECK_PROGS(CXX, $CCC g++ c++ gpp aCC CC cxx cc++ cl FCC KCC RCC xlC_r xlC, :)
if test "$CXX" != ":"; then
  dnl Use a modified version of AC_PROG_CXX_WORKS that does not exit
  dnl upon failure.
  AC_MSG_CHECKING([whether the C++ compiler ($CXX $CXXFLAGS $LDFLAGS) works])
  AC_LANG_PUSH(C++)
  AC_ARG_VAR([CXX], [C++ compiler command])
  AC_ARG_VAR([CXXFLAGS], [C++ compiler flags])
  echo 'int main () { return 0; }' > conftest.$ac_ext
  if AC_TRY_EVAL([ac_link]) && test -s conftest$ac_exeext; then
    ac_cv_prog_cxx_works=yes
    if (./conftest; exit) 2>/dev/null; then
      ac_cv_prog_cxx_cross=no
    else
      ac_cv_prog_cxx_cross=yes
    fi
  else
    ac_cv_prog_cxx_works=no
  fi
  rm -fr conftest*
  AC_LANG_POP(C++)
  AC_MSG_RESULT($ac_cv_prog_cxx_works)
  if test $ac_cv_prog_cxx_works = no; then
    CXX=:
  else
    dnl Test for namespaces. Both libasprintf and tests/lang-c++ need it.
    dnl We don't bother supporting pre-ANSI-C++ compilers.
    AC_MSG_CHECKING([whether the C++ compiler supports namespaces])
    AC_LANG_PUSH(C++)
    cat <<EOF > conftest.$ac_ext
#include <iostream>
namespace test { using namespace std; }
std::ostream* ptr;
int main () { return 0; }
EOF
    if AC_TRY_EVAL([ac_link]) && test -s conftest$ac_exeext; then
      gt_cv_prog_cxx_namespaces=yes
    else
      gt_cv_prog_cxx_namespaces=no
    fi
    rm -fr conftest*
    AC_LANG_POP(C++)
    AC_MSG_RESULT($gt_cv_prog_cxx_namespaces)
    if test $gt_cv_prog_cxx_namespaces = no; then
      CXX=:
    fi
  fi
fi
])
