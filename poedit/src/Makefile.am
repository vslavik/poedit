
SUBDIRS = resources

RESCOMP = @RESCOMP@
POEDIT_WX_LIBS = @POEDIT_WX_LIBS@

INCLUDES = -I$(top_srcdir)/extras/wx-contrib/include

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
				 pluralforms/pl_evaluate.cpp

poedit_resource.o: poedit.rc
	$(RESCOMP) -i$(srcdir)/poedit.rc -o poedit_resource.o \
        --include-dir $(srcdir) @RC_WX_INCLUDES@ \
        --define __WIN32__ --define __WIN95__ --define __GNUWIN32__

poedit_LDADD = $(RESOURCE_FILES) \
               -L../extras/wx-contrib/src/gizmos \
               -L../extras/wx-contrib/src/xrc \
               -L../extras/wx-contrib/src/xrc/expat \
               -lmy_xrc -lmy_gizmos -lmy_expat $(POEDIT_WX_LIBS)
