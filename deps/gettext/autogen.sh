#!/bin/sh
# Convenience script for regenerating all autogeneratable files that are
# omitted from the version control repository. In particular, this script
# also regenerates all aclocal.m4, config.h.in, Makefile.in, configure files
# with new versions of autoconf or automake.
#
# This script requires autoconf-2.60..2.69 and automake-1.11.1..1.12 in the
# PATH.
# It also requires either
#   - the GNULIB_TOOL environment variable pointing to the gnulib-tool script
#     in a gnulib checkout, or
#   - the git program in the PATH and an internet connection.
# It also requires
#   - the bison program,
#   - the gperf program,
#   - the groff program,
#   - the makeinfo program from the texinfo package,
#   - perl.

# Copyright (C) 2003-2012 Free Software Foundation, Inc.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

# Usage: ./autogen.sh [--quick] [--skip-gnulib]
#
# Usage after a first-time git clone / cvs checkout:   ./autogen.sh
# Usage after a git clone / cvs update:                ./autogen.sh --quick
# This uses an up-to-date gnulib checkout.
# (The gettext-0.18.2 release was prepared using gnulib commit
# f022473fdaf724d84817c4003120b9a38fbf884b from 2012-12-19.)
#
# Usage from a released tarball:             ./autogen.sh --quick --skip-gnulib
# This does not use a gnulib checkout.

quick=false
skip_gnulib=false
while :; do
  case "$1" in
    --quick) quick=true; shift;;
    --skip-gnulib) skip_gnulib=true; shift;;
    *) break ;;
  esac
done

# The tests in gettext-tools/tests are not meant to be executable, because
# they have a TESTS_ENVIRONMENT that specifies the shell explicitly.

