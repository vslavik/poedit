
GIF_FILES = fileopen.gif filesave.gif fuzzy.gif logo.gif quotes.gif scanning.gif \
            update.gif fullscreen.gif
XRC_FILES = about.xrc menus.xrc prefs.xrc progress.xrc settings.xrc summary.xrc \
            toolbar.xrc find.xrc

pkgdatadir=$(datadir)/poedit

pkgdata_DATA = resources.zip

resources.zip: $(XRC_FILES) $(GIF_FILES)
	if test $(srcdir) != . ; then \
	  cp -fR $(srcdir)/*.xrc $(srcdir)/*.gif . \
	  ; fi
	zip -9 -r resources.zip *.xrc *.gif
	if test $(srcdir) != . ; then \
	  rm -rf *.xrc *.gif \
	  ; fi

EXTRA_DIST = $(XRC_FILES) $(GIF_FILES)
