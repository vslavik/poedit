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
    echo "
,s;$2;$3;g
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
    echo updating Makefile.am...
    replace_str Makefile.am \
            '\(POEDIT_LINGUAS = \).*' "\1$LANGS_POEDIT"
}


update_makefile_am
