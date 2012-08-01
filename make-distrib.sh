#!/bin/sh

#
# Creates distribution files
# $Id$
#

VERSION=1.5.1

#(
#cd docs_classes
#rm -f *.html *gif
#doxygen
#)

./bootstrap


find_unix_files()
{
  (find . -maxdepth 1 -type f ; \
  find admin locales src docs macosx win32 icons -type f) | \
    grep -v '/win32-' | \
    grep -v '/\.git' | \
    grep -v '/\.bakefile_gen.state' | \
    grep -v '\.#' | \
    grep -v '\.\(dsp\|dsw\|chm\|rtf\|ncb\|opt\|plg\)'
}


DIST_DIR=`pwd`

rm -rf /tmp/poedit-$VERSION
mkdir /tmp/poedit-$VERSION
tar -c `find_unix_files` | (cd /tmp/poedit-$VERSION ; tar -x)
(cd /tmp ; tar -c poedit-$VERSION | gzip -9 >$DIST_DIR/poedit-$VERSION.tar.gz)
rm -rf /tmp/poedit-$VERSION
