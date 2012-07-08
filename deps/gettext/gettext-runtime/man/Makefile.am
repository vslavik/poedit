## Makefile for the gettext-runtime/man subdirectory of GNU gettext
## Copyright (C) 2001-2003, 2006, 2009 Free Software Foundation, Inc.
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

VERSION = @VERSION@

AUTOMAKE_OPTIONS = 1.11 gnits
EXTRA_DIST =

# A manual page for each of the bin_PROGRAMS in src/Makefile.am.

man_aux = gettext.x ngettext.x envsubst.x

# Likewise, plus additional manual pages for the libintl functions.

man_MAN1GEN = gettext.1 ngettext.1
man_MAN1IN = gettext.1.in ngettext.1.in
man_MAN1OTHER = envsubst.1
man_MAN1 = $(man_MAN1GEN) $(man_MAN1OTHER)
man_MAN3 = gettext.3 ngettext.3 \
textdomain.3 bindtextdomain.3 bind_textdomain_codeset.3
man_MAN3IN = gettext.3.in ngettext.3.in \
textdomain.3.in bindtextdomain.3.in bind_textdomain_codeset.3.in
man_MAN3LINK = dgettext.3 dcgettext.3 dngettext.3 dcngettext.3
man_MANS = $(man_MAN1)
notrans_man_MANS = $(man_MAN3) $(man_MAN3LINK)

man_HTMLGEN = gettext.1.html ngettext.1.html
man_HTMLIN = gettext.1.html.in ngettext.1.html.in
man_HTMLOTHER = \
envsubst.1.html \
gettext.3.html ngettext.3.html \
textdomain.3.html bindtextdomain.3.html bind_textdomain_codeset.3.html
man_HTML = $(man_HTMLGEN) $(man_HTMLOTHER)

EXTRA_DIST += help2man $(man_aux) $(man_MAN1IN) $(man_MAN1OTHER) $(man_MAN3) $(man_MAN3IN) $(man_MAN3LINK) $(man_HTMLIN) $(man_HTMLOTHER)
CLEANFILES = $(man_MAN1GEN) $(man_HTMLGEN)
MAINTAINERCLEANFILES = $(man_MAN1IN) $(man_MAN1OTHER) $(man_MAN3) $(man_HTMLIN) $(man_HTMLOTHER)

PERL = @PERL@
RM = rm -f

# help2man 1.24 or newer.
HELP2MAN = $(PERL) -w -- $(srcdir)/help2man

# groff 1.17 or newer.
MAN2HTML = groff -mandoc -Thtml


# We distribute both the man pages and their HTML equivalent.
# The user can generate the parts, via
#   make man
#   make html

all-local: html-local
install-data-local: install-html
installdirs-local: installdirs-html
uninstall-local: uninstall-html


# Man pages.

# The progname.x files contain some extra information not found in the
# "progname --help" output.

gettext.1: gettext.1.in Makefile
	sed -e 's|@''localedir''@|$(localedir)|g' < `if test -f gettext.1.in; then echo .; else echo $(srcdir); fi`/gettext.1.in > t-$@
	mv t-$@ $@
ngettext.1: ngettext.1.in Makefile
	sed -e 's|@''localedir''@|$(localedir)|g' < `if test -f ngettext.1.in; then echo .; else echo $(srcdir); fi`/ngettext.1.in > t-$@
	mv t-$@ $@

gettext.1.in: gettext.x
	IN_HELP2MAN=1 $(SHELL) x-to-1 $(UPDATEMODE) "$(PERL)" "$(HELP2MAN)" ../src/gettext$(EXEEXT) $(srcdir)/gettext.x gettext.1.in
ngettext.1.in: ngettext.x
	IN_HELP2MAN=1 $(SHELL) x-to-1 $(UPDATEMODE) "$(PERL)" "$(HELP2MAN)" ../src/ngettext$(EXEEXT) $(srcdir)/ngettext.x ngettext.1.in

envsubst.1: envsubst.x
	$(SHELL) x-to-1 $(UPDATEMODE) "$(PERL)" "$(HELP2MAN)" ../src/envsubst$(EXEEXT) $(srcdir)/envsubst.x envsubst.1

# Depend on version.sh to get version number changes.
$(man_MAN1IN) $(man_MAN1OTHER): help2man $(top_srcdir)/../version.sh

