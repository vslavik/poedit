
SUBDIRS = src docs install

EXTRA_DIST = LICENSE poedit.dsp poedit.dsw

dist-hook:
	mkdir -p $(distdir)/docs
	cp -Rf $(srcdir)/docs/* $(distdir)/docs
	mkdir -p $(distdir)/install
	cp -Rf $(srcdir)/install/* $(distdir)/install
	mkdir -p $(distdir)/src/bitmaps
	cp -Rf $(srcdir)/src/bitmaps/* $(distdir)/src/bitmaps
