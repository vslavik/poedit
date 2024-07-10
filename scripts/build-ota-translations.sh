#!/bin/bash
set -e

DESTDIR="ota-update-$$"
rm -rf "$DESTDIR"
mkdir -p "$DESTDIR"
function finish {
    rm -rf "$DESTDIR"
}
trap finish EXIT

# check PO files for errors and remove the ones that won't compile:
for po in locales/*.po ; do
    if ! stderr=$(msgfmt -c -o /dev/null "$po" 2>&1) ; then
        echo "::error title=Failed to compile $po::`echo $stderr`"
        echo "::group::cat -n $po"
        cat -n "$po"
        echo "::endgroup::"
        rm "$po"
    fi
done

# compile PO files, taking care to make them reproducible, i.e. not change if the actual
# translations didn't change:
for po in locales/*.po ; do
    mo="$(basename "$po" .po).mo"
    mogz="$mo.gz"
    printf "%-30s" "$po"

    sed -e 's/^"PO-Revision-Date: .*/"PO-Revision-Date: \\n"/g' "$po" | msgfmt -c -o "$DESTDIR/$mo" -
    gzip --no-name -9 "$DESTDIR/$mo"

    size=$(ls -ks "$DESTDIR/$mogz" | cut -d' ' -f1)
    printf "%'3d kB  %s\n" "$size" "$(cd $DESTDIR && md5sum $mogz)"
done

# create a tarball of all updates:
tar -C "$DESTDIR" -cf ota-update.tar .

