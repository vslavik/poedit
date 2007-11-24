#!/bin/sh

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
VER_SHORT=`echo $VER_FULL | sed -e 's/pre[0-9]//g'`

replace_ver win32/poedit.iss \
            '\(#define VERSION *"\).*\("\)' "\1$VER_FULL\2"
replace_ver configure.in \
            '\(AC_INIT(\[poedit\], \[\)[^]]*\(\],.*\)' "\1$VER_FULL\2"
replace_ver make-distrib.sh \
            '\(VERSION=\).*' "\1$VER_FULL"
replace_ver src/edapp.cpp \
            '\(wxString version(_T("\).*\("));\)' "\1$VER_SHORT\2"

for i in locales/*.po ; do
    replace_ver $i \
                '\(Project-Id-Version:\)[^\\]*' "\1 Poedit $VER_SHORT"
done
