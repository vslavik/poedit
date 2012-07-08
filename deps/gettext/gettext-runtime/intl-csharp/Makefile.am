## Makefile for the gettext-runtime/intl-csharp subdirectory of GNU gettext
## Copyright (C) 2003, 2006 Free Software Foundation, Inc.
##
## This program is free software: you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation; either version 3 of the License, or
## (at your option) any later version.
##
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with this program.  If not, see <http://www.gnu.org/licenses/>.

## Process this file with automake to produce Makefile.in.

AUTOMAKE_OPTIONS = 1.2 gnits
EXTRA_DIST =
CLEANFILES =

RM = rm -f

CSHARPCOMP = $(SHELL) ../csharpcomp.sh
CSHARPCOMPFLAGS = @CSHARPCOMPFLAGS@


all-local: all-dll all-doc
install-data-local: install-dll install-doc
installdirs-local: installdirs-dll installdirs-doc
uninstall-local: uninstall-dll uninstall-doc


# Special rules for C# compilation.

all-dll: all-dll-@BUILDCSHARP@
all-dll-no:
all-dll-yes: GNU.Gettext.dll

GNU.Gettext.dll: intl.cs
	$(CSHARPCOMP) $(CSHARPCOMPFLAGS) -o $@ $(srcdir)/intl.cs

EXTRA_DIST += intl.cs

CLEANFILES += GNU.Gettext.dll GNU.Gettext.dll.mdb

install-dll: install-dll-@BUILDCSHARP@
install-dll-no:
	$(mkdir_p) $(DESTDIR)$(libdir)
install-dll-yes: all-dll-yes
	$(mkdir_p) $(DESTDIR)$(libdir)
	$(INSTALL_DATA) GNU.Gettext.dll $(DESTDIR)$(libdir)/GNU.Gettext.dll

installdirs-dll:
	$(mkdir_p) $(DESTDIR)$(libdir)

uninstall-dll:
	$(RM) $(DESTDIR)$(libdir)/GNU.Gettext.dll


# C# reference documentation. Requires the pnet tools.

csharpdoc:
	test -d csharpdoc || mkdir csharpdoc
	csdoc -flibrary-name=GNU.Gettext intl.cs | \
	csdoc2html -o csharpdoc -fmulti-file -fframes -fcombine-members -fno-namespace-directories -

intl-csharp.texi: intl.cs
	csdoc -flibrary-name=GNU.Gettext intl.cs | \
	csdoc2texi -fembedded -fparent='C#' -o $@ -

all-doc: $(srcdir)/csharpdoc/index.html

DOC_FILES = \
  csharpdoc/index.html \
  csharpdoc/namespaces.html \
  csharpdoc/begin.html \
  csharpdoc/GNU_Gettext.html \
  csharpdoc/GNU_Gettext_GettextResourceManager.html \
  csharpdoc/GNU_Gettext_GettextResourceSet.html

EXTRA_DIST += $(DOC_FILES)

install-doc: all-doc
	$(mkdir_p) $(DESTDIR)$(htmldir)/csharpdoc
	@for f in $(DOC_FILES); do \
	  echo "$(INSTALL_DATA) $(srcdir)/$$f $(DESTDIR)$(htmldir)/$$f"; \
	  $(INSTALL_DATA) $(srcdir)/$$f $(DESTDIR)$(htmldir)/$$f; \
	done

installdirs-doc:
	$(mkdir_p) $(DESTDIR)$(htmldir)/csharpdoc

uninstall-doc:
	@for f in $(DOC_FILES); do \
	  echo "$(RM) $(DESTDIR)$(htmldir)/$$f"; \
	  $(RM) $(DESTDIR)$(htmldir)/$$f; \
	done
