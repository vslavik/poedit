
RESCOMP = @RESCOMP@
WX_LIBS = @WX_LIBS@
RC_WX_INCLUDES = @RC_WX_INCLUDES@

if USE_WINDOWS
RESOURCE_FILES = poedit_resource.o
endif

bin_PROGRAMS = poedit

poedit_SOURCES = catalog.cpp catalog.h edapp.cpp edapp.h edframe.cpp \
                 edframe.h fileviewer.cpp fileviewer.h parser.cpp parser.h \
                 isocodes.cpp isocodes.h prefsdlg.cpp prefsdlg.h \
                 settingsdlg.cpp settingsdlg.h progressinfo.h \
                 progressinfo.cpp digger.h digger.cpp gexecute.h \
                 gexecute.cpp summarydlg.h summarydlg.cpp \
                 findframe.cpp findframe.h commentdlg.h commentdlg.cpp \
                 transmem.cpp transmem.h transmemupd.cpp transmemupd.h \
                 transmemupd_wizard.cpp transmemupd_wizard.h \
                 manager.h manager.cpp chooselang.cpp chooselang.h \
                 export_html.cpp icons.h icons.cpp \
                 pluralforms/pl_evaluate.cpp \
                 editlbox/editlbox.cpp \
                 edlistctrl.cpp edlistctrl.h digits.h \
				 compiled_xrc.cpp

poedit_LDADD = $(RESOURCE_FILES) $(WX_LIBS)

poedit_resource.o: poedit.rc
	$(RESCOMP) -i$(srcdir)/poedit.rc -o poedit_resource.o \
        --include-dir $(srcdir) $(RC_WX_INCLUDES) \
        --define __WIN32__ --define __WIN95__ --define __GNUWIN32__

XRC_RESOURCES = \
		$(srcdir)/resources/about.xrc \
		$(srcdir)/resources/menus.xrc \
		$(srcdir)/resources/prefs.xrc \
		$(srcdir)/resources/progress.xrc \
		$(srcdir)/resources/settings.xrc \
        $(srcdir)/resources/summary.xrc \
		$(srcdir)/resources/toolbar.xrc \
		$(srcdir)/resources/find.xrc \
		$(srcdir)/resources/comment.xrc \
		$(srcdir)/resources/fileview.xrc \
        $(srcdir)/resources/manager.xrc \
		$(srcdir)/resources/tm_update.xrc

XRC_MORE_FILES = \
		$(srcdir)/resources/about.html \
		$(srcdir)/resources/scanning.gif \
		$(srcdir)/resources/wxwin.gif \
		$(srcdir)/resources/sleepycat.gif \
		$(srcdir)/resources/sourceforge.gif \
        $(srcdir)/resources/tm_update.gif

compiled_xrc.cpp: $(XRC_RESOURCES) $(XRC_MORE_FILES)
	$(WXRC) -v -c -o $@ $(XRC_RESOURCES)


# Mac OS X bundle:
bundle: poedit
	rm -rf bundle
	mkdir -p poedit.app/Contents/MacOS
	mkdir -p poedit.app/Contents/Resources
	cp poedit poedit.app/Contents/MacOS/poedit
	strip poedit.app/Contents/MacOS/poedit
	`$(WX_CONFIG_WITH_ARGS) --rezflags` poedit.app/Contents/MacOS/poedit
	cp $(top_builddir)/macosx/Info.plist poedit.app/Contents
	cp $(top_srcdir)/macosx/poedit.icns poedit.app/Contents/Resources
