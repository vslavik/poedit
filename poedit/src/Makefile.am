
SUBDIRS = resources

INCLUDES = -I$(top_srcdir)/extras/wx-contrib/include

bin_PROGRAMS = poedit

poedit_SOURCES = catalog.cpp catalog.h edapp.cpp edframe.cpp \
                 edframe.h fileviewer.cpp fileviewer.h parser.cpp parser.h \
                 iso639.h prefsdlg.cpp prefsdlg.h settingsdlg.cpp \
                 settingsdlg.h progressinfo.h \
                 progressinfo.cpp digger.h digger.cpp gexecute.h \
                 gexecute.cpp summarydlg.h summarydlg.cpp

LDADD = -L../extras/wx-contrib/src/gizmos \
        -L../extras/wx-contrib/src/xml \
        -L../extras/wx-contrib/src/xml/expat \
        -lmy_xrc -lmy_gizmos -lmy_expat $(WX_LIBS)

#dist-hook:
#	mkdir $(distdir)/bitmaps
#	cp -p $(srcdir)/bitmaps/*.xpm $(srcdir)/bitmaps/*.ico \
#          $(srcdir)/bitmaps/*.gif $(distdir)/bitmaps
