# Makefile.am: Lib subdirectory automake template for the ed line editor.
#
# Copyright © 1993-2016 Andrew L. Moore, SlewSys Research
#
# Process this file with automake to create Makefile.in

# BSD getopt(3) requires optreset -- ln -sf bsd_getopt.h getopt.h
# libed_a_SOURCES = getopt_long.c
# noinst_LIBRARIES = libed.a
# noinst_HEADERS = error.h getopt.h gettext.h pathmax.h

# GNU getopt(3) -- ln -sf gnu_getopt.h getopt.h
#noinst_HEADERS =  getopt.h getopt_int.h gettext.h pathmax.h

noinst_LIBRARIES = libed.a
noinst_HEADERS = gettext.h memcopy.h
libed_a_SOURCES =
libed_a_LIBADD = $(LIBOBJS)

AM_CPPFLAGS = -I$(top_srcdir)						      \
              -I$(top_srcdir)/lib					      \
              -I$(top_srcdir)/include					      \
              -I$(top_builddir)						      \
              -I$(top_builddir)/lib					      \
              -I$(top_builddir)/include					      \
              -DLOCALEDIR=\"$(localedir)\"

localedir = $(datadir)/locale

# getopt.h generated from either gnu_getopt.h or bsd_getopt.h
# (see configure.ac)
if LIBADD_GETOPT_LONG
noinst_HEADERS += getopt.h getopt_int.h
libed_a_SOURCES += getopt.c getopt1.c
endif LIBADD_GETOPT_LONG

# regex.h generated (see configure.ac)
if WITH_INCLUDED_REGEX
noinst_HEADERS += regex.h
libed_a_SOURCES += regex.c
AM_CPPFLAGS += -DRE_ENABLE_I18N=1					      \
               -D__USE_GNU=1						      \
               -I$(top_srcdir)/regex					      \
               -I$(top_builddir)/regex
endif WITH_INCLUDED_REGEX
