#!/bin/sh

#
# Creates distribution files
#

VERSION=1.1.4

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
  find src docs extras install -type f) | \
    grep -v '/win32-' | \
    grep -v '/CVS' | \
    grep -v '\.\(dsp\|dsw\|chm\|rtf\|iss\|ico\|ncb\|opt\|plg\|rc\)' | \
    grep -v 'install/.*\.txt'
  echo 'docs_classes/Doxyfile'
}

find_win32_files()
{
  find src docs extras install -type f | \
    grep -v '/CVS' | \
    grep -v 'Makefile' | \
    grep -v 'appicon.xpm'
  echo \
"LICENSE
NEWS
README
poedit.dsw
poedit.dsp
docs_classes/Doxyfile"
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
(cd /tmp ; zip -9r $DIST_DIR/poedit-$VERSION-win32.zip  poedit-$VERSION)
rm -rf /tmp/poedit-$VERSION
