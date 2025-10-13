#!/bin/bash -e

POEDIT_VERSION="3.8"

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

find src -name "*.cpp" | sort | xargs xgettext ${XGETTEXT_ARGS}       -o $POTFILE
find src -name "*.h"   | sort | xargs xgettext ${XGETTEXT_ARGS}    -j -o $POTFILE
${WXRC} --gettext src/resources/*.xrc | xgettext ${XGETTEXT_ARGS}  -j -o $POTFILE -

msggrep -N src/export_html.cpp $POTFILE -o $POTFILE_QL
