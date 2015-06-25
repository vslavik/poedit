#!/bin/sh -e

PACKAGE_SHORT_VERSION=1.8.2

[ -n "${WXRC}" ] || WXRC=wxrc

XGETTEXT_ARGS="-C -k_ -kwxGetTranslation -kwxTRANSLATE -kwxPLURAL:1,2 -F \
              --add-comments=TRANSLATORS \
              --from-code=UTF-8 \
              --package-name=Poedit --package-version=${PACKAGE_SHORT_VERSION} \
              --msgid-bugs-address=help@poedit.net"


# remove changes that only touch refresh header and do nothing else:
remove_date_only_changes()
{
    changes=$(git diff $1 | grep '^[+-][^+-]' | grep -v '\(PO-Revision\|POT-Creation\)-Date' | wc -l)
    if [ $changes -eq 0 ] ; then
        git checkout $1
    fi
}


cd locales

find ../src -name "*.cpp" | xargs xgettext ${XGETTEXT_ARGS}             -o _poedit.pot
find ../src -name "*.h"   | xargs xgettext ${XGETTEXT_ARGS}          -j -o _poedit.pot
${WXRC} --gettext ../src/resources/*.xrc | xgettext ${XGETTEXT_ARGS} -j -o _poedit.pot -

# -F is incompatible with --no-location, results in weird order, so remove location info
# in a secondary pass:
msgcat --no-location -o poedit.pot _poedit.pot
rm _poedit.pot

remove_date_only_changes poedit.pot

# Merge with PO files:
if [ "x$1" != "x--no-po" ] ; then
    for p in *.po ; do
        msgmerge --quiet -o $p.tmp --no-fuzzy-matching --no-location $p poedit.pot
        msgattrib --no-obsolete -o $p $p.tmp
        rm $p.tmp
        remove_date_only_changes $p
    done
fi