# Update them also during "make dist", in order to propagate added command
# line options that were added, even if version.sh didn't change.
UPDATEMODE =
update-man1:
	$(MAKE) $(man_MAN1IN) $(man_MAN1OTHER) UPDATEMODE=--update
	$(MAKE)
# Hidden from automake, but really activated. Works around an automake-1.5 bug.
#distdir: update-man1

gettext.3: gettext.3.in $(top_srcdir)/../version.sh
	sed -e 's/@''VERSION''@/@VERSION@/g' < $(srcdir)/gettext.3.in > t-$@
	mv t-$@ $@
ngettext.3: ngettext.3.in $(top_srcdir)/../version.sh
	sed -e 's/@''VERSION''@/@VERSION@/g' < $(srcdir)/ngettext.3.in > t-$@
	mv t-$@ $@
textdomain.3: textdomain.3.in $(top_srcdir)/../version.sh
	sed -e 's/@''VERSION''@/@VERSION@/g' < $(srcdir)/textdomain.3.in > t-$@
	mv t-$@ $@
bindtextdomain.3: bindtextdomain.3.in $(top_srcdir)/../version.sh
	sed -e 's/@''VERSION''@/@VERSION@/g' < $(srcdir)/bindtextdomain.3.in > t-$@
	mv t-$@ $@
bind_textdomain_codeset.3: bind_textdomain_codeset.3.in $(top_srcdir)/../version.sh
	sed -e 's/@''VERSION''@/@VERSION@/g' < $(srcdir)/bind_textdomain_codeset.3.in > t-$@
	mv t-$@ $@


# Man pages in HTML format.

html-local: $(man_HTML)

gettext.1.html: gettext.1.html.in Makefile
	sed -e 's|@''localedir''@|$(localedir)|g' < `if test -f gettext.1.html.in; then echo .; else echo $(srcdir); fi`/gettext.1.html.in > t-$@
	mv t-$@ $@
ngettext.1.html: ngettext.1.html.in Makefile
	sed -e 's|@''localedir''@|$(localedir)|g' < `if test -f ngettext.1.html.in; then echo .; else echo $(srcdir); fi`/ngettext.1.html.in > t-$@
	mv t-$@ $@

gettext.1.html.in: gettext.1.in
	$(MAN2HTML) `if test -f gettext.1.in; then echo .; else echo $(srcdir); fi`/gettext.1.in | sed -e '/CreationDate:/d' > t-$@
	mv t-$@ $@
ngettext.1.html.in: ngettext.1.in
	$(MAN2HTML) `if test -f ngettext.1.in; then echo .; else echo $(srcdir); fi`/ngettext.1.in | sed -e '/CreationDate:/d' > t-$@
	mv t-$@ $@

envsubst.1.html: envsubst.1
	$(MAN2HTML) `if test -f envsubst.1; then echo .; else echo $(srcdir); fi`/envsubst.1 | sed -e '/CreationDate:/d' > t-$@
	mv t-$@ $@

gettext.3.html: gettext.3.in
	$(MAN2HTML) $(srcdir)/gettext.3.in | sed -e '/CreationDate:/d' > t-$@
	mv t-$@ $@
ngettext.3.html: ngettext.3.in
	$(MAN2HTML) $(srcdir)/ngettext.3.in | sed -e '/CreationDate:/d' > t-$@
	mv t-$@ $@
textdomain.3.html: textdomain.3.in
	$(MAN2HTML) $(srcdir)/textdomain.3.in | sed -e '/CreationDate:/d' > t-$@
	mv t-$@ $@
bindtextdomain.3.html: bindtextdomain.3.in
	$(MAN2HTML) $(srcdir)/bindtextdomain.3.in | sed -e '/CreationDate:/d' > t-$@
	mv t-$@ $@
bind_textdomain_codeset.3.html: bind_textdomain_codeset.3.in
	$(MAN2HTML) $(srcdir)/bind_textdomain_codeset.3.in | sed -e '/CreationDate:/d' > t-$@
	mv t-$@ $@

install-html-local:
	$(mkdir_p) $(DESTDIR)$(htmldir)
	for file in $(man_HTML); do \
	  if test -f $$file; then dir=.; else dir=$(srcdir); fi; \
	  $(INSTALL_DATA) $$dir/$$file $(DESTDIR)$(htmldir)/$$file; \
	done

installdirs-html:
	$(mkdir_p) $(DESTDIR)$(htmldir)

uninstall-html:
	for file in $(man_HTML); do \
	  $(RM) $(DESTDIR)$(htmldir)/$$file; \
	done
