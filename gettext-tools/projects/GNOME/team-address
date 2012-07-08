#!/bin/sh
# Print the team's address (to stdout) and output additional instructions
# (to stderr).
projectsdir="$1"
progdir="$2"
catalog="$3"  # e.g. "pt_BR"
language="$4" # e.g. "pt"

url=`cat "$projectsdir/GNOME/teams.url"`
html=`"$progdir/urlget" "$url" "$projectsdir/GNOME/teams.html"`
# The HTML page doesn't contain the languages with ISO 639 codes.
# So we have to translate ourselves...
case "$catalog" in
  af) english=Afrikaans;;
  sq) english=Albanian;;
  am) english=Amharic;;
  ar) english=Arabic;;
  hy) english=Armenian;;
  as) english=Assamese;;
  en_AU) english="Australian English";;
  az) english="Azerbaijani";;
  eu) english=Basque;;
  be) english=Belarusian;;
  bn) english=Bengali;;
  bs) english=Bosnian;;
  pt_BR) english="Brazilian Portug";; # someday they'll fix the mistake
  br) english=Breton;;
  bg) english=Bulgarian;;
  my) english=Burmese;;
  en_GB) english="British English";;
  en_CA) english="Canadian English";;
  ca) english=Catalan;;
  zh_CN) english="Chinese (simplified)";;
  zh_TW) english="Chinese (traditional)";;
  hr) english=Croatian;;
  cs) english=Czech;;
  da) english=Danish;;
  nl) english=Dutch;;
  dz) english=Dzongkha;;
  eo) english=Esperanto;;
  et) english=Estonian;;
  fj) english=Fijian;;
  fi) english=Finnish;;
  fr) english=French;;
  gl) english=Galician;;
  ka) english=Georgian;;
  de) english=German;;
  el) english=Greek;;
  gu) english=Gujarati;;
  ha) english=Hausa;;
  he) english=Hebrew;;
  hi) english=Hindi;;
  hu) english=Hungarian;;
  jab) english=Hyam;;
  is) english=Icelandic;;
  ig) english=Igbo;;
  id) english=Indonesian;;
  az_IR) english="Iranian Azerbaijani";;
  ga) english=Irish;;
  it) english=Italian;;
  ja) english=Japanese;;
  kaj) english=Jju;;
  kdm) english=Kagoma;;
  kn) english=Kannada;;
  kk) english=Kazakh;;
  km) english=Khmer;;
  rw) english=Kinyarwanda;;
  ky) english=Kirghiz;;
  ko) english=Korean;;
  ku) english=Kurdish;;
  lo) english=Lao;;
  la) english=Latin;;
  lv) english=Latvian;;
  li) english=Limburgish;;
  lt) english=Lithuanian;;
  nds) english="Low Saxon";;
  mk) english=Macedonian;;
  mai) english=Maithili;;
  ms) english=Malay;;
  ml) english=Malayalam;;
  mt) english=Maltese;;
  mi) english=Maori;;
  mr) english=Marathi;;
  myn) english=Mayan;;
  mn) english=Mongolian;;
  nah) english=Nahuatl;;
  ne) english=Nepali;;
  nso) english="Northern Sotho";;
  no) english=Norwegian;;
  oc) english=Occitan;;
  ang) english="Old English";;
  or) english=Oriya;;
  pbb) english="Páez";;
  fa) english=Persian;;
  pl) english=Polish;;
  pt) english=Portuguese;;
  pa) english=Punjabi;;
  ps) english=Pushto;;
  qu) english=Quechua;;
  ro) english=Romanian;;
  ru) english=Russian;;
  sa) english=Sanskrit;;
  sr) english=Serbian;;
  si) english=Sinhala;;
  sk) english=Slovak;;
  sl) english=Slovenian;;
  es) english=Spanish;;
  sw) english=Swahili;;
  sv) english=Swedish;;
  tl) english=Tagalog;;
  ta) english=Tamil;;
  te) english=Telugu;;
  tet) english=Tetum;;
  th) english=Thai;;
  bo) english=Tibetan;;
  ti) english=Tigrinya;;
  tr) english=Turkish;;
  tk) english=Turkmen;;
  kcg) english=Tyap;;
  ug) english=Uighur;;
  uk) english=Ukrainian;;
  ur) english=Urdu;;
  uz) english=Uzbek;;
  vi) english=Vietnamese;;
  fr_BE) english=Walloon;;
  cy) english=Welsh;;
  xh) english=Xhosa;;
  yi) english=Yiddish;;
  yo) english=Yoruba;;
  zap) english=Zapotec;;
  zu) english=Zulu;;
  *) english=;;
esac
if test -n "$english"; then
  (echo "Please consider joining your translation team, and visit"
   echo "  $url"
   echo "  http://developer.gnome.org/projects/gtp/"
  ) 1>&2
  sed_addnl='s,</tr>,</tr>\
,g'
  address=`echo "$html" | tr '\012' '|' | sed -e "$sed_addnl" | sed -n -e "s,^.*<td.*$english.*</td>.*<td.*</td>.*<td.*<a href=\"\\([^\"]*\\)\">.*</td>.*\$,\\1,p"`
  # address can be empty or contain 1 or more space separated URLs.
else
  (echo "A translation team for your "`if test "$catalog" = "$language"; then echo "language ($language)"; else echo "local dialect ($catalog)"; fi`
   echo "may not exist yet.  Please visit"
   echo "  $url"
   echo "  http://developer.gnome.org/projects/gtp/"
   echo "and decide whether you want to create a new translation team."
  ) 1>&2
  address=
fi
exit 0
