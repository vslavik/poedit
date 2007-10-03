
SUBDIRS = src docs install locales

ACLOCAL_AMFLAGS = -I admin


dist-osx:
	$(MAKE) -C src bundle
	rm -f poedit-$(VERSION).dmg
	hdiutil create -srcfolder src/Poedit.app poedit-$(VERSION).dmg
# can't do this, because Safari may misinterpret file's content and
# rename it to .dmg.bz2, see here:
# http://unsanity.org/archives/mac_os_x/my_dmg_is_bwoken_after_download.php
#hdiutil convert -format UDBZ -o poedit-$(VERSION).dmg poedit-base.dmg
