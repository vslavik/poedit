## Makefile for the gettext-tools/man subdirectory of GNU gettext
## Copyright (C) 2001-2003, 2006-2007 Free Software Foundation, Inc.
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

AUTOMAKE_OPTIONS = 1.2 gnits
EXTRA_DIST =

# A manual page for each of the bin_PROGRAMS in src/Makefile.am
# and for each of the bin_SCRIPTS in misc/Makefile.am.

man_aux = \
msgcmp.x msgfmt.x msgmerge.x msgunfmt.x xgettext.x \
msgattrib.x msgcat.x msgcomm.x msgconv.x msgen.x msgexec.x msgfilter.x msggrep.x msginit.x msguniq.x \
recode-sr-latin.x \
gettextize.x autopoint.x

# Likewise.

man_MAN1OTHER = \
msgcmp.1 msgfmt.1 msgmerge.1 msgunfmt.1 xgettext.1 \
msgattrib.1 msgcat.1 msgcomm.1 msgconv.1 msgen.1 msgexec.1 msgfilter.1 msggrep.1 msginit.1 msguniq.1 \
recode-sr-latin.1 \
gettextize.1 autopoint.1
man_MAN1 = $(man_MAN1OTHER)
man_MANS = $(man_MAN1)

man_HTMLOTHER = \
msgcmp.1.html msgfmt.1.html msgmerge.1.html msgunfmt.1.html xgettext.1.html \
msgattrib.1.html msgcat.1.html msgcomm.1.html msgconv.1.html msgen.1.html msgexec.1.html msgfilter.1.html msggrep.1.html msginit.1.html msguniq.1.html \
recode-sr-latin.1.html \
gettextize.1.html autopoint.1.html
man_HTML = $(man_HTMLOTHER)

EXTRA_DIST += help2man $(man_aux) $(man_MAN1OTHER) $(man_HTMLOTHER) ChangeLog.0
MAINTAINERCLEANFILES = $(man_MAN1OTHER) $(man_HTMLOTHER)

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

msgcmp.1: msgcmp.x
	$(SHELL) x-to-1 $(UPDATEMODE) "$(PERL)" "$(HELP2MAN)" ../src/msgcmp$(EXEEXT) $(srcdir)/msgcmp.x msgcmp.1
msgfmt.1: msgfmt.x
	$(SHELL) x-to-1 $(UPDATEMODE) "$(PERL)" "$(HELP2MAN)" ../src/msgfmt$(EXEEXT) $(srcdir)/msgfmt.x msgfmt.1
msgmerge.1: msgmerge.x
	$(SHELL) x-to-1 $(UPDATEMODE) "$(PERL)" "$(HELP2MAN)" ../src/msgmerge$(EXEEXT) $(srcdir)/msgmerge.x msgmerge.1
msgunfmt.1: msgunfmt.x
	$(SHELL) x-to-1 $(UPDATEMODE) "$(PERL)" "$(HELP2MAN)" ../src/msgunfmt$(EXEEXT) $(srcdir)/msgunfmt.x msgunfmt.1
xgettext.1: xgettext.x
	$(SHELL) x-to-1 $(UPDATEMODE) "$(PERL)" "$(HELP2MAN)" ../src/xgettext$(EXEEXT) $(srcdir)/xgettext.x xgettext.1
msgattrib.1: msgattrib.x
	$(SHELL) x-to-1 $(UPDATEMODE) "$(PERL)" "$(HELP2MAN)" ../src/msgattrib$(EXEEXT) $(srcdir)/msgattrib.x msgattrib.1
msgcat.1: msgcat.x
	$(SHELL) x-to-1 $(UPDATEMODE) "$(PERL)" "$(HELP2MAN)" ../src/msgcat$(EXEEXT) $(srcdir)/msgcat.x msgcat.1
msgcomm.1: msgcomm.x
	$(SHELL) x-to-1 $(UPDATEMODE) "$(PERL)" "$(HELP2MAN)" ../src/msgcomm$(EXEEXT) $(srcdir)/msgcomm.x msgcomm.1
msgconv.1: msgconv.x
	$(SHELL) x-to-1 $(UPDATEMODE) "$(PERL)" "$(HELP2MAN)" ../src/msgconv$(EXEEXT) $(srcdir)/msgconv.x msgconv.1
msgen.1: msgen.x
	$(SHELL) x-to-1 $(UPDATEMODE) "$(PERL)" "$(HELP2MAN)" ../src/msgen$(EXEEXT) $(srcdir)/msgen.x msgen.1
msgexec.1: msgexec.x
	$(SHELL) x-to-1 $(UPDATEMODE) "$(PERL)" "$(HELP2MAN)" ../src/msgexec$(EXEEXT) $(srcdir)/msgexec.x msgexec.1
msgfilter.1: msgfilter.x
	$(SHELL) x-to-1 $(UPDATEMODE) "$(PERL)" "$(HELP2MAN)" ../src/msgfilter$(EXEEXT) $(srcdir)/msgfilter.x msgfilter.1
msggrep.1: msggrep.x
	$(SHELL) x-to-1 $(UPDATEMODE) "$(PERL)" "$(HELP2MAN)" ../src/msggrep$(EXEEXT) $(srcdir)/msggrep.x msggrep.1
msginit.1: msginit.x
	$(SHELL) x-to-1 $(UPDATEMODE) "$(PERL)" "$(HELP2MAN)" ../src/msginit$(EXEEXT) $(srcdir)/msginit.x msginit.1
msguniq.1: msguniq.x
	$(SHELL) x-to-1 $(UPDATEMODE) "$(PERL)" "$(HELP2MAN)" ../src/msguniq$(EXEEXT) $(srcdir)/msguniq.x msguniq.1
