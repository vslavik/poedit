
SUBDIRS = wxxml_snapshot

INCLUDES = -I$(srcdir)/wxxml_snapshot/include

bin_PROGRAMS = poedit

poedit_SOURCES = catalog.cpp catalog.h edapp.cpp edframe.cpp \
                 edframe.h fileviewer.cpp fileviewer.h parser.cpp parser.h \
                 iso639.h prefsdlg.cpp prefsdlg.h settingsdlg.cpp \
                 settingsdlg.h compiled_resource.cpp progressinfo.h \
                 progressinfo.cpp digger.h digger.cpp gexecute.h \
                 gexecute.cpp summarydlg.h summarydlg.cpp
poedit_LDADD = -Lwxxml_snapshot -lwxxml_snapshot $(LDFLAGS)

EXTRA_DIST = resource.xml

dist-hook:
	mkdir $(distdir)/bitmaps
	cp -p $(srcdir)/bitmaps/*.xpm $(srcdir)/bitmaps/*.ico \
          $(srcdir)/bitmaps/*.gif $(distdir)/bitmaps
