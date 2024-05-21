#!/bin/bash -e

if [ "$EXPANDED_CODE_SIGN_IDENTITY" != "" ]; then
    signing_flags="-v --deep $OTHER_CODE_SIGN_FLAGS"

    if [ "$ENABLE_HARDENED_RUNTIME" = YES ]; then
        signing_flags="$signing_flags --options runtime"
    fi

    echo "note: Signing $* ($signing_flags)"
    codesign --force -s "${EXPANDED_CODE_SIGN_IDENTITY}" $signing_flags $*
fi