recode-sr-latin.1: recode-sr-latin.x
	$(SHELL) x-to-1 $(UPDATEMODE) "$(PERL)" "$(HELP2MAN)" ../src/recode-sr-latin$(EXEEXT) $(srcdir)/recode-sr-latin.x recode-sr-latin.1

gettextize.1: gettextize.x
	$(SHELL) x-to-1 $(UPDATEMODE) "$(PERL)" "$(HELP2MAN)" ../misc/gettextize $(srcdir)/gettextize.x gettextize.1
autopoint.1: autopoint.x
	$(SHELL) x-to-1 $(UPDATEMODE) "$(PERL)" "$(HELP2MAN)" ../misc/autopoint $(srcdir)/autopoint.x autopoint.1

# Depend on version.sh to get version number changes.
$(man_MAN1): help2man $(top_srcdir)/../version.sh

# Update them also during "make dist", in order to propagate added command
# line options that were added, even if version.sh didn't change.
UPDATEMODE =
update-man1:
	$(MAKE) $(man_MAN1) UPDATEMODE=--update
	$(MAKE)
# Hidden from automake, but really activated. Works around an automake-1.5 bug.
#distdir: update-man1


# Man pages in HTML format.

html-local: $(man_HTML)

msgcmp.1.html: msgcmp.1
	$(MAN2HTML) `if test -f msgcmp.1; then echo .; else echo $(srcdir); fi`/msgcmp.1 | sed -e '/CreationDate:/d' > t-$@
	mv t-$@ $@
msgfmt.1.html: msgfmt.1
	$(MAN2HTML) `if test -f msgfmt.1; then echo .; else echo $(srcdir); fi`/msgfmt.1 | sed -e '/CreationDate:/d' > t-$@
	mv t-$@ $@
msgmerge.1.html: msgmerge.1
	$(MAN2HTML) `if test -f msgmerge.1; then echo .; else echo $(srcdir); fi`/msgmerge.1 | sed -e '/CreationDate:/d' > t-$@
	mv t-$@ $@
msgunfmt.1.html: msgunfmt.1
	$(MAN2HTML) `if test -f msgunfmt.1; then echo .; else echo $(srcdir); fi`/msgunfmt.1 | sed -e '/CreationDate:/d' > t-$@
	mv t-$@ $@
xgettext.1.html: xgettext.1
	$(MAN2HTML) `if test -f xgettext.1; then echo .; else echo $(srcdir); fi`/xgettext.1 | sed -e '/CreationDate:/d' > t-$@
	mv t-$@ $@
msgattrib.1.html: msgattrib.1
	$(MAN2HTML) `if test -f msgattrib.1; then echo .; else echo $(srcdir); fi`/msgattrib.1 | sed -e '/CreationDate:/d' > t-$@
	mv t-$@ $@
msgcat.1.html: msgcat.1
	$(MAN2HTML) `if test -f msgcat.1; then echo .; else echo $(srcdir); fi`/msgcat.1 | sed -e '/CreationDate:/d' > t-$@
	mv t-$@ $@
msgcomm.1.html: msgcomm.1
	$(MAN2HTML) `if test -f msgcomm.1; then echo .; else echo $(srcdir); fi`/msgcomm.1 | sed -e '/CreationDate:/d' > t-$@
	mv t-$@ $@
msgconv.1.html: msgconv.1
	$(MAN2HTML) `if test -f msgconv.1; then echo .; else echo $(srcdir); fi`/msgconv.1 | sed -e '/CreationDate:/d' > t-$@
	mv t-$@ $@
msgen.1.html: msgen.1
	$(MAN2HTML) `if test -f msgen.1; then echo .; else echo $(srcdir); fi`/msgen.1 | sed -e '/CreationDate:/d' > t-$@
	mv t-$@ $@
msgexec.1.html: msgexec.1
	$(MAN2HTML) `if test -f msgexec.1; then echo .; else echo $(srcdir); fi`/msgexec.1 | sed -e '/CreationDate:/d' > t-$@
	mv t-$@ $@
msgfilter.1.html: msgfilter.1
	$(MAN2HTML) `if test -f msgfilter.1; then echo .; else echo $(srcdir); fi`/msgfilter.1 | sed -e '/CreationDate:/d' > t-$@
	mv t-$@ $@
msggrep.1.html: msggrep.1
	$(MAN2HTML) `if test -f msggrep.1; then echo .; else echo $(srcdir); fi`/msggrep.1 | sed -e '/CreationDate:/d' > t-$@
	mv t-$@ $@
msginit.1.html: msginit.1
	$(MAN2HTML) `if test -f msginit.1; then echo .; else echo $(srcdir); fi`/msginit.1 | sed -e '/CreationDate:/d' > t-$@
	mv t-$@ $@
msguniq.1.html: msguniq.1
	$(MAN2HTML) `if test -f msguniq.1; then echo .; else echo $(srcdir); fi`/msguniq.1 | sed -e '/CreationDate:/d' > t-$@
	mv t-$@ $@
recode-sr-latin.1.html: recode-sr-latin.1
	$(MAN2HTML) `if test -f recode-sr-latin.1; then echo .; else echo $(srcdir); fi`/recode-sr-latin.1 | sed -e '/CreationDate:/d' > t-$@
	mv t-$@ $@
gettextize.1.html: gettextize.1
	$(MAN2HTML) `if test -f gettextize.1; then echo .; else echo $(srcdir); fi`/gettextize.1 | sed -e '/CreationDate:/d' > t-$@
	mv t-$@ $@
autopoint.1.html: autopoint.1
	$(MAN2HTML) `if test -f autopoint.1; then echo .; else echo $(srcdir); fi`/autopoint.1 | sed -e '/CreationDate:/d' > t-$@
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
