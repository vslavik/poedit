#!/bin/sh

set -e

DESTDIR="$1"

rm -rf "$DESTDIR"

if [ "$ARCHS" = "x86_64" ]; then
    ln -sf "$DESTDIR.x86_64" "$DESTDIR"
    exit
elif [ "$ARCHS" = "arm64" ]; then
    ln -sf "$DESTDIR.arm64" "$DESTDIR"
    exit
fi

echo "merging into universal binaries ($ARCHS)..."
cp -a "$DESTDIR.x86_64" "$DESTDIR"

(cd "$DESTDIR" ; find . -type f) | while read fn ; do
    if file "$DESTDIR/$fn" | grep -q '\(Mach-O .* executable\|Mach-O .* dynamically linked shared library\|current ar archive\)'; then
        echo "  $fn..."
        lipo -create -output "$DESTDIR/$fn" "$DESTDIR.x86_64/$fn" "$DESTDIR.arm64/$fn"
    fi
done
