#!/bin/sh

#
# Creates distribution files
# $Id$
#

VERSION=1.2.0

#(
#cd docs_classes
#rm -f *.html *gif
#doxygen
#)

cp NEWS install/news.txt
cp README install/readme.txt
cp LICENSE install/license.txt
crlf -d install/*.txt

find_unix_files()
{
  (find . -type f -maxdepth 1 ; \
  find locales src docs extras install -type f) | \
    grep -v '/win32-' | \
    grep -v '/CVS' | \
    grep -v '\.\(dsp\|dsw\|chm\|rtf\|iss\|ico\|ncb\|opt\|plg\|rc\)' | \
    grep -v 'install/.*\.txt'
  echo 'docs_classes/Doxyfile'
}

find_win32_files()
{
  echo \
"docs/chm/*.chm
poedit.dsw
poedit.dsp"
ls -1 install/{*.txt,*.rtf,*.iss}
ls -1 src/*.{ico,rc}
ls -1 extras/win32-gettext/*.{exe,COPYING}
ls -1 extras/win32-db3/*.{h,dll}
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
