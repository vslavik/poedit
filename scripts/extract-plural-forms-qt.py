#!/usr/bin/env python3
# Update plural forms expressions used by Qt, from lupdate tool

import os.path
import re
import subprocess
import sys

MARKER_BEGIN = "// Code generated with scripts/extract-plural-forms-qt.py begins here"
MARKER_END   = "// Code generated with scripts/extract-plural-forms-qt.py ends here"


def extract_qt_plural_forms():
    """Extract plural forms from Qt's lupdate tool."""
    try:
        # Run lupdate -list-languages
        result = subprocess.run(['lupdate', '-list-languages'],
                              stdout=subprocess.PIPE,
                              stderr=subprocess.PIPE,
                              text=True)

        if result.returncode != 0:
            print(f"Error running lupdate: {result.stderr}", file=sys.stderr)
            return {}

        # pre-filled hard-coded corrections for lupdate output wrongness:
        plural_forms = {
            'en': 'nplurals=2; plural=(n != 1);',
            'pt-BR': 'nplurals=2; plural=(n > 1);',
            'pt': 'nplurals=2; plural=(n > 1);',
            'pt-PT': 'nplurals=2; plural=(n != 1);',
            'fil': 'nplurals=2; plural=(n > 1);',
        }

        # Parse each line of output
        for line in result.stdout.strip().split('\n'):
            if not line.strip():
                continue

            # Pattern to match: "Language Name [Country]    lang_CODE    nplurals=N; plural=expression;"
            match = re.search(r'\s+([a-z]{2,3}(?:_[A-Z]{2})?)\s+(nplurals=\d+;\s*plural=.+?;)\s*$', line)

            if match:
                locale = match.group(1).replace('_', '-')    # full language code (e.g., 'cs_CZ' or 'cs')
                lang = locale.split('-')[0]                  # basic language code (e.g., 'cs')
                plural_expr = match.group(2)

                # Filter out Qt crap:
                if lang == 'en' and lang != 'en-US':
                    continue
                if lang in ['pt', 'fil']:
                    continue

                if lang in plural_forms and plural_forms[lang] != plural_expr:
                    print(f"Warning: Duplicate plural forms found for {lang}: '{plural_forms[lang]}' vs '{plural_expr}'.", file=sys.stderr)

                plural_forms[lang] = plural_expr

        return plural_forms

    except FileNotFoundError:
        print("Error: lupdate command not found. Make sure Qt development tools are installed.", file=sys.stderr)
        return {}
    except Exception as e:
        print(f"Error: {e}", file=sys.stderr)
        return {}


def main():
    plural_forms = extract_qt_plural_forms()

    if not plural_forms:
        print("No plural forms extracted.", file=sys.stderr)
        return 1

    if os.path.isfile("src/catalog_qt_plurals.h"):
        outfname = "src/catalog_qt_plurals.h"
    elif os.path.isfile("../src/catalog_qt_plurals.h"):
        outfname = "../src/catalog_qt_plurals.h"
    else:
        raise RuntimeError("run this script from root or from scripts/ directory")

    with open(outfname, 'rt') as f:
        orig_content = f.read()

    output = f'{MARKER_BEGIN}\n\n'
    for lang in sorted(plural_forms.keys()):
        expr = plural_forms[lang]
        output += f'    {{ "{lang}", "{expr}" }},\n'
    output += f'\n{MARKER_END}\n'

    content = re.sub('%s(.*?)%s' % (MARKER_BEGIN, MARKER_END),
                    output,
                    orig_content,
                    count=0,
                    flags=re.DOTALL)

    with open(outfname, 'wt') as f:
        f.write(content)

    return 0


if __name__ == "__main__":
    sys.exit(main())
