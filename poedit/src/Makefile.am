
SUBDIRS = resources

RESCOMP = @RESCOMP@

INCLUDES = -I$(top_srcdir)/extras/wx-contrib/include

if USE_WINDOWS
RESOURCE_FILES = poedit_resource.o
endif

bin_PROGRAMS = poedit

poedit_SOURCES = catalog.cpp catalog.h edapp.cpp edapp.h edframe.cpp \
                 edframe.h fileviewer.cpp fileviewer.h parser.cpp parser.h \
                 iso639.cpp iso639.h prefsdlg.cpp prefsdlg.h settingsdlg.cpp \
                 settingsdlg.h progressinfo.h \
                 progressinfo.cpp digger.h digger.cpp gexecute.h \
                 gexecute.cpp summarydlg.h summarydlg.cpp \
                 findframe.cpp findframe.h commentdlg.h commentdlg.cpp \
                 transmem.cpp transmem.h transmemupd.cpp transmemupd.h \
                 manager.h manager.cpp

poedit_resource.o: poedit.rc
	$(RESCOMP) -i$(srcdir)/poedit.rc -o poedit_resource.o \
        --include-dir $(srcdir) --include-dir `@WX_CONFIG@ --prefix`/include \
        --define __WIN32__ --define __WIN95__

poedit_LDADD = $(RESOURCE_FILES) \
               -L../extras/wx-contrib/src/gizmos \
               -L../extras/wx-contrib/src/xrc \
               -L../extras/wx-contrib/src/xrc/expat \
               -lmy_xrc -lmy_gizmos -lmy_expat $(WX_LIBS)
