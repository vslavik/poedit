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
import subprocess
from tempfile import TemporaryDirectory
import xml.etree.ElementTree as ET


TABLE_URL = "https://www.unicode.org/repos/cldr/tags/latest/common/supplemental/plurals.xml"

MARKER_BEGIN = "// Code generated with scripts/extract-plural-forms.py begins here"
MARKER_END   = "// Code generated with scripts/extract-plural-forms.py ends here"

cldr_plurals = '/usr/local/opt/gettext/lib/gettext/cldr-plurals'


def validate_entry(lang, e):
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
            print(f"warning: {lang}: not all plural form values used (n=0..1001)")


if os.path.isfile("src/language_impl_plurals.h"):
    outfname = "src/language_impl_plurals.h"
elif os.path.isfile("../src/language_impl_plurals.h"):
    outfname = "../src/language_impl_plurals.h"
else:
    raise RuntimeError("run this script from root or from scripts/ directory")

with open(outfname, "rt") as f:
    orig_content = f.read()

langdata = {}
with TemporaryDirectory() as tmpdir:
    tmpfile = tmpdir + '/plurals.xml'
    with urllib.request.urlopen(TABLE_URL) as response, open(tmpfile, 'wb') as f:
        f.write(response.read())
    xml = ET.parse(tmpfile)
    for n in xml.findall('./plurals/pluralRules'):
        for lang in n.get('locales').split():
            expr = subprocess.run([cldr_plurals, lang, tmpfile], stdout=subprocess.PIPE).stdout.decode('utf-8').strip()
            langdata[lang] = expr

output = "// Code generated with scripts/extract-plural-forms.py begins here\n\n"

for lang in sorted(langdata.keys()):
    expr = langdata[lang]
    definition = '{ %-7s, "%s" },' % ('"%s"' % lang, expr)
    try:
        validate_entry(lang, expr)
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
