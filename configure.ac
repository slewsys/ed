# configure.ac: Configure autoconf template for the ed line editor.

# Copyright © 1993-2013 Andrew L. Moore, SlewSys Research

# Last modified: 2014-02-20 <alm@slewsys.org>

# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.65])
AC_INIT([ed],[0.271-b8],[bug-ed@gnu.org])
AC_CONFIG_AUX_DIR([.])
AC_CONFIG_SRCDIR([config.h.in])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_LIBOBJ_DIR([lib])
AC_CONFIG_MACRO_DIR([m4])
AC_USE_SYSTEM_EXTENSIONS
AM_INIT_AUTOMAKE([gnits])

# Preserve configured options for displaying in --info output.
[ac_cv_configure_args=''
for ac_option in `sed -n                                                      \
  -e '/^.*configure@<:@ @:>@\{1,\}\(--.*\)$/s//\1/p'                          \
  -e '/^--/q' config.log`
do
  expr _"$ac_option" : '_-' >/dev/null 2>&1 &&
  ! expr _"$ac_option" : '_--no-create' >/dev/null 2>&1 &&
  ! expr _"$ac_option" : '_--no-recursion' >/dev/null 2>&1 &&
  ac_cv_configure_args="$ac_cv_configure_args  $ac_option\\n"
done]
if test ! _"$[ac_cv_configure_args]" = _''; then
  AC_DEFINE_UNQUOTED([CONFIGURE_ARGS],
    ["Configured options:\n$ac_cv_configure_args"],
    [Options with which "$top_srcdir"/configure was invoked.])
else
  AC_DEFINE_UNQUOTED([CONFIGURE_ARGS], [""],
    [Options with which "$top_srcdir"/configure was invoked.])
fi

# Darwin disables nested functions which are used by some AC tests.
if test _$[GCC] = _yes; then
  AC_CACHE_CHECK([whether gcc needs -fnested-functions],
    [ac_cv_gcc_cflags_nested_functions],
    AC_RUN_IFELSE([AC_LANG_SOURCE([
#include <stdio.h>

int
main ()
{
  int nested () { return 0; }

  nested ();
  return 0;
}
      ])],
      [ac_cv_gcc_cflags_nested_functions=no],
      [ac_cv_gcc_cflags_nested_functions=yes],
      [ac_cv_gcc_cflags_nested_functions=no]))
  if test _$[ac_cv_gcc_cflags_nested_functions] = _yes; then
    CFLAGS="$CFLAGS -fnested-functions"
  fi
fi

# Check function prototypes.
CFLAGS="$CFLAGS -Wimplicit-function-declaration"
export CFLAGS

#
# Checks for various programs.
#
AC_ARG_VAR([TROFF], [typesetting processor])
AC_PATH_PROG([TROFF], [troff])

AC_ARG_VAR([DPOST], [troff PostScript postprocessor])
AC_PATH_PROG([DPOST], [dpost], [/usr/lib/lp/postscript/dpost])

AC_ARG_VAR([GROFF], [GNU typesetting processor])
AC_PATH_PROG([GROFF], [groff])

AC_ARG_VAR([PS2PDF], [PostScript to PDF filter])
AC_PATH_PROGS([PS2PDF], [ps2pdf14 ps2pdf ps2pdfwr])

# Checks for prior deployment of this package - used by `upgrade-sh'
AC_CACHE_CHECK([for ed], [ac_cv_path_ED_COMMAND],
    [AC_PATH_PROGS_FEATURE_CHECK([ED_COMMAND], ["$ED" ed],
      [if $ac_path_ED_COMMAND --version 2>&1 |                                \
           grep -i '^ed ' >/dev/null 2>&1; then
         ac_cv_path_ED_COMMAND=$ac_path_ED_COMMAND
         ac_path_ED_COMMAND_found=:
       fi],
      [ac_cv_path_ED_COMMAND=./src/ed])])
