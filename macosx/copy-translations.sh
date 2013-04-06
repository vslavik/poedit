#!/bin/sh

set -e

DESTDIR="$1"

MSGFMT="${MSGFMT-msgfmt} -c"

get_all_langs()
{
    for f in `ls locales/*.po` ; do
        basename "$f" .po
    done
}

lang_to_osx()
{
    echo $1 | sed -e 's/zh_TW/zh-Hant/' \
            | sed -e 's/zh_CN/zh-Hans/' \
            | sed -e 's/@latin/-Latn/' \
            | sed -e 's/_/-/'
}

try_compile_po()
{
    lang="$1"
    shortlang=`echo $lang | cut -c1-2`
    outfile="$2"
    podir="$3"

    if [ -f "$podir/$lang.po" ] ; then
        if [ '!' "$outfile" -nt "$podir/$lang.po" ] ; then
            echo "    $podir/$lang.po"
            $MSGFMT -o "$outfile" "$podir/$lang.po"
        fi
    elif [ -f "$podir/$shortlang.po" ] ; then
        if [ '!' "$outfile" -nt "$podir/$shortlang.po" ] ; then
            echo "    $podir/$shortlang.po"
            $MSGFMT -o "$outfile" "$podir/$shortlang.po"
        fi
    fi
}

for lang in `get_all_langs`; do
    lproj="$DESTDIR/`lang_to_osx $lang`.lproj"
    echo "creating $lproj ($lang)..."
    mkdir -p "$lproj"

    try_compile_po $lang "$lproj/poedit.mo"           locales
    try_compile_po $lang "$lproj/wxstd.mo"            deps/wx/locale
done
