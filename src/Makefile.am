
DB_LIBS = @DB_LIBS@
LUCENE_LIBS = @LUCENE_LIBS@
WINDRES = @WINDRES@
WX_LIBS = @WX_LIBS@
RC_WX_INCLUDES = @RC_WX_INCLUDES@

bin_PROGRAMS = poedit
libexec_PROGRAMS = poedit-dump-legacy-tm

poedit_SOURCES = attentionbar.cpp attentionbar.h \
                 errorbar.cpp errorbar.h \
                 catalog.cpp catalog.h edapp.cpp edapp.h edframe.cpp \
                 edframe.h fileviewer.cpp fileviewer.h parser.cpp parser.h \
                 prefsdlg.cpp prefsdlg.h \
                 propertiesdlg.cpp propertiesdlg.h progressinfo.h \
                 progressinfo.cpp digger.h digger.cpp gexecute.h \
                 gexecute.cpp summarydlg.h summarydlg.cpp \
                 findframe.cpp findframe.h commentdlg.h commentdlg.cpp \
                 tm/transmem.cpp tm/transmem.h tm/tm_migrate.cpp \
                 manager.h manager.cpp chooselang.cpp chooselang.h \
                 export_html.cpp icons.h icons.cpp \
                 pluralforms/pl_evaluate.cpp \
                 edlistctrl.cpp edlistctrl.h digits.h \
                 cat_sorting.cpp cat_sorting.h \
                 utility.cpp utility.h \
                 version.h errors.h \
                 language.cpp language.h \
                 languagectrl.cpp languagectrl.h \
                 welcomescreen.cpp welcomescreen.h \
                 compiled_xrc.cpp

poedit_LDADD = $(RESOURCE_FILES) $(WX_LIBS) $(LUCENE_LIBS) $(EXPAT_LIBS) \
			   -lboost_regex -lboost_system

poedit_dump_legacy_tm_SOURCES = tm/dump_legacy_tm.cpp
poedit_dump_legacy_tm_LDADD = $(DB_LIBS)

XRC_RESOURCES = \
		$(srcdir)/resources/menus.xrc \
		$(srcdir)/resources/prefs.xrc \
		$(srcdir)/resources/progress.xrc \
		$(srcdir)/resources/properties.xrc \
        $(srcdir)/resources/summary.xrc \
		$(srcdir)/resources/toolbar.xrc \
		$(srcdir)/resources/find.xrc \
		$(srcdir)/resources/comment.xrc \
        $(srcdir)/resources/manager.xrc

compiled_xrc.cpp: $(XRC_RESOURCES)
	$(WXRC) -v -c -o $@ $(XRC_RESOURCES)
