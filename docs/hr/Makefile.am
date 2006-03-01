
pkgdatadir=$(datadir)/poedit

pkgdata_DATA = help-hr.zip

help-hr.zip: basics.html index.html menu.html poedit.hhc poedit.hhp settings.html ui.html update.html
	if test $(srcdir) != . ; then \
	  cp -fR $(srcdir)/../en/img $(srcdir)/*.html $(srcdir)/*.hhc $(srcdir)/*.hhp . \
	  ; fi
	zip -9 -r help-hr.zip img *.html poedit.hhc poedit.hhp

