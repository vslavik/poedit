#!/bin/sh
#
# Updates lists of translations in various places
#
# $Id$
#

LANGS_POEDIT=`ls *.po 2>/dev/null | sed -n 's,\.po,,p'`
LANGS_POEDIT=`echo $LANGS_POEDIT`
LANGS_WX=`cd wxwin ; ls *.mo 2>/dev/null | sed -n 's,\.mo,,p'`
LANGS_WX=`echo $LANGS_WX`

replace_str()
{
    echo "
,s@$2@$3@g
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
    replace_str Makefile.am \
            '\(WXWIN_LINGUAS  = \).*' "\1$LANGS_WX"
}


update_poedit_iss()
{
    f_dirs=../win32/poedit-locale-dirs.iss
    f_files=../win32/poedit-locale-files.iss
   
    echo updating $f_dirs...
    rm -f $f_dirs
    for i in `uniq_list $LANGS_POEDIT $LANGS_WX` ; do
        echo "Name: {app}\\share\\locale\\$i; Components: i18n" >>$f_dirs
        echo "Name: {app}\\share\\locale\\$i\\LC_MESSAGES; Components: i18n" >>$f_dirs
    done
    
    echo updating $f_files...
    rm -f $f_files
    for i in $LANGS_POEDIT ; do
        echo "Source: locales\\$i.mo; DestDir: {app}\\share\\locale\\$i\\LC_MESSAGES; Components: i18n; DestName: poedit.mo" >>$f_files
    done
    for i in $LANGS_WX ; do
        echo "Source: locales\\wxwin\\$i.mo; DestDir: {app}\\share\\locale\\$i\\LC_MESSAGES; Components: i18n; DestName: wxstd.mo" >>$f_files
    done
}


update_makefile_am
update_poedit_iss
