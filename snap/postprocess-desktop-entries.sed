# This sed script processes desktop entries of Poedit for
# the snap packaging.  The main usage of it is to make
# desktop entries of snap-packaged Poedit distinguishable
# with others using different software distribution tech-
# nologies

## Append '(Snappy Edition)' to the application name ##
/^Name=.*$/s/$/ (Snappy Edition)/

## Fix-up application icon lookup ##
s|^Icon=.*|Icon=\${SNAP}/meta/gui/icon.png|
