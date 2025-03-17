#!/bin/bash -e

POEDIT_VERSION="3.6"

[ -n "${WXRC}" ] || WXRC=wxrc

XGETTEXT_ARGS="-C -F \
              -k_ -kwxGetTranslation -kwxTRANSLATE -kwxTRANSLATE_IN_CONTEXT:1c,2 -kwxPLURAL:1,2 \
              -kwxGETTEXT_IN_CONTEXT:1c,2 -kwxGETTEXT_IN_CONTEXT_PLURAL:1c,2,3 \
              --add-comments=TRANSLATORS \
              --from-code=UTF-8 \
              --package-name=Poedit --package-version=${POEDIT_VERSION} \
              --msgid-bugs-address=help@poedit.net"


# remove changes that only touch refresh header and do nothing else:
remove_date_only_changes()
{
    if ! git ls-files $1 --error-unmatch >/dev/null 2>&1 ; then
        return
    fi

    changes=$(git diff --no-ext-diff $1 | grep '^[+-][^+-]' | grep -v '\(PO-Revision\|POT-Creation\)-Date' | wc -l)
    if [ $changes -eq 0 ] ; then
        git checkout $1
    fi
}


cd locales

find ../src -name "*.cpp" | sort | xargs xgettext ${XGETTEXT_ARGS}             -o _poedit.pot
find ../src -name "*.h"   | sort | xargs xgettext ${XGETTEXT_ARGS}          -j -o _poedit.pot
${WXRC} --gettext ../src/resources/*.xrc | xgettext ${XGETTEXT_ARGS}        -j -o _poedit.pot -

msggrep -N ../src/export_html.cpp _poedit.pot -o poedit-quicklook.pot

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
