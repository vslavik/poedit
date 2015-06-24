#!/bin/sh
set -e

# Fetch translations updates:

crowdin-cli download


# Now work around deficiencies in Crowdin's CLI tools and remove bogus files:

# Remove platform-specific language files where the platform doesn't know that
# language:
remove_unsupported_languages()
{
    rm -f locales/win/windows_strings-Aragonese.rc
    rm -rf */osx/an.lproj

    rm -rf */osx/fy_NL.lproj
    rm -rf */osx/oc.lproj

    rm -f locales/win/windows_strings-Esperanto.rc
}

# Crowdin tools create empty .strings files for all translations, even the ones
# that don't exist:
remove_empty_lproj_string_files()
{
    for dir in locales/osx src ; do
        find $dir -type f -name '*.strings' -empty -delete
        find $dir -type d -name '*.lproj' -empty -delete
    done
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

remove_unsupported_languages

remove_empty_lproj_string_files
fixup_windows_rc_files

scripts/refresh-pot.sh
scripts/do-update-translations-lists.sh

git status
echo ""
exit_code=0
for i in locales/*.po ; do
    msgfmt -c -o /dev/null $i || exit_code=$?
done
exit $exit_code
