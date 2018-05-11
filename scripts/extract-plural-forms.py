#!/usr/bin/env python3

# Update plural forms expressions from the data collected by Unicode Consortium
# (see http://www.unicode.org/cldr/charts/supplemental/language_plural_rules.html),
# but from a JSON version by Transifex folks

import os.path
import sys
import urllib.request
import re
import gettext
import json

TABLE_URL = "https://github.com/transifex/transifex/raw/devel/transifex/languages/fixtures/all_languages.json"

MARKER_BEGIN = "// Code generated with scripts/extract-plural-forms.py begins here"
MARKER_END   = "// Code generated with scripts/extract-plural-forms.py ends here"


def validate_entry(e):
    """Validates plural forms expression."""
    m = re.match(r'nplurals=([0-9]+); plural=(.+);', e)
    if not m:
        raise ValueError("incorrect structure of plural form expression")
    c_expr = m.group(2)
    # verify the syntax is correct
    func = gettext.c2py(c_expr)
    # and that all indexes are used:
    count = int(m.group(1))
    used = [0] * count
    for i in range(0,1001):
        fi = func(i)
        if fi >= count:
            raise ValueError("expression out of range (n=%d -> %d)" % (i, fi))
        used[func(i)] = 1
    if sum(used) != count:
            raise ValueError("not all plural form values used (n=0..100)")


if os.path.isfile("src/language_impl_plurals.h"):
    outfname = "src/language_impl_plurals.h"
elif os.path.isfile("../src/language_impl_plurals.h"):
    outfname = "../src/language_impl_plurals.h"
else:
    raise RuntimeError("run this script from root or from scripts/ directory")

with open(outfname, "rt") as f:
    orig_content = f.read()


with urllib.request.urlopen(TABLE_URL) as response:
    data = json.load(response)

output = "// Code generated with scripts/extract-plural-forms.py begins here\n\n"

langdata = {}
shortlangdata = {}

for x in data:
    data = x['fields']
    lang = data['code']
    shortlang = lang.partition('_')[0]
    expr = 'nplurals=%d; plural=%s;' % (data['nplurals'], data['pluralequation'])
    langdata[lang] = expr
    shortlangdata.setdefault(shortlang, set()).add(expr)
    if shortlang=='ar':
        print(lang, repr(expr))


for lang in list(langdata.keys()):
    short,_,country = lang.partition('_')
    if not country:
        continue
    if len(shortlangdata[short]) == 1:
        # all variants of the language are the same w.r.t. plural forms
        assert short in langdata
        del langdata[lang]

for lang in sorted(langdata.keys()):
    expr = langdata[lang]
    definition = '{ %-7s, "%s" },' % ('"%s"' % lang, expr)
    try:
        validate_entry(expr)
        output += '%s\n' % definition
    except ValueError as e:
        output += '// %s // BROKEN\n' % definition
        sys.stderr.write('skipping %s: %s\n\t"%s"\n' % (lang, e, expr))

output += "\n// Code generated with scripts/extract-plural-forms.py ends here\n"

content = re.sub('%s(.*?)%s' % (MARKER_BEGIN, MARKER_END),
                 output,
                 orig_content,
                 0,
                 re.DOTALL)

with open(outfname, "wt") as f:
    f.write(content)
