# This sed script processes desktop entries of the snapped
# application.
#
# Documentation:
#
# * GNU Sed Manual
#   https://www.gnu.org/software/sed/manual
#     * `sed` script overview - `sed` scripts
#     * `sed` commands summary - `sed` scripts
#     * The `s` Command - `sed` scripts
#     * Overview of basic regular expression syntax - Regular
#       Expressions: selecting text
#     * Back-references and Subexpressions - Regular Expressions:
#       selecting text

## Fix-up application icon lookup
s|^Icon=.*|Icon=\${SNAP}/meta/gui/icon.png|
