

install-data-local: install-gnome install-kde

if USE_KDE
install-kde:
	mkdir -p $(KDE_DATA_DIR)/icons
	install -m 644 $(top_srcdir)/src/bitmaps/appicon.xpm $(KDE_DATA_DIR)/icons/poedit.xpm
	mkdir -p $(KDE_DATA_DIR)/mimelnk/application
	install -m 644 $(srcdir)/x-po.kdelnk $(KDE_DATA_DIR)/mimelnk/application
	mkdir -p $(KDE_DATA_DIR)/applnk/Development
	install -m 644 $(srcdir)/poedit.kdelnk $(KDE_DATA_DIR)/applnk/Development


else
install-kde:
	

endif


if USE_GNOME
install-gnome:
	mkdir -p $(GNOME_DATA_DIR)/pixmaps
	install -m 644 $(top_srcdir)/src/bitmaps/appicon.xpm $(GNOME_DATA_DIR)/pixmaps/poedit.xpm
	mkdir -p $(GNOME_DATA_DIR)/gnome/apps/Development
	install -m 644 $(srcdir)/poedit.desktop $(GNOME_DATA_DIR)/gnome/apps/Development
	mkdir -p $(GNOME_DATA_DIR)/mime-info
	install -m 644 $(srcdir)/poedit.mime $(GNOME_DATA_DIR)/mime-info
	install -m 644 poedit.keys $(GNOME_DATA_DIR)/mime-info


else
install-gnome:
	

endif
