# Makefile.am: Top-level automake template for the ed line editor.
#
# Copyright © 1993-2018 Andrew L. Moore, SlewSys Research
#
# Process this file with automake to create Makefile.in

gnulib_srcdir ?= /opt/src/gnu/gnulib

ACLOCAL_AMFLAGS = -I m4

SUBDIRS =

if LDADD_LIBGNU
SUBDIRS += lib
endif LDADD_LIBGNU

SUBDIRS += src po doc testsuite

EXTRA_DIST = m4/gnulib-cache.m4 include/pathmax.h include/argmax.h

.PHONY: quick-check maintainer-update-dist update-testsuite-dist update-po

quick-check: all
	cd $(top_builddir)/testsuite && $(MAKE) $(AM_MAKEFLAGS) $@

maintainer-update-dist: update-gnulib update-po

update-gnulib:
	@if test ! -x $(gnulib_srcdir)/gnulib-tool; then		      \
	  echo "ghulib-tool: No such file or directory.";		      \
	  exit;								      \
	else								      \
	  echo "*** Refreshing gnulib sources ***";			      \
	fi;								      \
	opwd=$$PWD;							      \
	cd "$(gnulib_srcdir)" && git pull;				      \
	cd $$opwd;							      \
	echo "*** Importing gnulib modules ***";			      \
	if test -f lib/regex.h; then					      \
	  rm -f lib/regex.h;						      \
	fi;								      \
	"$(gnulib_srcdir)/gnulib-tool" --import				      \
	  --lib=libgnu							      \
	  --source-base=lib						      \
	  --m4-base=m4							      \
	  --doc-base=doc						      \
	  --tests-base=tests						      \
	  --aux-dir=.							      \
	  --conditional-dependencies					      \
	  --no-libtool							      \
	  --macro-prefix=gl						      \
	  getopt-gnu							      \
	  regex
	@if test -f lib/regex.h; then					      \
	  echo "*** Moving lib/regex.h => lib/regex.h.in ***";		      \
	  mv lib/regex.h lib/regex.h.in;				      \
	fi;								      \
	if test -f lib/patch-Makefile.am.diff; then			      \
	  echo "*** Patching refreshed lib/Makefile.am ***";		      \
	  patch lib/Makefile.am lib/patch-Makefile.am.diff;		      \
	fi;								      \
	echo "*** Running autogen.sh ***";				      \
	./autogen.sh --silent

update-po:
	-$(MAKE) -C po $(AM_MAKEFLAGS) update-po

distclean-local:
	rm -f configure.orig
	rm -rf GPATH GRTAGS GSYMS GTAGS
	find . -name '[Tt][Aa][Gg][Ss]' -type f -delete
	find . \( -name '*~' -or -name '#*#' \) -type f -delete
