


pkgdatadir=$(datadir)/poedit

pkgdata_DATA = poedit_help.htb

poedit_help.htb: basics.html index.html menu.html poedit.hhc poedit.hhp settings.html ui.html update.html
	if test $(srcdir) != . ; then \
	  cp -fR $(srcdir)/gettext $(srcdir)/img $(srcdir)/*.html $(srcdir)/*.hhc $(srcdir)/*.hhp . \
	  ; fi
	zip -9 -r poedit_help.htb gettext img *.html *.hhc *.hhp


dist-hook:
	mkdir -p $(distdir)/img
	mkdir -p $(distdir)/gettext
	cp -p $(srcdir)/img/*.gif $(distdir)/img
	cp -p $(srcdir)/gettext/*.html $(distdir)/gettext
	cp -p $(srcdir)/*.html $(srcdir)/poedit.hhp $(srcdir)/poedit.hhc \
          $(srcdir)/technote.txt $(distdir)
