# Example for use of GNU gettext.
# This file is in the public domain.
#
# Makefile configuration - processed by automake.

# General automake options.
AUTOMAKE_OPTIONS = foreign no-dependencies
ACLOCAL_AMFLAGS = -I m4

# The list of subdirectories containing Makefiles.
SUBDIRS = m4 po

# The list of programs that are built.
bin_PROGRAMS = hello

# The source files of the 'hello' program.
hello_SOURCES = hello.c resources.c

# Define a C macro LOCALEDIR indicating where catalogs will be installed.
DEFS = -DLOCALEDIR=\"$(localedir)\" @DEFS@

# Make sure the gnome.h include file is found.
AM_CPPFLAGS = $(GTK_CFLAGS)

# Link time dependencies.
LDADD = $(GTK_LIBS) @LIBINTL@

BUILT_SOURCES = gschemas.compiled resources.c

# Compile GSettings schema.
gschemas.compiled: hello.gschema.xml
	$(AM_V_GEN) $(GLIB_COMPILE_SCHEMAS) .

# Compile assets into a C source and link it with the application.
resources.c: hello.gresource.xml hello.ui
	$(AM_V_GEN) $(GLIB_COMPILE_RESOURCES) $< --target=$@ \
		--sourcedir=$(srcdir) --generate-source

# Merge translations into a Desktop Entry file.
desktopdir = $(datadir)/applications
desktop_DATA = hello.desktop

hello.desktop.in: hello.desktop.in.in
	$(AM_V_GEN) $(MSGFMT) --desktop --template $< -o $@-t \
	  -d $(top_srcdir)/po && mv $@-t $@

hello.desktop: hello.desktop.in
	@rm -f $@ $@-t
	$(AM_V_GEN) srcdir=''; \
	  test -f ./$< || srcdir=$(srcdir)/; \
	  sed -e "s|@bindir[@]|$(bindir)|" $< > $@-t && mv $@-t $@

CLEANFILES = $(BUILT_SOURCES) hello.desktop $(desktop_DATA)

# Additional files to be distributed.
EXTRA_DIST = autogen.sh autoclean.sh hello.ui \
	hello.desktop.in.in hello.desktop.in \
	hello.gschema.xml \
	hello.gresource.xml

MAINTAINERCLEANFILES = hello.desktop.in
