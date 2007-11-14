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

VER=$1

replace_ver win32/poedit.iss \
            '\(#define VERSION *"\).*\("\)' "\1$VER\2"
replace_ver configure.in \
            '\(AC_INIT(\[poedit\], \[\)[^]]*\(\],.*\)' "\1$VER\2"
replace_ver make-distrib.sh \
            '\(VERSION=\).*' "\1$VER"
replace_ver src/edapp.cpp \
            '\(wxString version(_T("\).*\("));\)' "\1$VER\2"
