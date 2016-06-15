# Makefile.am: Top-level automake template for the ed line editor.
#
# Copyright © 1993-2016 Andrew L. Moore, SlewSys Research
#
# Process this file with automake to create Makefile.in

ACLOCAL_AMFLAGS = -I m4

SUBDIRS =

if LDADD_LIBED
SUBDIRS += lib
endif LDADD_LIBED

SUBDIRS += src po doc testsuite

.PHONY: quick-check maintainer-update-dist update-testsuite-dist update-po

quick-check: all
	cd $(top_builddir)/testsuite && $(MAKE) $(AM_MAKEFLAGS) $@

maintainer-update-dist: update-testsuite-dist update-po

update-testsuite-dist:
	@if test ."$(top_srcdir)" != ."$(top_builddir)"; then		      \
	    echo "$@: $(top_builddir): Must run from \$(top_srcdir)";	      \
	    exit 1;							      \
	fi;								      \
	if test ! -w "$(top_srcdir)"; then				      \
	    echo "$@: $(top_srcdir): Permission denied.";		      \
	    exit 1;							      \
	fi;								      \
	chmod +x $(top_srcdir)/bootstrap-sh;				      \
	chmod +x $(top_srcdir)/testsuite/ts-am-sw.ed;			      \
	if test -f $(top_srcdir)/testsuite/.ts-am-sw; then		      \
	  (cd $(top_srcdir)/testsuite && ./ts-am-sw.ed && rm -f ./ts-am-sw);  \
	fi;								      \
	if test ! -f $(top_srcdir)/testsuite/.ts-am-sw; then		      \
	  (cd $(top_srcdir)/testsuite && ./ts-am-sw.ed && touch ./.ts-am-sw); \
	fi;								      \
	(cd $(top_srcdir)/ && ./bootstrap-sh);				      \
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
	(cd $(top_srcdir)/ && ./bootstrap-sh);				      \
	$(MAKE) $(AM_MAKEFLAGS) distclean

distclean-local:
	rm -f configure.orig
	rm -rf  testsuite/.ts-am-sw autom4te.cache
	rm -rf GPATH GRTAGS GSYMS GTAGS
	find . -name '[Tt][Aa][Gg][Ss]' -type f -delete
	find . \( -name '*~' -or -name '#*#' \) -type f -delete
