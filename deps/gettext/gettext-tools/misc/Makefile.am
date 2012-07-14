## Makefile for the gettext-tools/misc subdirectory of GNU gettext
## Copyright (C) 1995-1997, 2000-2003, 2005-2007, 2009-2010 Free Software Foundation, Inc.
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
DISTCLEANFILES =


# Scripts for maintainers.

bin_SCRIPTS = gettextize autopoint

DISTCLEANFILES += gettextize autopoint convert-archive

CLEANFILES += archive.cvs.tar.gz archive.git.tar.gz

EXTRA_DIST += add-to-archive cvsuser.c archive.dir.tar.gz

gettextsrcdir = $(datadir)/gettext
gettextsrc_DATA = archive.@ARCHIVE_FORMAT@.tar.gz

# The archive.dir.tar.gz is not in the CVS. Alpha-testers must use the one from
# an already installed release.
archive.dir.tar.gz:
	if test -f $(gettextsrcdir)/archive.dir.tar.gz; then \
	  cp -p $(gettextsrcdir)/archive.dir.tar.gz $@; \
	else \
	  echo "archive.dir.tar.gz is not under version control. Please copy the archive.dir.tar.gz from an already installed release to gettext-tools/misc/." 1>&2; \
	  exit 1; \
	fi

# The archive.git.tar.gz is generated from archive.dir.tar.gz.
archive.git.tar.gz: archive.dir.tar.gz
	if test -f archive.dir.tar.gz; then \
	  inputfile=archive.dir.tar.gz; \
	else \
	  inputfile='$(srcdir)'/archive.dir.tar.gz; \
	fi; \
	./convert-archive dir git "$$inputfile" $@

# The archive.cvs.tar.gz is generated from archive.dir.tar.gz.
archive.cvs.tar.gz: archive.dir.tar.gz
	if test -f archive.dir.tar.gz; then \
	  inputfile=archive.dir.tar.gz; \
	else \
	  inputfile='$(srcdir)'/archive.dir.tar.gz; \
	fi; \
	./convert-archive dir cvs "$$inputfile" $@

# Emacs Lisp code.

lisp_LISP = start-po.el po-mode.el po-compat.el

EXTRA_DIST += $(lisp_LISP)
