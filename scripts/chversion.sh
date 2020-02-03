#!/bin/sh

set -e

if [ -f /usr/local/bin/gsed ] ; then
    SED=/usr/local/bin/gsed
else
    SED=sed
fi


if [ -z "$1" ] ; then
    echo "Usage: $0 \"version\"" >&2
    exit 1
fi

if [ ! -f configure.ac ] ; then
    echo "Run this script from project root directory" >&2
    exit 2
fi

replace_ver()
{
    echo "replacing in $1..."
    $SED -e "s@$2@$3@g" --in-place $1
}

VERSION=$1
VER_WIN="`echo $VERSION | tr '.' ','`"
if [ `echo $VER_WIN | awk 'BEGIN{FS=","} {print NF}'` = 2 ] ; then
    VER_WIN="$VER_WIN,0"
fi

replace_ver win32/poedit.iss \
            '\(#define VERSION *"\).*\("\)' "\1$VERSION\2"
replace_ver win32/version.props \
            '\(<PoeditVersion>\).*\(</PoeditVersion>\)' "\1$VERSION\2"
replace_ver configure.ac \
            '\(AC_INIT(\[poedit\], \[\)[^]]*\(\],.*\)' "\1$VERSION\2"
replace_ver src/version.h \
            '\(POEDIT_VERSION *"\).*\("\)' "\1$VERSION\2"
replace_ver src/version.h \
            '\(POEDIT_VERSION_WIN *\).*' "\1$VER_WIN"
replace_ver .travis.yml \
            '\(file: poedit-\).*\(.tar.gz\)' "\1$VERSION\2"
replace_ver Poedit.xcodeproj/project.pbxproj \
            '\(POEDIT_VERSION = \).*\(;\)' "\1$VERSION\2"
replace_ver snap/snapcraft.yaml \
            '\(version: \"\).*\("\)' "\1$VERSION\2"
touch macos/Poedit-Info.plist