if ! $skip_gnulib; then
  if test -z "$GNULIB_TOOL"; then
    # Check out gnulib in a subdirectory 'gnulib'.
    if test -d gnulib; then
      (cd gnulib && git pull)
    else
      git clone git://git.savannah.gnu.org/gnulib.git
    fi
    # Now it should contain a gnulib-tool.
    if test -f gnulib/gnulib-tool; then
      GNULIB_TOOL=`pwd`/gnulib/gnulib-tool
    else
      echo "** warning: gnulib-tool not found" 1>&2
    fi
  fi
  # Skip the gnulib-tool step if gnulib-tool was not found.
  if test -n "$GNULIB_TOOL"; then
    # In gettext-runtime:
    GNULIB_MODULES_RUNTIME_FOR_SRC='
      atexit
      basename
      closeout
      error
      getopt-gnu
      gettext-h
      havelib
      memmove
      progname
      propername
      relocatable-prog
      setlocale
      sigpipe
      stdbool
      stdio
      stdlib
      strtoul
      unlocked-io
      xalloc
    '
    GNULIB_MODULES_RUNTIME_OTHER='
      gettext-runtime-misc
      ansi-c++-opt
      csharpcomp-script
      java
      javacomp-script
    '
    $GNULIB_TOOL --dir=gettext-runtime --lib=libgrt --source-base=gnulib-lib --m4-base=gnulib-m4 --no-libtool --local-dir=gnulib-local --local-symlink \
      --import $GNULIB_MODULES_RUNTIME_FOR_SRC $GNULIB_MODULES_RUNTIME_OTHER
    # In gettext-runtime/libasprintf:
    GNULIB_MODULES_LIBASPRINTF='
      alloca
      errno
      verify
      xsize
    '
    GNULIB_MODULES_LIBASPRINTF_OTHER='
    '
    $GNULIB_TOOL --dir=gettext-runtime/libasprintf --source-base=. --m4-base=gnulib-m4 --lgpl=2 --makefile-name=Makefile.gnulib --libtool --local-dir=gnulib-local --local-symlink \
      --import $GNULIB_MODULES_LIBASPRINTF $GNULIB_MODULES_LIBASPRINTF_OTHER
    $GNULIB_TOOL --copy-file m4/intmax_t.m4 gettext-runtime/libasprintf/gnulib-m4/intmax_t.m4
    # In gettext-tools:
    GNULIB_MODULES_TOOLS_FOR_SRC='
      alloca-opt
      atexit
      backupfile
      basename
      binary-io
      bison-i18n
      byteswap
      c-ctype
      c-strcase
      c-strcasestr
      c-strstr
      clean-temp
      closedir
      closeout
      copy-file
      csharpcomp
      csharpexec
      error
      error-progname
      execute
      fd-ostream
      file-ostream
      filename
      findprog
      fnmatch
      fopen
      fstrcmp
      full-write
      fwriteerror
      gcd
      getline
      getopt-gnu
      gettext-h
      hash
      html-styled-ostream
      iconv
      javacomp
      javaexec
      libunistring-optional
      localcharset
      locale
      localename
      lock
      memmove
      memset
      minmax
      obstack
      open
      opendir
      openmp
      ostream
      pipe-filter-ii
      progname
      propername
      readdir
      relocatable-prog
      relocatable-script
      setlocale
      sh-quote
      sigpipe
      sigprocmask
      spawn-pipe
      stdbool
      stdio
      stdlib
      stpcpy
      stpncpy
      strcspn
      strerror
      strpbrk
      strtol
      strtoul
      styled-ostream
      sys_select
      sys_stat
      sys_time
      term-styled-ostream
      unilbrk/ulc-width-linebreaks
      uniname/uniname
      unistd
      unistr/u8-mbtouc
      unistr/u8-mbtoucr
      unistr/u8-uctomb
      unistr/u16-mbtouc
      uniwidth/width
      unlocked-io
      vasprintf
      wait-process
      write
      xalloc
      xconcat-filename
      xmalloca
      xerror
      xsetenv
      xstriconv
      xstriconveh
      xvasprintf
    '
    # Common dependencies of GNULIB_MODULES_TOOLS_FOR_SRC and GNULIB_MODULES_TOOLS_FOR_LIBGREP.
    GNULIB_MODULES_TOOLS_FOR_SRC_COMMON_DEPENDENCIES='
      alloca-opt
      extensions
      gettext-h
      include_next
      locale
      localcharset
      malloc-posix
      mbrtowc
      mbsinit
      multiarch
      snippet/arg-nonnull
      snippet/c++defs
      snippet/warn-on-use
      ssize_t
      stdbool
      stddef
      stdint
      stdlib
      streq
      unistd
      verify
      wchar
      wctype-h
    '
    GNULIB_MODULES_TOOLS_OTHER='
      gettext-tools-misc
      ansi-c++-opt
      csharpcomp-script
      csharpexec-script
      gcj
      java
      javacomp-script
      javaexec-script
      stdint
    '
    GNULIB_MODULES_TOOLS_LIBUNISTRING_TESTS='
      unilbrk/u8-possible-linebreaks-tests
      unilbrk/ulc-width-linebreaks-tests
      unistr/u8-mbtouc-tests
      unistr/u8-mbtouc-unsafe-tests
      uniwidth/width-tests
    '
    $GNULIB_TOOL --dir=gettext-tools --lib=libgettextlib --source-base=gnulib-lib --m4-base=gnulib-m4 --tests-base=gnulib-tests --makefile-name=Makefile.gnulib --libtool --with-tests --local-dir=gnulib-local --local-symlink \
      --import --avoid=hash-tests `for m in $GNULIB_MODULES_TOOLS_LIBUNISTRING_TESTS; do echo --avoid=$m; done` $GNULIB_MODULES_TOOLS_FOR_SRC $GNULIB_MODULES_TOOLS_FOR_SRC_COMMON_DEPENDENCIES $GNULIB_MODULES_TOOLS_OTHER
    # In gettext-tools/libgrep:
    GNULIB_MODULES_TOOLS_FOR_LIBGREP='
      mbrlen
      regex
    '
    $GNULIB_TOOL --dir=gettext-tools --macro-prefix=grgl --lib=libgrep --source-base=libgrep --m4-base=libgrep/gnulib-m4 --witness-c-macro=IN_GETTEXT_TOOLS_LIBGREP --makefile-name=Makefile.gnulib --local-dir=gnulib-local --local-symlink \
      --import `for m in $GNULIB_MODULES_TOOLS_FOR_SRC_COMMON_DEPENDENCIES; do if test \`$GNULIB_TOOL --extract-applicability $m\` != all; then echo --avoid=$m; fi; done` $GNULIB_MODULES_TOOLS_FOR_LIBGREP
    # In gettext-tools/libgettextpo:
    # This is a subset of the GNULIB_MODULES_FOR_SRC.
    GNULIB_MODULES_LIBGETTEXTPO='
      basename
      close
      c-ctype
      c-strcase
      c-strstr
      error
      error-progname
      file-ostream
      filename
      fopen
      fstrcmp
      fwriteerror
      gcd
      getline
      gettext-h
      hash
      iconv
      libunistring-optional
      minmax
      open
      ostream
      progname
      relocatable-lib
      sigpipe
      stdbool
      stdio
      stdlib
      strerror
      unilbrk/ulc-width-linebreaks
      unistr/u8-mbtouc
      unistr/u8-mbtoucr
      unistr/u8-uctomb
      unistr/u16-mbtouc
      uniwidth/width
      unlocked-io
      vasprintf
      xalloc
      xconcat-filename
      xmalloca
      xerror
      xstriconv
      xvasprintf
    '
    GNULIB_MODULES_LIBGETTEXTPO_OTHER='
    '
    $GNULIB_TOOL --dir=gettext-tools --source-base=libgettextpo --m4-base=libgettextpo/gnulib-m4 --macro-prefix=gtpo --makefile-name=Makefile.gnulib --libtool --local-dir=gnulib-local --local-symlink \
      --import $GNULIB_MODULES_LIBGETTEXTPO $GNULIB_MODULES_LIBGETTEXTPO_OTHER
  fi
