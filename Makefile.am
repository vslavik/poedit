
ACLOCAL_AMFLAGS = -I admin

SUBDIRS = src docs locales icons

desktopdir=$(datadir)/applications
desktop_DATA = poedit.desktop


dist-osx:
	$(MAKE) -C src bundle
	rm -f poedit-$(VERSION).dmg
	$(srcdir)/macosx/make-dmg poedit-$(VERSION).dmg src/Poedit.app $(srcdir)/macosx/dmg