AC_SUBST(ED_COMMAND, [$ac_cv_path_ED_COMMAND])

# Checks availability of GNU make
AC_CACHE_CHECK([for GNU make], [ac_cv_path_GNU_MAKE_COMMAND],
    [AC_PATH_PROGS_FEATURE_CHECK([GNU_MAKE_COMMAND],
    ["$MAKE" make gmake gnumake],
    [if $ac_path_GNU_MAKE_COMMAND --version 2>&1 |                            \
       grep -i 'gnu' >/dev/null 2>&1; then
         ac_cv_path_GNU_MAKE_COMMAND=$ac_path_GNU_MAKE_COMMAND
         ac_path_GNU_MAKE_COMMAND_found=:
       fi],
      [])])
AC_SUBST(GNU_MAKE_COMMAND, [$ac_cv_path_GNU_MAKE_COMMAND])

AC_SYS_LARGEFILE
AC_PROG_CC
AC_PROG_LN_S
AC_PROG_INSTALL
AC_PROG_MKDIR_P
AC_PROG_YACC

#
# Checks feature requests.
#
AC_MSG_CHECKING([whether included regex is requested])
AC_ARG_WITH([included-regex],
  [AS_HELP_STRING([--with-included-regex],
    [use GNU regex library included here])],[],
    [with_included_regex=check])
AC_MSG_RESULT([$with_included_regex])
AS_IF([test "_$with_included_regex" != _yes],
  [AC_CHECK_FUNCS([regcomp],[],
    [AC_MSG_WARN([System regex not found, falling back to included version])
     with_included_regex=yes])])
