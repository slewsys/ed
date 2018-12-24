# Makefile.am: Src subdirectory automake template for the ed line editor.
#
# Copyright © 1993-2018 Andrew L. Moore, SlewSys Research
#
# Process this file with automake to create Makefile.in

bin_PROGRAMS = ed
ed_SOURCES = aux.c							      \
	     buf.c							      \
	     cbc.c							      \
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
	     utf8.c							      \
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

if LDADD_LIBGNU
ed_LDADD += ../lib/libgnu.a
endif LDADD_LIBGNU

if LIBADD_REGEX
AM_CPPFLAGS += -DRE_ENABLE_I18N=1
endif LIBADD_REGEX

if LDADD_LIBINTL
AM_CPPFLAGS += -I$(top_srcdir)/intl -I$(top_builddir)/intl
ed_LDADD += @LIBINTL@
else !LDADD_LIBINTL
ed_LDADD += @LTLIBINTL@
endif !LDADD_LIBINTL

if LDADD_LIBCRYPT
AM_CPPFLAGS += @OPENSSL_INCLUDES@
ed_LDADD += @OPENSSL_LDFLAGS@ @OPENSSL_LIBS@
endif LDADD_LIBCRYPT

if LDADD_LIBBSD
ed_LDADD += -lbsd
endif LDADD_LIBBSD

maintainer-clean-local:
	rm -f .gdb_history

check-syntax:
	$(CC) -o /dev/null -S $(ed_SOURCES)
