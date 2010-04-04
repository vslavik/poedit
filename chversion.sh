#!/bin/sh

if [ -z "$1" ] ; then
    echo "Usage: $0 \"version\"" >&2
    exit 1
fi

replace_ver()
{
    echo "replacing in $1..."
    echo "
,s@$2@$3@g
w
q
" | ed -s $1 2>/dev/null
}

VER_FULL=$1
VER_SHORT="`echo $VER_FULL | sed -e 's/\(pre\|beta\|rc\)[0-9]//g'`"
VER_WIN="`echo $VER_SHORT | tr '.' ','`,0"

replace_ver win32/poedit.iss \
            '\(#define VERSION_FULL *"\).*\("\)' "\1$VER_FULL\2"
replace_ver win32/poedit.iss \
            '\(#define VERSION *"\).*\("\)' "\1$VER_SHORT\2"
replace_ver configure.ac \
            '\(AC_INIT(\[poedit\], \[\)[^]]*\(\],.*\)' "\1$VER_FULL\2"
replace_ver make-distrib.sh \
            '\(VERSION=\).*' "\1$VER_FULL"
replace_ver src/version.h \
            '\(POEDIT_VERSION.*"\).*\("\)' "\1$VER_SHORT\2"
replace_ver src/version.h \
            '\(POEDIT_VERSION_WIN *\).*' "\1$VER_WIN"

for i in locales/*.po locales/*.pot ; do
    replace_ver $i \
                '\(Project-Id-Version:\)[^\\]*' "\1 Poedit $VER_SHORT"
done
