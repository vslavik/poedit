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

remove_unchanged_po_files
remove_empty_lproj_string_files

(cd locales && ./update-translation-lists.sh)

git status
echo ""
exit_code=0
for i in locales/*.po ; do
    msgfmt -c -o /dev/null $i || exit_code=$?
done
exit $exit_code
