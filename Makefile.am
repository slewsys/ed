# Makefile.am: Top-level automake template for the ed line editor.
#
# Process this file with automake to create Makefile.in

gnulib_srcdir ?= /opt/src/savannah/gnulib

ACLOCAL_AMFLAGS = -I m4

SUBDIRS =

if LDADD_LIBGNU
SUBDIRS += lib
endif LDADD_LIBGNU

SUBDIRS += src po doc testsuite

EXTRA_DIST = m4/gnulib-cache.m4 include/pathmax.h include/argmax.h

HEAD_COMMIT = $(shell git rev-parse --short HEAD)

.PHONY: maintainer-update-dist update-testsuite-dist update-po

# dist:
#	git -C $(abs_top_srcdir) archive --format=tar			      \
#		--prefix=$(PACKAGE)-$(VERSION)/ v$(VERSION) |		      \
#		gzip >$(abs_top_builddir)/ed-$(VERSION)~g$(HEAD_COMMIT).tgz

check-valgrind: clean
	$(MAKE) $(AM_MAKEFLAGS) CFLAGS='-g -O0'				      \
	    -C $(abs_top_builddir) all
	$(MAKE) -C $(abs_top_srcdir)/testsuite check-valgrind

maintainer-update-dist: update-gnulib update-po

spdx-sbom:
	reuse --suppress-deprecation spdx > $(PACKAGE)-$(VERSION).spdx

update-gnulib:
	@if test ! -x $(gnulib_srcdir)/gnulib-tool; then		      \
	  echo "gnulib-tool: No such file or directory.";		      \
	  exit;								      \
	else								      \
	  echo "*** Refreshing gnulib sources ***";			      \
	fi;								      \
	git -C "$(gnulib_srcdir)" pull;					      \
	echo "*** Importing gnulib modules ***";			      \
	if test -f lib/regex.h; then					      \
	  rm -f lib/regex.h;						      \
	fi;								      \
	"$(gnulib_srcdir)/gnulib-tool" --import				      \
	  --dir=$($top_srcdir)                                                \
	  --lib=libgnu							      \
	  --source-base=lib						      \
	  --m4-base=m4							      \
	  --po-base=po                                                        \
	  --doc-base=doc						      \
	  --tests-base=tests						      \
	  --aux-dir=$(top_srcdir)					      \
	  --conditional-dependencies					      \
	  --no-libtool							      \
	  --macro-prefix=gl						      \
	  --po-domain=ed                                                      \
	  getopt-gnu							      \
	  regex
	@if test -f lib/regex.h; then					      \
	  echo "*** Moving lib/regex.h => lib/regex.h.in ***";		      \
	  mv lib/regex.h lib/regex.h.in;				      \
	fi;								      \
	if test -f lib/dynarray.h; then					      \
	  echo "*** Moving lib/dynarray.h => lib/dynarray.h.in ***";	      \
	  mv lib/dynarray.h lib/dynarray.h.in;				      \
	fi;								      \
	if test -f lib/malloc/dynarray.h; then				      \
	  echo "*** Moving lib/malloc/dynarray.h => lib/malloc/dynarray.h.in ***"; \
	  mv lib/malloc/dynarray.h lib/malloc/dynarray.h.in;		      \
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
	rm -f configure.orig  GPATH GRTAGS GSYMS GTAGS
	rm -rf .cache
	find . -name '[Tt][Aa][Gg][Ss]' -type f -delete
	find . \( -name '*~' -or -name '#*#' \) -type f -delete
	if test -f $(top_builddir)/testsuite/Makefile; then		      \
		$(MAKE) -C $(top_builddir)/testsuite distclean;		      \
	fi
	rm -f po/POTFILES.in
