


pkgdatadir=$(datadir)/poedit

pkgdata_DATA = resources.zip

resources.zip: about.xrc fileopen.gif filesave.gif fuzzy.gif logo.gif menus.xrc prefs.xrc progress.xrc quotes.gif scanning.gif settings.xrc summary.xrc toolbar.xrc update.gif
	if test $(srcdir) != . ; then \
	  cp -fR $(srcdir)/*.xrc $(srcdir)/*.gif . \
	  ; fi
	zip -9 -r resources.zip *.xrc *.gif
	if test $(srcdir) != . ; then \
	  rm -rf *.xrc *.gif \
	  ; fi
