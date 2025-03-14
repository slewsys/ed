# Makefile.am: Src subdirectory automake template for the ed line editor.
#
# Process this file with automake to create Makefile.in

bin_PROGRAMS = ed
ed_SOURCES = buffer.c							\
	     cbc.c							\
	     cmds.c							\
	     ed.h							\
	     exec.c							\
	     filter.c							\
	     global.c							\
	     io.c							\
	     main.c							\
	     page.c							\
	     parse.c							\
	     regexp.c							\
	     register.c							\
	     signal.c							\
	     subst.c							\
	     undo.c							\
	     utf8.c							\
	     version.c

ed_LDADD =

# To override system libs, link order important here.
localedir = $(datadir)/locale

AM_CPPFLAGS = -I$(top_srcdir)						\
	      -I$(top_builddir)						\
	      -I$(top_srcdir)/lib					\
	      -I$(top_builddir)/lib					\
	      -I$(top_srcdir)/include					\
	      -I$(top_builddir)/include					\
	      -DLOCALEDIR=\"$(localedir)\"				\
	      -D_FORTIFY_SOURCE=3

if LDADD_GNULIB
ed_LDADD += ../lib/libgnu.a
endif LDADD_GNULIB

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

clean-local:
	rm -f .gdb_history

distclean-local: clean-local
	rm -f Makefile

maintainer-clean-local: clean-local