fi

# Fetch config.guess, config.sub.
if test -n "$GNULIB_TOOL"; then
  for file in config.guess config.sub; do
    $GNULIB_TOOL --copy-file build-aux/$file; chmod a+x build-aux/$file
  done
else
  for file in config.guess config.sub; do
    wget -q --timeout=5 -O build-aux/$file.tmp "http://git.savannah.gnu.org/gitweb/?p=gnulib.git;a=blob_plain;f=build-aux/${file};hb=HEAD" \
      && mv build-aux/$file.tmp build-aux/$file \
      && chmod a+x build-aux/$file
  done
fi

(cd gettext-runtime/libasprintf
 ../../build-aux/fixaclocal aclocal -I ../../m4 -I ../m4 -I gnulib-m4
 autoconf
 autoheader && touch config.h.in
 automake --add-missing --copy
)

(cd gettext-runtime
 ../build-aux/fixaclocal aclocal -I m4 -I ../m4 -I gnulib-m4
 autoconf
 autoheader && touch config.h.in
 automake --add-missing --copy
 # Rebuilding the PO files and manual pages is only rarely needed.
 if ! $quick; then
   ./configure --disable-java --disable-native-java --disable-csharp \
     && (cd po && make update-po) \
     && (cd intl && make) && (cd gnulib-lib && make) && (cd src && make) \
     && (cd man && make update-man1 all) \
     && make distclean
 fi
)

cp -p gettext-runtime/ABOUT-NLS gettext-tools/ABOUT-NLS

(cd gettext-tools/examples
 ../../build-aux/fixaclocal aclocal -I ../../gettext-runtime/m4 -I ../../m4
 autoconf
 automake --add-missing --copy
 # Rebuilding the examples PO files is only rarely needed.
 if ! $quick; then
   ./configure && (cd po && make update-po) && make distclean
 fi
)

(cd gettext-tools
 ../build-aux/fixaclocal aclocal -I m4 -I ../gettext-runtime/m4 -I ../m4 -I gnulib-m4 -I libgrep/gnulib-m4 -I libgettextpo/gnulib-m4
 autoconf
 autoheader && touch config.h.in
 test -d intl || mkdir intl
 automake --add-missing --copy
 # Rebuilding the PO files, manual pages, documentation, test files is only rarely needed.
 if ! $quick; then
   ./configure --disable-java --disable-native-java --disable-csharp --disable-openmp \
     && (cd po && make update-po) \
     && (cd intl && make) && (cd gnulib-lib && make) && (cd libgrep && make) && (cd src && make) \
     && (cd man && make update-man1 all) \
     && (cd doc && make all) \
     && (cd tests && make update-expected) \
     && make distclean
 fi
)

build-aux/fixaclocal aclocal -I m4
autoconf
automake