AM_CONDITIONAL([LIBADD_REGEX], [test "_$with_included_regex" = _yes])
AM_COND_IF([LIBADD_REGEX],
  [AC_DEFINE([HAVE_REG_SYNTAX_T], [1],
    [Define to 1 if regex.h defines `reg_syntax_t'.])],
  [AC_CHECK_TYPES([reg_syntax_t],
    [AC_DEFINE([HAVE_REG_SYNTAX_T], [1],
      [Define to 1 if regex.h defines `reg_syntax_t'.])],[],
      [[#include <sys/types.h>
#include <regex.h>]])])

AC_MSG_CHECKING([whether ED environment variable support is requested])
AC_ARG_ENABLE([ed-envar],
  [AS_HELP_STRING([--enable-ed-envar],
    [enable ED environment variable])],
  [AC_DEFINE([WANT_ED_ENVAR], [1],
    [Define to 1 to enable support for ED environment variable.])],[])
AC_MSG_RESULT([${enable_ed_envar:-no}])

AC_MSG_CHECKING([whether external filter support is requested])
AC_ARG_ENABLE([external-filter],
  [AS_HELP_STRING([--enable-external-filter],
    [enable external filtering of buffer])],
  [AC_DEFINE([WANT_EXTERNAL_FILTER], [1],
    [Define to 1 to enable modifying buffer via external filters.])],[])
AC_MSG_RESULT([${enable_external_filter:-no}])

AC_MSG_CHECKING([whether file glob support is requested])
AC_ARG_ENABLE([file-glob],
  [AS_HELP_STRING([--enable-file-glob],
    [enable file globbing and multi-file support])],
  [AC_DEFINE([WANT_FILE_GLOB], [1],
    [Define to 1 to enable file glob and multi-file support.])],[])
AC_MSG_RESULT([${enable_file_glob:-no}])

dnl AC_MSG_CHECKING([whether safe write support is requested])
dnl AC_ARG_ENABLE([safe-write],
dnl   [AS_HELP_STRING([--enable-safe-write],
dnl     [enable safe write])],
dnl   [AC_DEFINE([WANT_SAFE_WRITE], [1],
dnl     [Define to 1 to interpret `~w' as `w' followed by `~e'.])],[])
dnl AC_MSG_RESULT([${enable_safe_write:-no}])

AC_MSG_CHECKING([whether file lock support is requested])
AC_ARG_ENABLE([file-lock],
  [AS_HELP_STRING([--enable-file-lock],
    [enable file locking support])],[],[])
AC_MSG_RESULT([${enable_file_lock:-no}])
if test _$[enable_file_lock] = _yes; then
  if test _$[ac_cv_func_flock] = _no &&
    test _$[ac_cv_have_decl_f_setlk] = _no; then
    AC_MSG_WARN([Don't know how to set file locks; disabling])
  else
    AC_DEFINE([WANT_FILE_LOCK], [1],
     [Define to 1 to enable file lock support.])
  fi
fi

#
# Checks for libraries.
#
AC_CHECK_LIB([c], [fopen])

#
# Checks for headers.
#
AC_HEADER_STDBOOL
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_HEADER_TIOCGWINSZ
AC_CHECK_HEADERS_ONCE([fcntl.h glob.h libintl.h limits.h locale.h stdlib.h    \
  string.h strings.h sys/file.h sys/ioctl.h sys/param.h termios.h unistd.h    \
  wchar.h wctype.h])

#
# Checks for typedefs, structures, and compiler characteristics.
#
AC_C_CONST
AC_TYPE_PID_T
AC_TYPE_OFF_T
AC_TYPE_SIZE_T
AC_C_VOLATILE
AC_CHECK_TYPES([sig_atomic_t],
  [AC_DEFINE([HAVE_SIG_ATOMIC_T], [1],
     [Define to 1 to enable file lock support.])],
  [],
  [#include <signal.h>])
AC_CHECK_DECL([F_SETLK],
  [ac_cv_have_decl_f_setlk=yes],
  [ac_cv_have_decl_f_setlk=no],
  [#include <fcntl.h>])

# Checks storage size of size_t
AC_CACHE_CHECK([size of size_t],
 [ac_cv_size_t_size],
 AC_RUN_IFELSE([AC_LANG_SOURCE([
#include <limits.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>

int
main()
{
  _exit (sizeof (unsigned long long) - sizeof (size_t));
}
  ])],
  [ac_cv_size_t_size="unsigned long long"],
  [ac_cv_size_t_size="unsigned long"],
  [ac_cv_size_t_size="unsigned long"]))
AC_DEFINE_UNQUOTED([SIZE_T_SIZE], [$ac_cv_size_t_size],
  [Define int with storage size of size_t.])

# Checks storage size of off_t
AC_CACHE_CHECK([size of off_t],
 [ac_cv_off_t_size],
 AC_RUN_IFELSE([AC_LANG_SOURCE([
#include <limits.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>

int
main()
{
  _exit (sizeof (long long) - sizeof (off_t));
}
  ])],
  [ac_cv_off_t_size="long long"],
  [ac_cv_off_t_size="long"],
  [ac_cv_off_t_size="long"]))
AC_DEFINE_UNQUOTED([OFF_T_SIZE], [$ac_cv_off_t_size],
  [Define int with storage size of off_t.])

# Checks for string format to output value of off_t
AC_CACHE_CHECK([how to print value off_t],
 [ac_cv_off_t_format_string],
 AC_RUN_IFELSE([AC_LANG_SOURCE([
#include <limits.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>

int
main()
{
  _exit (sizeof (long long) - sizeof (off_t));
}
  ])],
  [ac_cv_off_t_format_string="lld"],
  [ac_cv_off_t_format_string="ld"],
  [ac_cv_off_t_format_string="ld"]))
AC_DEFINE_UNQUOTED([OFF_T_FORMAT_STRING], ["$ac_cv_off_t_format_string"],
  [Define format string to print value of type off_t.])


#
# Checks for system calls and library functions.
#
AC_CHECK_FUNC([getopt_long])
AM_CONDITIONAL([LIBADD_GETOPT_LONG],
    [test "_$ac_cv_func_getopt_long" = _no])

AC_PROG_LIBTOOL

AM_GNU_GETTEXT([no-libtool], [need-formatstring-macros], [../intl])
AM_GNU_GETTEXT_VERSION([0.18.3])
AM_CONDITIONAL([LDADD_LIBINTL], [test "_$LTLIBINTL" != _])
AM_COND_IF([LDADD_LIBINTL], [],
    [AM_GNU_GETTEXT([external])]
    :)

AC_FUNC_FORK
AC_FUNC_FSEEKO
AC_PROG_GCC_TRADITIONAL

# NB: malloc.c from GNU libc is problematic, e.g., for cross-compiling
AC_FUNC_MALLOC
AC_FUNC_REALLOC

AC_FUNC_MBRTOWC
AC_FUNC_SETVBUF_REVERSED
AC_DEFINE_UNQUOTED([RETSIGTYPE],[$ac_cv_type_signal],
  [Define as the return type of signal handlers (`int' or `void').])
AC_REPLACE_FUNCS([memset strspn strtok])

AC_CHECK_FUNCS_ONCE([dup2 flock ftruncate memmove pathconf sigaction          \
  siglongjmp setbuffer setlocale stat64 strchr strerror strtol                \
  strtoul strtoll])

# Checks for string format to output value of off_t
AC_CACHE_CHECK([how to output value of off_t],
 [ac_cv_off_t_format_string],
 AC_RUN_IFELSE([AC_LANG_SOURCE([
#include <stdio.h>
#include <sys/types.h>
#include <limits.h>

#ifndef OFF_T_MAX
# ifndef LLONG_MAX
#  define LLONG_MAX \
  ((long long) (~(unsigned long long) 0 >> (unsigned long long) 1))
#  define LLONG_MIN (-LLONG_MAX - 1)
# endif
# define OFF_T_MAX LLONG_MAX
# define OFF_T_MIN LLONG_MIN
#endif

int
main()
{
  exit (sizeof (long) < sizeof (OFF_T_MIN) /* && sizeof (int) < sizeof (long) */ ? 0 : 1);
}
  ])],
  [ac_cv_off_t_format_string="lld"],
  [ac_cv_off_t_format_string="ld"],
  [ac_cv_off_t_format_string="ld"]))
AC_DEFINE_UNQUOTED([OFF_T_FORMAT_STRING], ["$ac_cv_off_t_format_string"],
  [Define format string to output a value of type off_t.])

AM_CONDITIONAL([LDADD_LIBED],
  [test "_$LIBOBJS" != _ ||
   test "_$with_included_regex" = _yes ||
   test "_$ac_cv_func_getopt_long" = _no ||
   test "_$ac_cv_func_malloc_0_nonnull" = _no ||
   test "_$ac_cv_func_realloc_0_nonnull" = _no])

# Generate autotest helpers.
AC_CONFIG_TESTDIR([testsuite])
AC_CONFIG_FILES([testsuite/ed],
  [chmod 755 testsuite/ed])
AC_CONFIG_FILES([testsuite/ts-am-sw.ed],
  [chmod 755 testsuite/ts-am-sw.ed])

# Generate configure helper.
AM_MISSING_PROG([AUTOM4TE], [autom4te])

dnl intl/Makefile

# Generate Makefiles and conditional headers.
AC_CONFIG_FILES([Makefile                                                     \
  doc/Makefile                                                                \
  doc/bwk/Makefile                                                            \
  lib/Makefile                                                                \
  po/Makefile.in                                                              \
  po/ed.pot                                                                   \
  src/Makefile                                                                \
  testsuite/Makefile])

AC_OUTPUT

# Local variables:
# mode: autoconf
# eval: (add-hook 'write-file-functions 'time-stamp)
# time-stamp-start: "Last modified: "
# time-stamp-format: "%:y-%02m-%02d <%u@%h>"
# time-stamp-end: "$"
# End:
