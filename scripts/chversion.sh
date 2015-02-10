#!/bin/sh

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
    sed -e "s@$2@$3@g" -i $1
}

VER_FULL=$1
VER_SHORT="`echo $VER_FULL | sed -e 's/\(pre\|beta\|rc\)[0-9]//g'`"
VER_WIN="`echo $VER_SHORT | tr '.' ','`,0"

replace_ver win32/poedit.iss \
            '\(#define VERSION_FULL *"\).*\("\)' "\1$VER_FULL\2"
replace_ver win32/poedit.iss \
            '\(#define VERSION *"\).*\("\)' "\1$VER_SHORT\2"
replace_ver win32/distrib.proj \
            '\(<PoeditVersion>\).*\(</PoeditVersion>\)' "\1$VER_SHORT\2"
replace_ver configure.ac \
            '\(AC_INIT(\[poedit\], \[\)[^]]*\(\],.*\)' "\1$VER_FULL\2"
replace_ver configure.ac \
            '\(PACKAGE_SHORT_VERSION=\).*' "\1$VER_SHORT"
replace_ver scripts/refresh-pot.sh \
            '\(PACKAGE_SHORT_VERSION=\).*' "\1$VER_SHORT"
replace_ver src/version.h \
            '\(POEDIT_VERSION.*"\).*\("\)' "\1$VER_FULL\2"
replace_ver src/version.h \
            '\(POEDIT_VERSION_WIN *\).*' "\1$VER_WIN"
replace_ver .travis.yml \
            '\(file: poedit-\).*\(.tar.gz\)' "\1$VER_FULL\2"
replace_ver Poedit.xcodeproj/project.pbxproj \
            '\(POEDIT_VERSION = \).*\(;\)' "\1$VER_FULL\2"
touch macosx/Poedit-Info.plist
