# configure.ac: Configure autoconf template for the ed line editor.
#
# Copyright © 1993-2018 Andrew L. Moore, SlewSys Research
#
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([ed],[2.0.7],[bug-ed@gnu.org])
AC_CONFIG_AUX_DIR([.])
AC_CONFIG_SRCDIR([config.h.in])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_LIBOBJ_DIR([lib])
AC_CONFIG_MACRO_DIR([m4])
AC_USE_SYSTEM_EXTENSIONS
AM_INIT_AUTOMAKE([foreign])

# Capture configured options for displaying in --version output.
[ac_cv_configure_args=''
 while read line; do
    configure_args=`expr ."$line" : '^..*set X /bin/.*sh.*configure. \(.*\)$ac_configure_extra_args.*$'`
    test ."$configure_args" != .'' && break
 done <config.status
 for ac_option in $configure_args; do
   ac_cv_configure_args="$ac_cv_configure_args $ac_option\n"
 done
]

AS_IF([test ! ."$ac_cv_configure_args" = .''],
  AC_DEFINE_UNQUOTED([CONFIGURE_ARGS],
    ["Configured options:\n$ac_cv_configure_args"],
    [Options with which "$top_srcdir"/configure was invoked.]),
    AC_DEFINE_UNQUOTED([CONFIGURE_ARGS], [""],
      [Options with which "$top_srcdir"/configure was invoked.]))

# Darwin disables nested functions which are used by some AC tests.
AS_IF([test ."$GCC" = .yes],
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
      [ac_cv_gcc_cflags_nested_functions=no])))

AS_IF([test ."$ac_cv_gcc_cflags_nested_functions" = .'yes'],
    [CFLAGS="$CFLAGS -fnested-functions"])

# Check function prototypes.
CFLAGS="$CFLAGS -Wimplicit-function-declaration"
export CFLAGS

#
# Checks for various programs.
#
AC_ARG_VAR([ZCAT], [decompression filter])
AC_PATH_PROGS([ZCAT], [zcat gzcat])

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
    [case `$ac_path_ED_COMMAND --version 2>&1` in
       ed)
         ac_cv_path_ED_COMMAND=$ac_path_ED_COMMAND
         ac_path_ED_COMMAND_found=:
         ;;
     esac],
    [ac_cv_path_ED_COMMAND=./src/ed])])
AC_SUBST(ED_COMMAND, [$ac_cv_path_ED_COMMAND])

# Checks availability of GNU make
AX_CHECK_GNU_MAKE()
AS_IF([test ."$_cv_gnu_make_command" != .''],
   AC_SUBST(GNU_MAKE_COMMAND, [$_cv_gnu_make_command]))

AC_SYS_LARGEFILE

AC_PROG_CC
gl_EARLY

AC_PROG_LN_S
AC_PROG_INSTALL
AC_PROG_MKDIR_P
AC_PROG_YACC

AC_MSG_CHECKING([whether included regex is requested])
AC_ARG_WITH([included-regex],
  [AS_HELP_STRING([--with-included-regex],
    [use GNU regex library included here])],[],
    [with_included_regex=check])
AC_MSG_RESULT([$with_included_regex])

AS_IF([test ! ."$with_included_regex" = .'yes'],
  [AC_CHECK_FUNCS([regcomp],[with_included_regex=no],
    [AC_MSG_WARN([System regex not found, falling back to included version])
     with_included_regex=yes])])
AM_CONDITIONAL([LIBADD_REGEX], [test ."$with_included_regex" = .'yes'])
AM_COND_IF([LIBADD_REGEX],
  [AC_DEFINE([HAVE_REG_SYNTAX_T], [1],
    [Define to 1 if regex.h defines `reg_syntax_t'.])],
  [AC_CHECK_TYPES([reg_syntax_t],
    [AC_DEFINE([HAVE_REG_SYNTAX_T], [1],
      [Define to 1 if regex.h defines `reg_syntax_t'.])],[],
      [[#include <sys/types.h>
#include <regex.h>]])])

gl_INIT
AM_CONDITIONAL([LDADD_LIBGNU],
  [test ."$with_included_regex" = .'yes' \
    || test ."$ac_cv_func_getopt_long_only" = .'yes'])

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
AC_CHECK_HEADERS_ONCE([fcntl.h glob.h libintl.h limits.h locale.h \
  stddef.h stdlib.h string.h strings.h sys/file.h sys/ioctl.h sys/param.h \
  termios.h unistd.h wchar.h wctype.h])

#
# Checks for typedefs, structures, and compiler characteristics.
#
AC_C_CONST
AC_TYPE_PID_T
AC_TYPE_OFF_T
AC_TYPE_SIZE_T
AC_TYPE_LONG_LONG_INT
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

# Headers for compile checks.
headers='#ifdef HAVE_STDDEF_H
#include <stddef.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
'

# Checks storage sizes
AX_COMPILE_CHECK_SIZEOF(long)
AX_COMPILE_CHECK_SIZEOF(long long)
AX_COMPILE_CHECK_SIZEOF(size_t, $headers)
AX_COMPILE_CHECK_SIZEOF(off_t, $headers)

AS_IF([test ."$ac_cv_sizeof_size_t" = ."$ac_cv_sizeof_long"],
  AC_DEFINE_UNQUOTED([SIZE_T_SIZE], [unsigned long],
    [Define int with storage size of size_t.])
  AC_DEFINE_UNQUOTED([SIZE_T_FORMAT_STRING], ["lu"],
    [Define format string to print value of type size_t.]),
  AS_IF([test ."$ac_cv_sizeof_size_t" = ."$ac_cv_sizeof_long_long"],
    AC_DEFINE_UNQUOTED([SIZE_T_SIZE], [unsigned long long],
      [Define int with storage size of size_t.])
    AC_DEFINE_UNQUOTED([SIZE_T_FORMAT_STRING], ["llu"],
      [Define format string to print value of type size_t.]),
      AC_DEFINE_UNQUOTED([SIZE_T_SIZE], [unsigned long long],
       [Define int with storage size of size_t.])
      AC_DEFINE_UNQUOTED([SIZE_T_FORMAT_STRING], ["llu"],
       [Define format string to print value of type size_t.])))

AS_IF([test ."$ac_cv_sizeof_off_t" = ."$ac_cv_sizeof_long"],
  AC_DEFINE_UNQUOTED([OFF_T_SIZE], [long],
    [Define int with storage size of off_t.])
  AC_DEFINE_UNQUOTED([OFF_T_FORMAT_STRING], ["ld"],
    [Define format string to print value of type off_t.]),
  AS_IF([test ."$ac_cv_sizeof_off_t" = ."$ac_cv_sizeof_long_long"],
    AC_DEFINE_UNQUOTED([OFF_T_SIZE], [long long],
      [Define int with storage size of off_t.])
    AC_DEFINE_UNQUOTED([OFF_T_FORMAT_STRING], ["lld"],
      [Define format string to print value of type off_t.]),
      AC_DEFINE_UNQUOTED([OFF_T_SIZE], [long long],
        [Define int with storage size of size_t.])
      AC_DEFINE_UNQUOTED([OFF_T_FORMAT_STRING], ["lld"],
        [Define format string to print value of type off_t.])))

AC_PROG_LIBTOOL

AM_GNU_GETTEXT([no-libtool], [need-formatstring-macros], [../intl])
AM_GNU_GETTEXT_VERSION([0.19.8])
AM_CONDITIONAL([LDADD_LIBINTL], [test ."$LTLIBINTL" != .''])
AM_COND_IF([LDADD_LIBINTL], [],
    [AM_GNU_GETTEXT([external])]
    :)

AX_CHECK_OPENSSL([ac_cv_have_openssl='yes'])

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

#
# Checks feature requests.
#
AC_MSG_CHECKING([whether all ED extensions are requested])
AC_ARG_ENABLE([all-extensions],
  [AS_HELP_STRING([--enable-all-extensions],
    [enable all ED extensions.])], [], [])
AC_MSG_RESULT([${enable_all_extensions:-no}])

# Want all ED extensions...
AS_IF([test ."$enable_all_extensions" = .'yes'],
  [enable_ed_envar=yes]
  [enable_external_filter=yes]
  [enable_file_glob=yes]
  [enable_file_lock=yes]
  [enable_ed_macro=yes]
  [enable_ed_scrolling=yes]
  [enable_des_encryption=yes])

AC_MSG_CHECKING([whether ED environment variable support is requested])
AC_ARG_ENABLE([ed-envar],
  [AS_HELP_STRING([--enable-ed-envar],
    [enable ED environment variable])],
  [AC_DEFINE([WANT_ED_ENVAR], [1],
    [Define to 1 to enable support for ED environment variable.])],[])
AC_MSG_RESULT([${enable_ed_envar:-no}])

AC_MSG_CHECKING([whether ED external filter support is requested])
AC_ARG_ENABLE([external-filter],
  [AS_HELP_STRING([--enable-external-filter],
    [enable external filtering of ED buffer])],
  [AC_DEFINE([WANT_EXTERNAL_FILTER], [1],
    [Define to 1 to enable modifying ED buffer via external filters.])],[])
AC_MSG_RESULT([${enable_external_filter:-no}])

AC_MSG_CHECKING([whether ED file globbing support is requested])
AC_ARG_ENABLE([file-glob],
  [AS_HELP_STRING([--enable-file-glob],
    [enable ED file globbing and multi-file support])],
  [AC_DEFINE([WANT_FILE_GLOB], [1],
    [Define to 1 to enable ED file glob and multi-file support.])],[])
AC_MSG_RESULT([${enable_file_glob:-no}])

AC_MSG_CHECKING([whether ED file locking support is requested])
AC_ARG_ENABLE([file-lock],
  [AS_HELP_STRING([--enable-file-lock],
    [enable ED file locking support])],[],[])
AC_MSG_RESULT([${enable_file_lock:-no}])

# Enable file locking if requested, but only if available.
AS_IF([test ."$enable_file_lock" = .'yes' \
  &&  { test ! ."$ac_cv_func_flock" = .'no' \
    || test ! ."$ac_cv_have_decl_f_setlk" = .'no'; }],
  AC_DEFINE([WANT_FILE_LOCK], [1],
    [Define to 1 to enable ED file lock support.]), [])

AS_IF([test ."$enable_file_lock" = .'yes' \
  &&  { test ."$ac_cv_func_flock" = .'no' \
    && test ."$ac_cv_have_decl_f_setlk" != .'no'; }],
      [enable_file_lock=no]
      AC_MSG_WARN([Don't know how to set file locks; disabling]), [])

AC_MSG_CHECKING([whether ED macro support is requested])
AC_ARG_ENABLE([ed-macro],
  [AS_HELP_STRING([--enable-ed-macro],
    [enable ED macro support (implies registers and  script flags)])],
  [AC_DEFINE([WANT_ED_MACRO], [1],
    [Define to 1 to enable ED macro support.])],[])
AC_MSG_RESULT([${enable_ed_macro:-no}])

# Macro support implies register support and script flags.
AS_IF([test ."$enable_ed_macro" = .'yes'], [enable_ed_register=yes])
AS_IF([test ."$enable_ed_macro" = .'yes'], [enable_script_flags=yes])

# Override --disable-ed-register if macro support requested.
AC_MSG_CHECKING([whether ED register support is requested])
AC_ARG_ENABLE([ed-register],
  [AS_HELP_STRING([--enable-ed-register],
    [enable ED register support])],
  [AS_IF([test ."$enable_ed_register" = .'no' \
    && test ."$enable_ed_macro" = .'yes' ],
    [enable_ed_register=yes])]
  [AS_IF([test ."$enable_ed_register" = .'yes'],
    [AC_DEFINE([WANT_ED_REGISTER], [1],
      [Define to 1 to enable ED register support.])],[])])
AC_MSG_RESULT([${enable_ed_register:-no}])

# Override --disable-script-flags if macro support requested.
AC_MSG_CHECKING([whether ED script flags are requested])
AC_ARG_ENABLE([script-flags],
  [AS_HELP_STRING([--enable-script-flags],
    [enable ED command-line script flags])],
  [AS_IF([test ."$enable_script_flags" = .'no' \
    && test ."$enable_ed_macro" = .'yes' ],
    [enable_script_flags=yes])]
  [AS_IF([test ."$enable_script_flags" = .'yes'],
    [AC_DEFINE([WANT_SCRIPT_FLAGS], [1],
      [Define to 1 to enable ED command-line script flags.])],[])])
AC_MSG_RESULT([${enable_script_flags:-no}])

AC_MSG_CHECKING([whether ED scrolling support is requested])
AC_ARG_ENABLE([ed-scrolling],
  [AS_HELP_STRING([--enable-ed-scrolling],
    [enable ED scrolling.])],
  [AC_DEFINE([WANT_ED_SCROLLING], [1],
    [Define to 1 to enable ED scrolling support.])],[])
AC_MSG_RESULT([${enable_ed_scrolling:-no}])

AC_MSG_CHECKING([whether DES encryption support is requested])
AC_ARG_ENABLE([des-encryption],
  [AS_HELP_STRING([--enable-des-encryption],
    [enable DES encryption/decryption of files.])],[],[])
AC_MSG_RESULT([${enable_des_encryption:-no}])

AM_CONDITIONAL([LDADD_LIBCRYPT],
  [test ."$enable_des_encryption" = .'yes' \
    && test ."$ac_cv_have_openssl" = .'yes'])
AM_COND_IF([LDADD_LIBCRYPT],
  [AC_DEFINE([WANT_DES_ENCRYPTION], [1],
    [Define to 1 to enable DES encryption/decryption of files.])])


# Generate autotest helpers.
AC_CONFIG_TESTDIR([testsuite])
AC_CONFIG_FILES([testsuite/ed],
  [chmod 755 testsuite/ed])
AC_CONFIG_FILES([testsuite/ts-am-sw.ed],
  [chmod 755 testsuite/ts-am-sw.ed])

# Generate configure helper.
AM_MISSING_PROG([AUTOM4TE], [autom4te])

# Generate Makefiles and conditional headers.
AC_CONFIG_FILES([Makefile                                                     \
  doc/Makefile                                                                \
  doc/bwk/Makefile                                                            \
  lib/Makefile                                                                \
  po/Makefile.in                                                              \
  po/ed.pot                                                                   \
  src/Makefile                                                                \
  testsuite/Makefile
  testsuite/proto.ok-so-at.in])

AC_OUTPUT
