
GIF_FILES = fileopen.gif filesave.gif fuzzy.gif quotes.gif scanning.gif \
            update.gif fullscreen.gif comment.gif edit.gif delete.gif new.gif \
            help.gif wxwin.gif sleepycat.gif sourceforge.gif about.html \
            tm_update.gif
XRC_FILES = about.xrc menus.xrc prefs.xrc progress.xrc settings.xrc \
            summary.xrc toolbar.xrc find.xrc comment.xrc fileview.xrc \
            manager.xrc tm_update.xrc

pkgdatadir=$(datadir)/poedit

pkgdata_DATA = resources.zip

resources.zip: $(XRC_FILES) $(GIF_FILES)
	if test $(srcdir) != . ; then \
	  cp -fR $(srcdir)/*.xrc $(srcdir)/*.gif $(srcdir)/*.html . \
	  ; fi
	$(ZIP) -9 -r resources.zip *.xrc *.gif *.html
	if test $(srcdir) != . ; then rm -rf *.xrc *.gif *.html ; fi
