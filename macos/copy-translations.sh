#!/bin/bash

set -e

DESTDIR="$1"

MSGFMT="${MSGFMT-msgfmt} -c"

get_all_langs()
{
    for f in `ls locales/*.po` ; do
        basename "$f" .po
    done
}

lang_to_macos()
{
    x="$1"
    x="${x/zh_TW/zh_Hant}"
    x="${x/zh_CN/zh_Hans}"
    x="${x/@latin/_Latn}"
    echo "$x"
}

try_compile_po()
{
    lang="$1"
    shortlang=${lang:0:2}
    outfile="$2"
    podir="$3"

    if [ -f "$podir/$lang.po" ] ; then
        if [ '!' "$outfile" -nt "$podir/$lang.po" ] ; then
            echo "Compiling $podir/$lang.po"
            $MSGFMT -o "$outfile" "$podir/$lang.po"
        fi
    elif [ -f "$podir/$shortlang.po" ] ; then
        if [ '!' "$outfile" -nt "$podir/$shortlang.po" ] ; then
            echo "Compiling $podir/$shortlang.po"
            $MSGFMT -o "$outfile" "$podir/$shortlang.po"
        fi
    fi
}

for lang in `get_all_langs`; do
    lproj="$DESTDIR/`lang_to_macos $lang`.lproj"

    mkdir -p "$lproj"

    try_compile_po $lang "$lproj/poedit.mo"           locales
    try_compile_po $lang "$lproj/wxstd.mo"            deps/wx/locale
done

# macOS uses pt.lproj for pt_BR localization; bizarre as it is, we must do the
# same otherwise the localization wouldn't be used. Use a symlink, though, to
# keep things non-confusing:
ln -sfh "pt_BR.lproj" "$DESTDIR/pt.lproj"
