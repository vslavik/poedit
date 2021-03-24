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

PRISM_COMPONENTS_URL = 'https://github.com/PrismJS/prism/raw/master/components.json'
LANGUAGE_MAP_URL = 'https://github.com/blakeembrey/language-map/raw/master/languages.json'

# resolve ambiguities:
OVERRIDES = {
    'h'     : 'cpp',
    'inc'   : 'php',
    'cake'  : 'coffeescript',
    'es'    : 'javascript',
    'fcgi'  : 'lua',
    'cgi'   : 'perl',
    'pl'    : 'perl',
    'pro'   : 'perl',
    'ts'    : 'typescript',
    'tsx'   : 'typescript',
    'sch'   : 'scheme',
    'cs'    : 'csharp',
    'st'    : 'smalltalk',
}

# known irrelevant languages:
BLACKLIST = set([
    'glsl', 'nginx', 'apacheconf', 'matlab', 'opencl', 'puppet', 'reason', 'renpy',
    'plsql', 'sql', 'tex',
])

# ...and extensions:
BLACKLIST_EXT = set([
    'spec', 'pluginspec', 'ml',
])


MARKER_BEGIN = "// Code generated with scripts/extract-fileviewer-mappings.py begins here"
MARKER_END   = "// Code generated with scripts/extract-fileviewer-mappings.py ends here"



prism_langs = json.loads(urllib.request.urlopen(PRISM_COMPONENTS_URL).read().decode('utf-8'))['languages']
del prism_langs['meta']
language_map  = json.loads(urllib.request.urlopen(LANGUAGE_MAP_URL).read().decode('utf-8'))

prism_known = {}
for lang, data in prism_langs.items():
    prism_known[lang] = lang
    for a in data.get('alias', []):
        prism_known[a] = lang


ext_to_lang = {}
for lang, data in language_map.items():
    lang = lang.lower()
    lango = lang
    if not lang in prism_known:
        for a in data.get('aliases', []):
            if a in prism_known:
                lang = a
                break
    if lang not in prism_known:
        continue
    if lang in BLACKLIST:
        continue

    for ext in data.get('extensions', []):
        assert ext[0] == '.'
        ext = ext[1:].lower()
        if ext in BLACKLIST_EXT:
            continue
        if ext != lang:
            if ext in ext_to_lang:
                if ext in OVERRIDES:
                    ext_to_lang[ext] = OVERRIDES[ext]
                else:
                    sys.stderr.write(f'SKIPPING due to extension conflict: {ext} both {lang} and {ext_to_lang[ext]}\n')
                    ext_to_lang[ext] = lang
            else:
                ext_to_lang[ext] = lang


output = f'{MARKER_BEGIN}\n\n'

for ext in sorted(ext_to_lang.keys()):
    lang = ext_to_lang[ext]
    output += f'{{ "{ext}", "{lang}" }},\n'

output += f'\n{MARKER_END}\n'



if os.path.isfile("src/fileviewer.extensions.h"):
    outfname = "src/fileviewer.extensions.h"
else:
    raise RuntimeError("run this script from project root directory")

with open(outfname, "rt") as f:
    orig_content = f.read()

content = re.sub('%s(.*?)%s' % (MARKER_BEGIN, MARKER_END),
                 output,
                 orig_content,
                 0,
                 re.DOTALL)

with open(outfname, "wt") as f:
    f.write(content)

print(output)
sys.stderr.write(f'Generated code written to {outfname}\n')