


pkgdatadir=$(datadir)/poedit

pkgdata_DATA = help.zip

help.zip: basics.html index.html menu.html poedit.hhc poedit.hhp settings.html ui.html update.html
	if test $(srcdir) != . ; then \
	  cp -fR $(srcdir)/gettext $(srcdir)/img $(srcdir)/*.html $(srcdir)/*.hhc $(srcdir)/*.hhp . \
	  ; fi
	zip -9 -r help.zip gettext img *.html *.hhc *.hhp

