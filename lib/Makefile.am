# Makefile.am: Lib subdirectory automake template for the ed line editor.

# Copyright © 1993-2013 Andrew L. Moore, SlewSys Research

# Last modified: 2011-04-14 <alm@slewsys.org>

# Process this file with automake to create Makefile.in

localedir = $(datadir)/locale

noinst_LIBRARIES = libed.a
noinst_HEADERS = getopt_int.h regex_internal.h 
nodist_libed_a_SOURCES =
libed_a_SOURCES =
BUILT_SOURCES =
libed_a_LIBADD = $(LIBOBJS)

AM_CPPFLAGS = -I$(top_srcdir)						      \
              -I$(top_srcdir)/lib					      \
              -I$(top_srcdir)/include					      \
              -I$(top_builddir)						      \
              -I$(top_builddir)/lib					      \
              -I$(top_builddir)/include					      \
              -DLOCALEDIR=\"$(localedir)\"

CLEANFILES =

if LIBADD_GETOPT_LONG
nodist_libed_a_SOURCES += getopt.h
libed_a_SOURCES += getopt.c getopt1.c
BUILT_SOURCES += getopt.h
CLEANFILES += getopt.h
getopt.h: getopt.h.in
	$(LN_S) $(top_srcdir)/lib/$@.in $@

endif LIBADD_GETOPT_LONG

if LIBADD_REGEX
nodist_libed_a_SOURCES += regex.h
libed_a_SOURCES += regex.c
BUILT_SOURCES += regex.h
CLEANFILES += regex.h
AM_CPPFLAGS += -DRE_ENABLE_I18N=1 -D__USE_GNU=1
regex.h: regex.h.in
	$(LN_S) $(top_srcdir)/lib/$@.in $@

endif LIBADD_REGEX

# Force removal of built headers, in the event that distribution is
# reconfigured without a prior `make clean'.
clean-local:
	rm -f getopt.h regex.h

distclean-local: clean-local

# Local variables:
# mode: makefile-gmake
# eval: (add-hook 'write-file-functions 'time-stamp)
# time-stamp-start: "Last modified: "
# time-stamp-format: "%:y-%02m-%02d <%u@%h>"
# time-stamp-end: "$"
# End:
