
bin_PROGRAMS = poedit

poedit_SOURCES = catalog.cpp catalog.h edapp.cpp edframe.cpp \
                 edframe.h fileviewer.cpp fileviewer.h parser.cpp parser.h \
                 iso639.h prefsdlg.cpp prefsdlg.h settingsdlg.cpp \
                 settingsdlg.h compiled_resource.cpp progressinfo.h \
                 progressinfo.cpp digger.h digger.cpp gexecute.h \
                 gexecute.cpp summarydlg.h summarydlg.cpp
poedit_LDADD = -lwxxmlrc -lwxxml

compiled_resource.cpp: resource.xml
	$(WXRC) -v -c -o compiled_resource.cpp $(srcdir)/resource.xml

EXTRA_DIST = resource.xml
