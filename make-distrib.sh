#!/bin/sh

#
# Creates distribution files
# $Id$
#

VERSION=1.3.9

#(
#cd docs_classes
#rm -f *.html *gif
#doxygen
#)

(
cd locales
for i in *.po ; do msgfmt -o `basename $i .po`.mo $i ; done
)


find_unix_files()
{
  (find . -maxdepth 1 -type f ; \
  find admin build locales src docs install macosx win32 -type f) | \
    grep -v '/win32-' | \
    grep -v '/\.svn' | \
    grep -v '/\.bakefile_gen.state' | \
    grep -v '\.#' | \
    grep -v '\.\(dsp\|dsw\|chm\|rtf\|ncb\|opt\|plg\)' | \
    grep -v 'install/.*\.txt'
}

find_win32_files()
{
  echo "docs/chm/*.chm"
}


DIST_DIR=`pwd`

rm -rf /tmp/poedit-$VERSION
mkdir /tmp/poedit-$VERSION
tar -c `find_unix_files` | (cd /tmp/poedit-$VERSION ; tar -x)
(cd /tmp ; tar -c poedit-$VERSION | gzip -9 >$DIST_DIR/poedit-$VERSION.tar.gz)
rm -rf /tmp/poedit-$VERSION

mkdir /tmp/poedit-$VERSION
tar -c `find_win32_files` | (cd /tmp/poedit-$VERSION ; tar -x)
(cd /tmp ; tar -c poedit-$VERSION | gzip -9 >$DIST_DIR/poedit-$VERSION-win32-addon.tar.gz)
rm -rf /tmp/poedit-$VERSION
