
LUCENE_LIBS = @LUCENE_LIBS@
WX_LIBS = @WX_LIBS@

bin_PROGRAMS = poedit

if HAVE_CPPREST
ACCOUNTS_SUPPORT_SRC = \
                 http_client.h http_client.cpp http_client_cpprestsdk.cpp \
                 cloud_accounts.h cloud_accounts.cpp cloud_accounts_ui.h cloud_accounts_ui.cpp \
                 crowdin_client.h crowdin_client.cpp \
                 crowdin_gui.h crowdin_gui.cpp \
                 localazy_client.h localazy_client.cpp \
                 localazy_gui.h localazy_gui.cpp \
                 keychain/keytar_posix.cc keychain/keytar.h \
                 json.h
ACCOUNTS_SUPPORT_LIBS = $(CPPREST_LIBS) $(LIBSECRET_LIBS)
endif

poedit_SOURCES = \
                 app_updates.cpp app_updates.h \
                 attentionbar.cpp attentionbar.h \
                 cat_update.h cat_update.cpp \
                 cat_sorting.cpp cat_sorting.h \
                 catalog.cpp catalog.h \
                 catalog_po.cpp catalog_po.h \
                 catalog_json.cpp catalog_json.h \
                 catalog_xcloc.cpp catalog_xcloc.h \
                 catalog_xliff.cpp catalog_xliff.h \
                 uilang.cpp uilang.h \
                 cloud_sync.h \
                 colorscheme.h colorscheme.cpp \
                 commentdlg.h commentdlg.cpp \
                 concurrency.cpp concurrency.h \
                 configuration.cpp configuration.h \
                 custom_buttons.cpp custom_buttons.h \
                 customcontrols.cpp customcontrols.h \
                 custom_notebook.cpp custom_notebook.h \
                 edapp.cpp edapp.h \
                 edframe.cpp edframe.h \
                 editing_area.cpp editing_area.h \
                 edlistctrl.cpp edlistctrl.h \
                 errors.cpp errors.h \
                 export_html.cpp \
                 extractors/extractor.cpp extractors/extractor.h \
                 extractors/extractor_gettext.cpp \
                 extractors/extractor_legacy.cpp extractors/extractor_legacy.h \
                 filemonitor.cpp filemonitor.h \
                 fileviewer.cpp fileviewer.extensions.h fileviewer.h \
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
                 menus.h menus.cpp \
                 pluralforms/pl_evaluate.cpp pluralforms/pl_evaluate.h \
                 prefsdlg.cpp prefsdlg.h \
                 pretranslate.cpp pretranslate.h \
                 progress.h progress_ui.h progress.cpp progress_ui.cpp \
                 propertiesdlg.cpp propertiesdlg.h \
                 qa_checks.cpp qa_checks.h \
                 recent_files.cpp recent_files.h \
                 sidebar.cpp sidebar.h \
                 spellchecking.h spellchecking.cpp \
                 static_ids.h \
                 str_helpers.h \
                 subprocess.h subprocess.cpp \
                 syntaxhighlighter.cpp syntaxhighlighter.h \
                 text_control.h text_control.cpp \
                 titleless_window.h titleless_window.cpp \
                 tm/suggestions.cpp tm/suggestions.h \
                 tm/transmem.cpp tm/transmem.h \
                 tm/tmx_io.cpp tm/tmx_io.h \
                 unicode_helpers.h unicode_helpers.cpp \
                 utility.cpp utility.h \
                 version.h \
                 welcomescreen.cpp welcomescreen.h \
                 pugixml.h \
                 $(ACCOUNTS_SUPPORT_SRC)
nodist_poedit_SOURCES = compiled_xrc.cpp

poedit_LDADD = $(WX_LIBS) $(LUCENE_LIBS) $(CLD2_LIBS) $(PUGIXML_LIBS) $(ACCOUNTS_SUPPORT_LIBS) \
               $(BOOST_LDFLAGS) $(BOOST_THREAD_LIB) $(BOOST_SYSTEM_LIB)

XRC_RESOURCES = \
        $(srcdir)/resources/comment.xrc \
        $(srcdir)/resources/manager.xrc \
        $(srcdir)/resources/menus.xrc \
        $(srcdir)/resources/prefs.xrc \
        $(srcdir)/resources/properties.xrc \
        $(srcdir)/resources/summary.xrc \
        $(srcdir)/resources/toolbar.xrc

compiled_xrc.cpp: $(XRC_RESOURCES)
	$(WXRC) -v -c -o $@ $(XRC_RESOURCES)

DISTCLEANFILES = $(nodist_poedit_SOURCES)

EXTRA_DIST = $(XRC_RESOURCES) pluralforms/COPYING
