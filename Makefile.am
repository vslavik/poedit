
SUBDIRS = src docs install locales

ACLOCAL_AMFLAGS = -I admin


dist-osx:
	$(MAKE) -C src bundle
	rm -f poedit-base.dmg
	hdiutil create -srcfolder src/poedit.app poedit-base.dmg
	rm -f poedit-$(VERSION).dmg
	hdiutil convert -format UDBZ -o poedit-$(VERSION).dmg poedit-base.dmg
	rm -f poedit-base.dmg
