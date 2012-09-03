#!/bin/sh
#
# Updates lists of translations in various places
#
# $Id$
#

LANGS_POEDIT=`ls *.po 2>/dev/null | sed -n 's,\.po,,p'`
LANGS_POEDIT=`echo $LANGS_POEDIT`

replace_str()
{
    echo ",s;$2;$3;g
w
q
" | ed -s $1 2>/dev/null
}

uniq_list()
{
    arg="$*"
    python -c "
list=\"$arg\".split()
done=[]
for i in list:
  if i not in done:
    print i
    done.append(i)
"
}


update_makefile_am()
{
    echo "updating Makefile.am..."
    replace_str Makefile.am \
            '\(POEDIT_LINGUAS = \).*' "\1$LANGS_POEDIT"
}


INFO_PLIST=../macosx/Info.plist.in

generate_info_plist()
{
    sed -n -e '1,/begin localizations list/ p' $INFO_PLIST
    for lang in $LANGS_POEDIT ; do
        echo "        <string>$lang</string>"
    done
    sed -n -e '/end localizations list/,$ p' $INFO_PLIST
}

update_info_plist()
{
    echo "updating Info.plist.in..."
    generate_info_plist >$INFO_PLIST.new
    mv -f $INFO_PLIST.new $INFO_PLIST
}


update_makefile_am
update_info_plist
