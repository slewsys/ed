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

EXTRA_DIST = m4/gnulib-cache.m4

.PHONY: quick-check maintainer-update-dist update-testsuite-dist update-po

quick-check: all
	cd $(top_builddir)/testsuite && $(MAKE) $(AM_MAKEFLAGS) $@

maintainer-update-dist: update-gnulib update-testsuite-dist update-po

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
	fi;								      \
	patch lib/Makefile.am lib/patch-Makefile.am.diff;		      \
	echo "*** Running autogen.sh ***";				      \
	./autogen.sh --silent

update-testsuite-dist:
	@if test ."$(top_srcdir)" != ."$(top_builddir)"; then		      \
	    echo "$@: $(top_builddir): Must run from \$(top_srcdir)";	      \
	    exit 1;							      \
	fi;								      \
	if test ! -w "$(top_srcdir)"; then				      \
	    echo "$@: $(top_srcdir): Permission denied.";		      \
	    exit 1;							      \
	fi;								      \
	chmod +x $(top_srcdir)/upgrade.sh;				      \
	chmod +x $(top_srcdir)/testsuite/ts-am-sw.ed;			      \
	if test -f $(top_srcdir)/testsuite/.ts-am-sw; then		      \
	  (cd $(top_srcdir)/testsuite && ./ts-am-sw.ed && rm -f ./ts-am-sw);  \
	fi;								      \
	if test ! -f $(top_srcdir)/testsuite/.ts-am-sw; then		      \
	  (cd $(top_srcdir)/testsuite && ./ts-am-sw.ed && touch ./.ts-am-sw); \
	fi;								      \
	(cd $(top_srcdir)/ && ./upgrade.sh);				      \
	$(MAKE) $(AM_MAKEFLAGS) all
	-rm -f $(top_srcdir)/testsuite/testsuite;			      \
	$(MAKE) $(AM_MAKEFLAGS) check;					      \
	cp $(top_srcdir)/testsuite/testsuite				      \
	  $(top_srcdir)/testsuite/testsuite-dist

update-po:
	-(cd $(top_srcdir)/po && $(MAKE) $(AM_MAKEFLAGS) update-po);	      \
	if test -f $(top_srcdir)/testsuite/.ts-am-sw; then		      \
	  (cd $(top_srcdir)/testsuite && ./ts-am-sw.ed && rm -f ./ts-am-sw);  \
	fi;								      \
	(cd $(top_srcdir)/ && ./upgrade.sh);				      \
	$(MAKE) $(AM_MAKEFLAGS) distclean

distclean-local:
	rm -f configure.orig
	rm -rf  testsuite/.ts-am-sw autom4te.cache
	rm -rf GPATH GRTAGS GSYMS GTAGS
	find . -name '[Tt][Aa][Gg][Ss]' -type f -delete
	find . \( -name '*~' -or -name '#*#' \) -type f -delete
