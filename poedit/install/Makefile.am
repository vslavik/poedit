

install-data-local: install-gnome install-kde

if USE_KDE
install-kde:
	mkdir -p $(KDE_DATA_DIR)/icons
	mkdir -p $(KDE_DATA_DIR)/mimelnk/application
	mkdir -p $(KDE_DATA_DIR)/applnk/Development
	if test -w $(KDE_DATA_DIR)/icons ; then \
	  install -m 644 $(top_srcdir)/src/appicon.xpm $(KDE_DATA_DIR)/icons/poedit.xpm; \
	fi
	if test -w $(KDE_DATA_DIR)/mimelnk/application ; then \
	  install -m 644 $(srcdir)/x-po.kdelnk $(KDE_DATA_DIR)/mimelnk/application; \
	fi
	if test -w $(KDE_DATA_DIR)/applnk/Development ; then \
	  install -m 644 $(srcdir)/poedit.kdelnk $(KDE_DATA_DIR)/applnk/Development; \
	fi

else
install-kde:
	

endif


if USE_GNOME
install-gnome:
	mkdir -p $(GNOME_DATA_DIR)/pixmaps
	mkdir -p $(GNOME_DATA_DIR)/gnome/apps/Development
	mkdir -p $(GNOME_DATA_DIR)/mime-info
	if test -w $(GNOME_DATA_DIR)/pixmaps ; then \
	  install -m 644 $(top_srcdir)/src/appicon.xpm $(GNOME_DATA_DIR)/pixmaps/poedit.xpm; \
	fi
	if test -w $(GNOME_DATA_DIR)/gnome/apps/Development ; then \
	  install -m 644 $(srcdir)/poedit.desktop $(GNOME_DATA_DIR)/gnome/apps/Development; \
	fi
	if test -w $(GNOME_DATA_DIR)/mime-info ; then \
	  install -m 644 $(srcdir)/poedit.mime $(GNOME_DATA_DIR)/mime-info; \
	  install -m 644 poedit.keys $(GNOME_DATA_DIR)/mime-info; \
	fi


else
install-gnome:
	

endif

