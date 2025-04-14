#!/bin/bash -e

POEDIT_VERSION="3.6.1"

[ -n "${WXRC}" ] || WXRC=wxrc

POTFILE=locales/poedit.pot
POTFILE_QL=locales/poedit-quicklook.pot

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


find src -name "*.cpp" | sort | xargs xgettext ${XGETTEXT_ARGS}       -o $POTFILE
find src -name "*.h"   | sort | xargs xgettext ${XGETTEXT_ARGS}    -j -o $POTFILE
${WXRC} --gettext src/resources/*.xrc | xgettext ${XGETTEXT_ARGS}  -j -o $POTFILE -

msggrep -N src/export_html.cpp $POTFILE -o $POTFILE_QL

remove_date_only_changes $POTFILE
remove_date_only_changes $POTFILE_QL
