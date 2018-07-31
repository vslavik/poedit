
ACLOCAL_AMFLAGS = -I admin

SUBDIRS = src docs locales artwork

desktopdir=$(datadir)/applications
dist_desktop_DATA = net.poedit.Poedit.desktop net.poedit.PoeditURI.desktop

metainfodir=$(datadir)/metainfo
dist_metainfo_DATA = net.poedit.Poedit.appdata.xml

EXTRA_DIST = \
	deps/json/LICENSE.MIT \
	deps/json/single_include/nlohmann/json.hpp \
	deps/pugixml/LICENSE.md \
	deps/pugixml/src/pugiconfig.hpp \
	deps/pugixml/src/pugixml.cpp \
	deps/pugixml/src/pugixml.hpp \
	bootstrap
