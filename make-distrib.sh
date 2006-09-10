#!/bin/sh

#
# Creates distribution files
# $Id$
#

VERSION=1.3.5

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
  find admin locales src docs extras install macosx -type f) | \
    grep -v '/win32-' | \
    grep -v '/\.svn' | \
    grep -v '\.#' | \
    grep -v '\.\(dsp\|dsw\|chm\|rtf\|iss\|ico\|ncb\|opt\|plg\|rc\)' | \
    grep -v 'install/.*\.txt'
}

find_win32_files()
{
  echo \
"docs/chm/*.chm
src/icons/appicon/poedit.ico"
ls -1 install/*.iss
ls -1 src/*.rc
ls -1 extras/win32-gettext/*.{exe,dll,COPYING}
}


DIST_DIR=`pwd`/distrib

rm -f $DIST_DIR/poedit-$VERSION*

rm -rf /tmp/poedit-$VERSION
mkdir /tmp/poedit-$VERSION
tar -c `find_unix_files` | (cd /tmp/poedit-$VERSION ; tar -x)
(cd /tmp ; tar -c poedit-$VERSION | gzip -9 >$DIST_DIR/poedit-$VERSION.tar.gz)
(cd /tmp ; tar -c poedit-$VERSION | bzip2 -9 >$DIST_DIR/poedit-$VERSION.tar.bz2)
rm -rf /tmp/poedit-$VERSION

mkdir /tmp/poedit-$VERSION
tar -c `find_win32_files` | (cd /tmp/poedit-$VERSION ; tar -x)
(cd /tmp ; tar -c poedit-$VERSION | gzip -9 >$DIST_DIR/poedit-$VERSION-win32-addon.tar.gz)
(cd /tmp ; tar -c poedit-$VERSION | bzip2 -9 >$DIST_DIR/poedit-$VERSION-win32-addon.tar.bz2)
rm -rf /tmp/poedit-$VERSION
