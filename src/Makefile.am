
LUCENE_LIBS = @LUCENE_LIBS@
WX_LIBS = @WX_LIBS@

bin_PROGRAMS = poedit

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
                 keychain/keytar_posix.cc keychain/keytar.h \
                 json.h json/src/json.hpp
CROWDIN_SUPPORT_LIBS = $(CPPREST_LIBS) $(LIBSECRET_LIBS)
endif

poedit_SOURCES = \
                 attentionbar.cpp attentionbar.h \
                 cat_update.h cat_update.cpp \
                 cat_sorting.cpp cat_sorting.h \
                 catalog.cpp catalog.h \
                 chooselang.cpp chooselang.h \
                 cloud_sync.h \
                 colorscheme.h colorscheme.cpp \
                 commentdlg.h commentdlg.cpp \
                 concurrency.cpp concurrency.h \
                 configuration.cpp configuration.h \
                 custom_buttons.cpp custom_buttons.h \
                 customcontrols.cpp customcontrols.h \
                 edapp.cpp edapp.h \
                 edframe.cpp edframe.h \
                 editing_area.cpp editing_area.h \
                 edlistctrl.cpp edlistctrl.h \
                 errors.h \
                 export_html.cpp \
                 extractors/extractor.cpp extractors/extractor.h \
                 extractors/extractor_gettext.cpp \
                 extractors/extractor_legacy.cpp extractors/extractor_legacy.h \
                 fileviewer.cpp fileviewer.h \
                 findframe.cpp findframe.h \
                 gexecute.h gexecute.cpp \
                 hidpi.cpp hidpi.h \
                 icons.h icons.cpp \
                 language.cpp language.h \
                 language_impl_legacy.h language_impl_plurals.h \
                 languagectrl.cpp languagectrl.h \
                 logcapture.h \
                 main_toolbar.h wx/main_toolbar.cpp \
                 manager.h manager.cpp \
                 pluralforms/pl_evaluate.cpp pluralforms/pl_evaluate.h \
                 prefsdlg.cpp prefsdlg.h \
                 pretranslate.cpp pretranslate.h \
                 progressinfo.h progressinfo.cpp \
                 propertiesdlg.cpp propertiesdlg.h \
                 qa_checks.cpp qa_checks.h \
                 sidebar.cpp sidebar.h \
                 spellchecking.h spellchecking.cpp \
                 str_helpers.h \
                 syntaxhighlighter.cpp syntaxhighlighter.h \
                 text_control.h text_control.cpp \
                 tm/suggestions.cpp tm/suggestions.h \
                 tm/transmem.cpp tm/transmem.h \
                 unicode_helpers.h unicode_helpers.cpp \
                 utility.cpp utility.h \
                 version.h \
                 welcomescreen.cpp welcomescreen.h \
                 $(CROWDIN_SUPPORT_SRC) \
                 $(WX_BACKPORT_SRC)
nodist_poedit_SOURCES = compiled_xrc.cpp

poedit_LDADD = $(WX_LIBS) $(LUCENE_LIBS) $(CLD2_LIBS) $(CROWDIN_SUPPORT_LIBS) \
               $(BOOST_LDFLAGS) $(BOOST_THREAD_LIB) $(BOOST_REGEX_LIB) $(BOOST_SYSTEM_LIB)

XRC_RESOURCES = \
        $(srcdir)/resources/comment.xrc \
        $(srcdir)/resources/manager.xrc \
        $(srcdir)/resources/menus.xrc \
        $(srcdir)/resources/prefs.xrc \
        $(srcdir)/resources/progress.xrc \
        $(srcdir)/resources/properties.xrc \
        $(srcdir)/resources/summary.xrc \
        $(srcdir)/resources/toolbar.xrc

compiled_xrc.cpp: $(XRC_RESOURCES)
	$(WXRC) -v -c -o $@ $(XRC_RESOURCES)

DISTCLEANFILES = $(nodist_poedit_SOURCES)

EXTRA_DIST = $(XRC_RESOURCES) pluralforms/COPYING
