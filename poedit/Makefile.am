
SUBDIRS = extras src docs install

EXTRA_DIST = LICENSE poedit.dsp poedit.dsw

dist-hook:
	mkdir -p $(distdir)/extras
	(cd $(srcdir)/extras ; tar -c .) | (cd $(distdir)/extras ; tar -x)
	(cd $(distdir)/extras ; rm -rf `find -name CVS`)
