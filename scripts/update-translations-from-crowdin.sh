#!/bin/sh
set -e

# Fetch translations updates:

crowdin-cli download


# Now work around deficiencies in Crowdin's CLI tools and remove bogus files:

# Some PO files only have PO-Revision-Date difference:
remove_unchanged_po_files()
{
    for i in locales/*.po ; do
        if git diff --numstat "$i" | grep -q '^1\t1\t' ; then
            if git diff "$i" | grep -q '^+"PO-Revision-Date:' ; then
                git checkout "$i"
            fi
        fi
    done
}

# Crowdin tools create empty .strings files for all translations, even the ones
# that don't exist:
remove_empty_lproj_string_files()
{
    find macosx -type f -name '*.strings' -empty -delete
    find macosx -type d -name '*.lproj' -empty -delete
}

# Massage RC files from Crowdin to be actually usable:
fixup_windows_rc_files()
{
    # Empty RC files have "" entries instead of proper translations. Remove the files that have
    # nothing else in them:
    for i in locales/win/*-*.rc ; do
        cnt=$(grep '[0-9]\+, ".\+"' "$i" | wc -l)
        if [ $cnt -eq 0 ] ; then
            rm -f "$i"
        fi
    done

    # In the remaining files, substitute correct LANGUAGE settings:
    for i in locales/win/*-*.rc ; do
        stripped=`basename "$i" .rc | cut -d- -f2 | tr [a-z] [A-Z]`
        lang=`echo $stripped | cut -d_ -f1`
        sublang=`echo $stripped | cut -s -d_ -f2`
        if [ -n "$sublang" ] ; then
            code="LANG_${lang}, SUBLANG_${lang}_${sublang}"
        else
            code="LANG_${lang}, SUBLANG_NEUTRAL"
        fi
        sed --in-place -e "s/LANGUAGE.*/LANGUAGE $code/" "$i"
    done

    # Finally, include all files in a master file:
    master="locales/win/windows_strings.rc"
    echo '#pragma code_page(65001)'             >$master
    echo '#include <windows.h>'                >>$master
    echo '#include "windows_strings_base.rc"'  >>$master
    for i in locales/win/*-*.rc ; do
        incl=`basename "$i"`
        echo "#include \"$incl\"" >>$master
    done
}

remove_unchanged_po_files
remove_empty_lproj_string_files
fixup_windows_rc_files

(cd locales && ./update-translation-lists.sh)

git status
echo ""
exit_code=0
for i in locales/*.po ; do
    msgfmt -c -o /dev/null $i || exit_code=$?
done
exit $exit_code
