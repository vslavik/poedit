
pkgdatadir=$(datadir)/poedit

pkgdata_DATA = help.zip help-gettext.zip

help.zip: basics.html index.html menu.html poedit.hhc poedit.hhp settings.html ui.html update.html
	if test $(srcdir) != . ; then \
	  cp -fR $(srcdir)/img $(srcdir)/*.html $(srcdir)/*.hhc $(srcdir)/*.hhp . \
	  ; fi
	zip -9 -r help.zip img *.html poedit.hhc poedit.hhp


help-gettext.zip:
	if test $(srcdir) != . ; then \
	  cp -fR $(srcdir)/gettext $(srcdir)/gettext.hhc $(srcdir)/gettext.hhp . \
	  ; fi
	zip -9 -r help-gettext.zip gettext gettext.hhc gettext.hhp

