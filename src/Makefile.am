
DB_LIBS = @DB_LIBS@
LUCENE_LIBS = @LUCENE_LIBS@
WX_LIBS = @WX_LIBS@

bin_PROGRAMS = poedit

if MIGRATE_LEGACY_TM
libexec_PROGRAMS = poedit-dump-legacy-tm
poedit_dump_legacy_tm_SOURCES = tm/dump_legacy_tm.cpp
poedit_dump_legacy_tm_LDADD = $(DB_LIBS)
endif

WX_BACKPORT_SRC = wx_backports/wx_gtk_activityindicator.cpp \
                  wx_backports/activityindicator.h \
                  wx_backports/gtk_activityindicator.h \
                  wx_backports/nativewin.h \
                  wx_backports/wx_gtk_nativewin.cpp

if HAVE_CPPREST
CROWDIN_SUPPORT_SRC = \
                 http_client.h http_client.cpp http_client_casablanca.cpp \
                 crowdin_client.h crowdin_client.cpp \
                 crowdin_gui.h crowdin_gui.cpp \
                 keychain/keytar_posix.cc keychain/keytar.h
CROWDIN_SUPPORT_LIBS = $(CPPREST_LIBS) $(LIBSECRET_LIBS)
endif

poedit_SOURCES = attentionbar.cpp attentionbar.h \
                 errorbar.cpp errorbar.h \
                 catalog.cpp catalog.h edapp.cpp edapp.h edframe.cpp \
                 edframe.h fileviewer.cpp fileviewer.h extractor.cpp extractor.h \
                 prefsdlg.cpp prefsdlg.h \
                 propertiesdlg.cpp propertiesdlg.h progressinfo.h \
                 progressinfo.cpp digger.h digger.cpp gexecute.h \
                 gexecute.cpp summarydlg.h summarydlg.cpp \
                 spellchecking.h spellchecking.cpp \
                 findframe.cpp findframe.h commentdlg.h commentdlg.cpp \
                 tm/suggestions.cpp tm/suggestions.h tm/transmem.cpp tm/transmem.h tm/tm_migrate.cpp \
                 manager.h manager.cpp chooselang.cpp chooselang.h \
                 export_html.cpp icons.h icons.cpp \
                 pluralforms/pl_evaluate.cpp pluralforms/pl_evaluate.h \
                 edlistctrl.cpp edlistctrl.h \
                 cat_sorting.cpp cat_sorting.h \
                 utility.cpp utility.h ThreadPool.h concurrency.cpp concurrency.h \
                 version.h errors.h \
                 str_helpers.h logcapture.h \
                 language.cpp language.h language_impl_legacy.h language_impl_plurals.h \
                 languagectrl.cpp languagectrl.h \
                 welcomescreen.cpp welcomescreen.h \
                 syntaxhighlighter.cpp syntaxhighlighter.h \
                 sidebar.cpp sidebar.h \
                 text_control.h text_control.cpp \
                 customcontrols.cpp customcontrols.h \
                 hidpi.cpp hidpi.h \
                 main_toolbar.h wx/main_toolbar.cpp \
                 $(CROWDIN_SUPPORT_SRC) \
                 $(WX_BACKPORT_SRC)
nodist_poedit_SOURCES = compiled_xrc.cpp

poedit_LDADD = $(WX_LIBS) $(LUCENE_LIBS) $(EXPAT_LIBS) $(CLD2_LIBS) $(CROWDIN_SUPPORT_LIBS) \
               $(BOOST_LDFLAGS) $(BOOST_REGEX_LIB) $(BOOST_SYSTEM_LIB)

XRC_RESOURCES = \
		$(srcdir)/resources/menus.xrc \
		$(srcdir)/resources/prefs.xrc \
		$(srcdir)/resources/progress.xrc \
		$(srcdir)/resources/properties.xrc \
        $(srcdir)/resources/summary.xrc \
		$(srcdir)/resources/toolbar.xrc \
		$(srcdir)/resources/comment.xrc \
        $(srcdir)/resources/manager.xrc

compiled_xrc.cpp: $(XRC_RESOURCES)
	$(WXRC) -v -c -o $@ $(XRC_RESOURCES)

DISTCLEANFILES = $(nodist_poedit_SOURCES)

EXTRA_DIST = $(XRC_RESOURCES) pluralforms/COPYING
