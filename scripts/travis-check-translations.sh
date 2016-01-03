#!/bin/sh
set -e

msgfmt --version

# Validate all translations:

for i in locales/*.po ; do
    msgfmt -c -o /dev/null $i
done
