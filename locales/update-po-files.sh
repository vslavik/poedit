#!/bin/sh -e

PACKAGE_SHORT_VERSION=1.6

[ -n "${WXRC}" ] || WXRC=wxrc

XGETTEXT_ARGS="-C -k_ -kwxGetTranslation -kwxTRANSLATE -kwxPLURAL:1,2 -s \
              --add-comments=TRANSLATORS \
              --from-code=UTF-8 \
              --package-name=Poedit --package-version=${PACKAGE_SHORT_VERSION} \
              --msgid-bugs-address=poedit@googlegroups.com"

echo "extracting strings into poedit.pot..."
find ../src -name "*.cpp" | xargs xgettext ${XGETTEXT_ARGS}             -o poedit.pot
find ../src -name "*.h"   | xargs xgettext ${XGETTEXT_ARGS}          -j -o poedit.pot
${WXRC} --gettext ../src/resources/*.xrc | xgettext ${XGETTEXT_ARGS} -j -o poedit.pot -

echo "merging into *.po..."
for p in *.po ; do
    msgmerge --quiet --update --backup=none $p poedit.pot
done
