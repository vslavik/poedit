#!/bin/bash
set -e

if [ -f /opt/homebrew/bin/gsed ] ; then
    SED=/opt/homebrew/bin/gsed
elif [ -f /usr/local/bin/gsed ] ; then
    SED=/usr/local/bin/gsed
else
    SED=sed
fi


# Fetch translations updates:

crowdin download


# Now work around deficiencies in Crowdin's CLI tools and remove bogus files:

# Remove platform-specific language files where the platform doesn't know that
# language:
remove_unsupported_languages()
{
    rm -f locales/win/windows_strings-Aragonese.rc
    rm -rf */macos/an.lproj

    rm -rf */macos/fy_NL.lproj
    rm -rf */macos/oc.lproj

    rm -f locales/win/windows_strings-Esperanto.rc
    rm -f locales/win/windows_strings-Kabyle.rc  # Win10 supports, but no LCID

    # No LTR support on macOS yet:
    rm -rf */macos/ar.lproj
    rm -rf */macos/he.lproj
}

# Crowdin tools create empty .strings files for all translations, even the ones
# that don't exist:
remove_empty_lproj_string_files()
{
    for dir in locales/macos src ; do
        find $dir -type f -name '*.strings' -empty -delete
        find $dir -type d -name '*.lproj' -empty -delete
    done
}

# Create sr-Latn files from sr-Cyrl ones:
build_sr_latin()
{
    recode-sr-latin <locales/sr.po >locales/sr@latin.po
    recode-sr-latin <locales/win/windows_strings-Serbian_Cyrillic.rc >locales/win/windows_strings-Serbian_Latin.rc
    recode-sr-latin <locales/macos/sr.lproj/InfoPlist.strings >locales/macos/sr-Latn.lproj/InfoPlist.strings
    recode-sr-latin <locales/macos/sr.lproj/MoveApplication.strings >locales/macos/sr-Latn.lproj/MoveApplication.strings
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
        lang="${lang/KAZAKH/KAZAK}"
        sublang=`echo $stripped | cut -s -d_ -f2`
        if [ "$lang" = "BOSNIAN" ] ; then
            sublang="BOSNIA_HERZEGOVINA_LATIN"
        fi
        if [ "$lang" = "CROATIAN" ] ; then
            sublang="CROATIA"
        fi
        if [ -n "$sublang" ] ; then
            code="LANG_${lang}, SUBLANG_${lang}_${sublang}"
        else
            code="LANG_${lang}, SUBLANG_NEUTRAL"
        fi
        $SED --in-place -e "s/LANGUAGE.*/LANGUAGE $code/" "$i"
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


# Remove some issues with Crowdin-generated files
fixup_po_files()
{
    for p in locales/*.po ; do
        msgmerge --quiet -o $p.tmp --no-fuzzy-matching $p locales/poedit.pot
        msgattrib --no-obsolete -o $p $p.tmp
        rm $p.tmp
        remove_date_only_changes $p
    done
}


remove_unsupported_languages

remove_empty_lproj_string_files
build_sr_latin
fixup_windows_rc_files

scripts/refresh-pot.sh
fixup_po_files

scripts/do-update-translations-lists.sh

git status
echo ""

uv run --script scripts/check-translations.py

