# Makefile.am: Src subdirectory automake template for the ed line editor.

# Copyright © 1993-2014 Andrew L. Moore, SlewSys Research

# Last modified: 2011-04-14 <alm@slewsys.org>

# Process this file with automake to create Makefile.in

bin_PROGRAMS = ed
ed_SOURCES = aux.c							      \
             buf.c							      \
             cmds.c							      \
             ed.h							      \
             exec.c							      \
             glbl.c							      \
             io.c							      \
             main.c							      \
             page.c							      \
             parse.c							      \
             re.c							      \
             signal.c							      \
             sub.c							      \
             undo.c							      \
             version.c

ed_LDADD =

# To override system libs, link order important here.
localedir = $(datadir)/locale

AM_CPPFLAGS = -I$(top_srcdir)						      \
              -I$(top_builddir)						      \
              -I$(top_srcdir)/lib					      \
              -I$(top_builddir)/lib					      \
              -I$(top_srcdir)/include					      \
              -I$(top_builddir)/include					      \
              -DLOCALEDIR=\"$(localedir)\"

if LDADD_LIBED
ed_LDADD += ../lib/libed.a
endif LDADD_LIBED

if LIBADD_REGEX
AM_CPPFLAGS += -DRE_ENABLE_I18N=1 -D__USE_GNU=1
endif

if LDADD_LIBINTL
AM_CPPFLAGS += -I$(top_srcdir)/intl -I$(top_builddir)/intl
ed_LDADD += @LIBINTL@
else !LDADD_LIBINTL
ed_LDADD += @LTLIBINTL@
endif !LDADD_LIBINTL

maintainer-clean-local:
	rm -f .gdb_history

check-syntax:
	gcc -o /dev/null -S $(ed_SOURCES)

# Local variables:
# mode: makefile-gmake
# eval: (add-hook 'write-file-functions 'time-stamp)
# time-stamp-start: "Last modified: "
# time-stamp-format: "%:y-%02m-%02d <%u@%h>"
# time-stamp-end: "$"
# End:
