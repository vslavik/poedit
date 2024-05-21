#!/bin/bash

set -Eeo pipefail

SCOPE="$1"
DESTDIR="$2"

get_all_langs()
{
    for f in `ls locales/*.po` ; do
        basename "$f" .po
    done
}

lang_to_macos()
{
    x="${1/_/-}"
    x="${x/zh-TW/zh-Hant}"
    x="${x/zh-CN/zh-Hans}"
    x="${x/@latin/-Latn}"
    echo "$x"
}

try_compile_po()
{
    lang="$1"
    shortlang=${lang:0:2}
    outfile="$2"
    podir="$3"
    reference_pot="$4"

    if [ -f "$podir/$lang.po" ] ; then
        infile="$podir/$lang.po"
    elif [ -f "$podir/$shortlang.po" ] ; then
        infile="$podir/$shortlang.po"
    fi
    [ -n "$infile" ] || exit 1

    echo "compiling $podir/$lang.po"

    if [ -n "$reference_pot" ] ; then
        msgmerge -o- "$infile" "$reference_pot" | msgfmt -o "$outfile" -
    else
        msgfmt -o "$outfile" "$infile"
    fi
}

mkdir -p "$DESTDIR"

for lang in `get_all_langs`; do
    lproj="$DESTDIR/`lang_to_macos $lang`.lproj"

    mkdir -p "$lproj"

    if [ $SCOPE = "app" ]; then
        try_compile_po $lang "$lproj/poedit.mo" locales
        try_compile_po $lang "$lproj/wxstd.mo"  deps/wx/locale
    else
        try_compile_po $lang "$lproj/poedit-$SCOPE.mo" locales "locales/poedit-$SCOPE.pot"
    fi
done

# macOS uses pt.lproj for pt-BR localization; bizarre as it is, we must do the
# same otherwise the localization wouldn't be used. Use a symlink, though, to
# keep things non-confusing:
ln -sfh "pt-BR.lproj" "$DESTDIR/pt.lproj"
