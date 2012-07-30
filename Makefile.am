
ACLOCAL_AMFLAGS = -I admin

SUBDIRS = src docs locales icons

desktopdir=$(datadir)/applications
desktop_DATA = poedit.desktop

bundle: all
	$(MAKE) $(MAKEOPTS) -C src bundle

dist-osx: bundle
	$(srcdir)/macosx/make-dmg poedit-$(VERSION).dmg src/Poedit.app $(srcdir)/macosx/dmg
