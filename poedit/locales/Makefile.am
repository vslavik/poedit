
ALL_LINGUAS = cs

localedir = $(datadir)/locale

install-data-local: @CATALOGS_TO_INSTALL@

install-poedit-catalogs:
	for i in $(ALL_LINGUAS) ; do \
	    $(mkinstalldirs) $(DESTDIR)$(localedir)/$$i/LC_MESSAGES ; \
	    $(INSTALL_DATA) $(srcdir)/$$i.mo $(DESTDIR)$(localedir)/$$i/LC_MESSAGES/poedit.mo ; \
	done

install-wxstd-catalogs:
	for i in $(ALL_LINGUAS) ; do \
	    $(INSTALL_DATA) $(srcdir)/$$i-wxstd.mo $(DESTDIR)$(localedir)/$$i/LC_MESSAGES/poedit-wxstd.mo ; \
	done
