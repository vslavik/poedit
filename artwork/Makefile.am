
iconsdir=$(datadir)/icons/hicolor
appicons16dir=$(iconsdir)/16x16/apps
appicons22dir=$(iconsdir)/22x22/apps
appicons24dir=$(iconsdir)/24x24/apps
appicons32dir=$(iconsdir)/32x32/apps
appicons48dir=$(iconsdir)/48x48/apps
appiconsscalabledir=$(iconsdir)/scalable/apps
mimeicons16dir=$(iconsdir)/16x16/mimetypes
mimeicons22dir=$(iconsdir)/22x22/mimetypes
mimeicons24dir=$(iconsdir)/24x24/mimetypes
mimeicons32dir=$(iconsdir)/32x32/mimetypes
mimeiconsscalabledir=$(iconsdir)/scalable/mimetypes
pixmapsdir=$(datadir)/pixmaps

uiiconsdir=$(datadir)/poedit/icons

dist_appicons16_DATA = linux/appicon/16x16/apps/poedit.png
dist_appicons22_DATA = linux/appicon/22x22/apps/poedit.png
dist_appicons24_DATA = linux/appicon/24x24/apps/poedit.png
dist_appicons32_DATA = linux/appicon/32x32/apps/poedit.png
dist_appicons48_DATA = linux/appicon/48x48/apps/poedit.png
dist_appiconsscalable_DATA = linux/appicon/scalable/apps/poedit.svg

dist_mimeicons16_DATA = linux/mime-gnome/16x16/mimetypes/poedit-translation-generic.png
dist_mimeicons22_DATA = linux/mime-gnome/22x22/mimetypes/poedit-translation-generic.png
dist_mimeicons24_DATA = linux/mime-gnome/24x24/mimetypes/poedit-translation-generic.png
dist_mimeicons32_DATA = linux/mime-gnome/32x32/mimetypes/poedit-translation-generic.png
dist_mimeiconsscalable_DATA = linux/mime-gnome/scalable/mimetypes/poedit-translation-generic.svg

dist_pixmaps_DATA = linux/appicon/48x48/apps/poedit.png

dist_uiicons_DATA = \
		linux/document-open.png \
		linux/document-save.png \
		linux/document-properties.png \
		linux/document-new.png \
		linux/edit-delete.png \
		linux/poedit-fuzzy.png \
		linux/poedit-update.png \
		linux/poedit-sync.png \
		linux/poedit-validate.png \
		linux/sidebar.png \
		poedit-status-automatic.png \
		poedit-status-bookmark.png \
		poedit-status-comment.png \
		poedit-status-nothing.png \
		poedit-status-cat-mid.png \
		poedit-status-cat-no.png \
		poedit-status-cat-ok.png \
		window-close.png \
		SuggestionError.png \
		SuggestionTM.png \
		PoeditWelcome.png \
		CrowdinLogo.png


MIMETYPES = \
		text-x-gettext-translation \
		gnome-mime-text-x-gettext-translation


install-data-hook:
	for type in $(MIMETYPES) ; do \
		rm -f $(DESTDIR)$(iconsdir)$$size/mimetypes/*/$$type.*; \
		$(LN_S) -f poedit-translation-generic.png $(DESTDIR)$(mimeicons16dir)/$$type.png; \
		$(LN_S) -f poedit-translation-generic.png $(DESTDIR)$(mimeicons22dir)/$$type.png; \
		$(LN_S) -f poedit-translation-generic.png $(DESTDIR)$(mimeicons24dir)/$$type.png; \
		$(LN_S) -f poedit-translation-generic.png $(DESTDIR)$(mimeicons32dir)/$$type.png; \
		$(LN_S) -f poedit-translation-generic.svg $(DESTDIR)$(mimeiconsscalabledir)/$$type.svg; \
	done
