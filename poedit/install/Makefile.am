

install-data-local: install-gnome install-kde

if USE_KDE
install-kde:
	mkdir -p $(DESTDIR)$(KDE_DATA_DIR)/icons
	mkdir -p $(DESTDIR)$(KDE_DATA_DIR)/icons/mini
	mkdir -p $(DESTDIR)$(KDE_DATA_DIR)/icons/large
	mkdir -p $(DESTDIR)$(KDE_DATA_DIR)/mimelnk/application
	mkdir -p $(DESTDIR)$(KDE_DATA_DIR)/applnk/Development
	if test -w $(DESTDIR)$(KDE_DATA_DIR)/icons ; then \
	  install -m 644 $(top_srcdir)/src/icons/poedit-16.png $(DESTDIR)$(KDE_DATA_DIR)/icons/mini/poedit.png; \
	  install -m 644 $(top_srcdir)/src/icons/poedit-48.png $(DESTDIR)$(KDE_DATA_DIR)/icons/large/poedit.png; \
	  install -m 644 $(top_srcdir)/src/icons/poedit-32.png $(DESTDIR)$(KDE_DATA_DIR)/icons/poedit.png; \
	fi
	if test -w $(DESTDIR)$(KDE_DATA_DIR)/mimelnk/application ; then \
	  install -m 644 $(srcdir)/x-po.desktop $(DESTDIR)$(KDE_DATA_DIR)/mimelnk/application; \
	fi
	if test -w $(DESTDIR)$(KDE_DATA_DIR)/applnk/Development ; then \
	  install -m 644 $(srcdir)/poedit.desktop $(DESTDIR)$(KDE_DATA_DIR)/applnk/Development; \
	fi

else
install-kde:
	

endif


if USE_GNOME
install-gnome:
	mkdir -p $(DESTDIR)$(GNOME_DATA_DIR)/pixmaps
	mkdir -p $(DESTDIR)$(GNOME_DATA_DIR)/gnome/apps/Development
	mkdir -p $(DESTDIR)$(GNOME_DATA_DIR)/mime-info
	if test -w $(DESTDIR)$(GNOME_DATA_DIR)/pixmaps ; then \
	  install -m 644 $(top_srcdir)/src/icons/poedit.xpm $(DESTDIR)$(GNOME_DATA_DIR)/pixmaps/poedit.xpm; \
	  install -m 644 $(top_srcdir)/src/icons/poedit-32.png $(DESTDIR)$(GNOME_DATA_DIR)/pixmaps/poedit.png; \
	fi
	if test -w $(DESTDIR)$(GNOME_DATA_DIR)/gnome/apps/Development ; then \
	  install -m 644 $(srcdir)/poedit.desktop $(DESTDIR)$(GNOME_DATA_DIR)/gnome/apps/Development; \
	fi
	if test -w $(DESTDIR)$(GNOME_DATA_DIR)/mime-info ; then \
	  install -m 644 $(srcdir)/poedit.mime $(DESTDIR)$(GNOME_DATA_DIR)/mime-info; \
	  install -m 644 poedit.keys $(DESTDIR)$(GNOME_DATA_DIR)/mime-info; \
	fi


else
install-gnome:
	

endif

