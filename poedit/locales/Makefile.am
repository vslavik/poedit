
ALL_LINGUAS = cs zh zh_TW.Big5 nl

localedir = $(datadir)/locale

install-data-local: @CATALOGS_TO_INSTALL@

install-poedit-catalogs:
	for i in $(ALL_LINGUAS) ; do \
	  if [ -f $(srcdir)/$$i.mo ] ; then  \
	    $(mkinstalldirs) $(DESTDIR)$(localedir)/$$i/LC_MESSAGES ; \
	    $(INSTALL_DATA) $(srcdir)/$$i.mo $(DESTDIR)$(localedir)/$$i/LC_MESSAGES/poedit.mo ; \
	    fi ; \
	done

install-wxstd-catalogs:
	for i in $(ALL_LINGUAS) ; do \
	  if [ -f $(srcdir)/$$i-wxstd.mo ] ; then  \
	    $(mkinstalldirs) $(DESTDIR)$(localedir)/$$i/LC_MESSAGES ; \
	    $(INSTALL_DATA) $(srcdir)/$$i-wxstd.mo $(DESTDIR)$(localedir)/$$i/LC_MESSAGES/poedit-wxstd.mo ; \
	    fi ; \
	done
