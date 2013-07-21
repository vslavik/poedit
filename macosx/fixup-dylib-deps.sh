#!/bin/sh

#
# Fixes runtime linker references to not be absolute, but relative (e.g. @rpath
# or @executable_path).
#
# Usage: fixup-dylib-deps.sh <old_prefix> <new_prefix> <dylibs_dir> <binaries>
#
# Both the binaries and dylibs in <dylibs_dir> are modified
#
# For example, Gettext binaries are build with //lib prefix by build/deps.xml,
# see:
#   $ otool -L deps/bin-osx/Debug/gettext/bin/xgettext
#   deps/bin-osx/Debug/gettext/bin/xgettext:
#   	/System/Library/Frameworks/CoreFoundation.framework/Versions/A/CoreFoundation (compatibility version 150.0.0, current version 744.18.0)
#   	//lib/libgettextsrc-0.18.3.dylib (compatibility version 0.0.0, current version 0.0.0)
#   	//lib/libgettextlib-0.18.3.dylib (compatibility version 0.0.0, current version 0.0.0)
#   	/usr/lib/libxml2.2.dylib (compatibility version 10.0.0, current version 10.8.0)
#   	/usr/lib/libncurses.5.4.dylib (compatibility version 5.4.0, current version 5.4.0)
#   	//lib/libintl.8.dylib (compatibility version 10.0.0, current version 10.2.0)
#   	/usr/lib/libSystem.B.dylib (compatibility version 1.0.0, current version 169.3.0)
#   	/usr/lib/libexpat.1.dylib (compatibility version 7.0.0, current version 7.2.0)
#   	/usr/lib/libiconv.2.dylib (compatibility version 7.0.0, current version 7.0.0)
#
# To fix this, run this script as follow:
#
#   fixup-dylib-deps.sh "//lib" "@executable_path/../lib" \
#                       deps/bin-osx/Debug/gettext/lib \
#                       deps/bin-osx/Debug/gettext/bin/*
#
# Resulting in:
#
#   $ otool -L deps/bin-osx/Debug/gettext/bin/xgettext
#   deps/bin-osx/Debug/gettext/bin/xgettext:
#   	/System/Library/Frameworks/CoreFoundation.framework/Versions/A/CoreFoundation (compatibility version 150.0.0, current version 744.18.0)
#   	@executable_path/../lib/libgettextsrc-0.18.3.dylib (compatibility version 0.0.0, current version 0.0.0)
#   	@executable_path/../libgettextlib-0.18.3.dylib (compatibility version 0.0.0, current version 0.0.0)
#   	/usr/lib/libxml2.2.dylib (compatibility version 10.0.0, current version 10.8.0)
#   	/usr/lib/libncurses.5.4.dylib (compatibility version 5.4.0, current version 5.4.0)
#   	@executable_path/../libintl.8.dylib (compatibility version 10.0.0, current version 10.2.0)
#   	/usr/lib/libSystem.B.dylib (compatibility version 1.0.0, current version 169.3.0)
#   	/usr/lib/libexpat.1.dylib (compatibility version 7.0.0, current version 7.2.0)
#   	/usr/lib/libiconv.2.dylib (compatibility version 7.0.0, current version 7.0.0)

set -e

OLD_PREFIX="$1"
NEW_PREFIX="$2"
DYLIBS_DIR="$3"

shift 3

binaries=$*
dylibs="$DYLIBS_DIR"/*.dylib

for binary in $binaries $dylibs ; do
    echo "fixing dylib references in $binary..."
    for dylib in $dylibs ; do
        libname=`basename $dylib`
        install_name_tool -change "$OLD_PREFIX/$libname" "$NEW_PREFIX/$libname" $binary
    done
done
