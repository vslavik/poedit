#!/bin/sh
# Usage: xsmallpot.sh srcdir hello-foo [hello-foobar.pot]

set -e

# Nuisances.
(unset CDPATH) >/dev/null 2>&1 && unset CDPATH

test $# = 2 || test $# = 3 || { echo "Usage: xsmallpot.sh srcdir hello-foo [hello-foobar.pot]" 1>&2; exit 1; }
srcdir=$1
directory=$2
potfile=${3-$directory.pot}

abs_srcdir=`cd "$srcdir" && pwd`

cd ..
rm -rf tmp-$directory
cp -p -r "$abs_srcdir"/../$directory tmp-$directory
chmod -R u+w tmp-$directory
cd tmp-$directory
case $directory in
  hello-c++-kde)
    ./autogen.sh
    sed -e 's,tmp-,,' < configure.in > configure.ac
    grep '^\(AC_INIT\|AC_CONFIG\|AC_PROG_\|AC_SUBST(.*OBJC\|AM_INIT\|AM_CONDITIONAL\|AM_GNU_GETTEXT\|AM_PO_SUBDIRS\|AC_OUTPUT\)' configure.ac > configure.in
    rm -f configure.ac 
    autoconf
    ./configure
    ;;
  hello-objc-gnustep)
    ./autogen.sh
    ;;
  *)
    grep '^\(AC_INIT\|AC_CONFIG\|AC_PROG_\|AC_SUBST(.*OBJC\|AM_INIT\|AM_CONDITIONAL\|AM_GNU_GETTEXT\|AM_PO_SUBDIRS\|AC_OUTPUT\)' configure.ac > configure.in
    rm -f configure.ac 
    ./autogen.sh
    ./configure
    ;;
esac
cd po
make $potfile
sed -e "/^#:/ {
s, \\([^ ]\\), $directory/\\1,g
}" < $potfile > ../../po/$potfile
cd ..
cd ..
rm -rf tmp-$directory
