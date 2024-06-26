# configure.ac: Configure autoconf template for the ed line editor.
#
# SPDX-FileCopyrightText: 1993-2024 Andrew L. Moore <slewsys@gmail.com>, SlewSys Research
#
# SPDX-License-Identifier: BSD-2-Clause OR GPL-2.0-or-later OR MIT
#
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([ed],[2.1.0],[https://github.com/slewsys/ed])
AC_CONFIG_AUX_DIR([.])
AC_CONFIG_SRCDIR([config.h.in])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_LIBOBJ_DIR([lib])
AC_CONFIG_MACRO_DIR([m4])
AC_USE_SYSTEM_EXTENSIONS
AM_INIT_AUTOMAKE([foreign])
LT_PREREQ([2.4.6])
LT_INIT([])
AC_SUBST([LIBTOOL_DEPS])

# Check function prototypes.
CFLAGS="$CFLAGS -Wimplicit-function-declaration"
export CFLAGS

#
# Checks for various programs.
#
AC_ARG_VAR([ZSTD_CMD], [decompression filter])
AC_PATH_PROG([ZSTD_CMD], [zstd])

AC_ARG_VAR([TROFF], [typesetting processor])
AC_PATH_PROG([TROFF], [troff])

AC_ARG_VAR([DPOST], [troff PostScript postprocessor])
AC_PATH_PROG([DPOST], [dpost], [/usr/lib/lp/postscript/dpost])

AC_ARG_VAR([GROFF], [GNU typesetting processor])
AC_PATH_PROG([GROFF], [groff])

AC_ARG_VAR([PS2PDF], [PostScript to PDF filter])
AC_PATH_PROGS([PS2PDF], [ps2pdf14 ps2pdf ps2pdfwr])

AC_ARG_VAR([MAKEINFO], [Texinfo translation filter])
AC_PATH_PROGS([MAKEINFO], [makeinfo texi2any])
AM_CONDITIONAL([HAVE_MAKEINFO], [test ! ."$ac_cv_path_MAKEINFO" = .''])

AC_ARG_VAR([TEXI2DVI], [Texinfo to PDF filter])
AC_PATH_PROG([TEXI2DVI], [texi2dvi])
AM_CONDITIONAL([HAVE_TEXI2DVI], [test ! ."$ac_cv_path_TEXI2DVI" = .''])

# Checks availability of GNU make
AX_CHECK_GNU_MAKE()
if test ."$_cv_gnu_make_command" != .''; then
    AC_SUBST(GNU_MAKE_COMMAND, [$_cv_gnu_make_command])
fi

AC_SYS_LARGEFILE

AC_PROG_CC
gl_EARLY

AC_PROG_LN_S
AC_PROG_INSTALL
AC_PROG_MKDIR_P
AC_PROG_YACC

# Enable memory/heap diagnostics and profiling.
AX_VALGRIND_DFLT([helgrind], [off])
AX_VALGRIND_DFLT([drd], [off])
AX_VALGRIND_CHECK

AC_MSG_CHECKING([whether included regex is requested])
AC_ARG_WITH([included-regex],
  [AS_HELP_STRING([--with-included-regex],
    [use GNU regex library included here])],
    [], [with_included_regex=check])
if test ! ."$with_included_regex" = .'yes'; then
    AC_CHECK_FUNCS([regcomp],
      [with_included_regex=no],
      [with_included_regex=yes
       AC_MSG_WARN([System regex not found, falling back to included version])])
fi
AC_MSG_RESULT([$with_included_regex])
if test ."$with_included_regex" = .'yes'; then
   ln -sf POTFILES-regex.in po/POTFILES.in
else
   ln -sf POTFILES-noregex.in po/POTFILES.in
fi
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
AM_CONDITIONAL([LDADD_LIBGNU], [true])
  dnl [test ."$with_included_regex" = .'yes' \
  dnl   || test ."$ac_cv_func_getopt_long_only" = .'yes'])

#
# Checks for libraries.
#
AC_CHECK_LIB([c], [fopen])
AC_CHECK_LIB([bsd], [arc4random_buf],
  [ac_cv_lib_bsd='yes'], [ac_cv_lib_bsd='no'])
AM_CONDITIONAL([LDADD_LIBBSD], [test ."$ac_cv_lib_bsd" = .'yes'])
AM_COND_IF([LDADD_LIBBSD],
  [AC_DEFINE([HAVE_LIB_BSD], [1],
    [Define to 1 if libbsd is present])])

#
# Checks for headers.
#
AC_HEADER_STDBOOL
AC_CHECK_INCLUDES_DEFAULT
AC_PROG_EGREP

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

if test ."$ac_cv_sizeof_size_t" = ."$ac_cv_sizeof_long_long"; then
    AC_DEFINE_UNQUOTED([SIZE_T_SIZE], [unsigned long long],
      [Define int with storage size of size_t.])
    AC_DEFINE_UNQUOTED([SIZE_T_FORMAT_STRING], ["llu"],
      [Define format string to print value of type size_t.])
else
    AC_DEFINE_UNQUOTED([SIZE_T_SIZE], [unsigned long],
      [Define int with storage size of size_t.])
    AC_DEFINE_UNQUOTED([SIZE_T_FORMAT_STRING], ["lu"],
      [Define format string to print value of type size_t.])
fi

case `$CC -dumpmachine | cut -d- -f1` in
    aarch64|x86_64)
        if test ."$ac_cv_sizeof_off_t" = ."$ac_cv_sizeof_long"; then
            AC_DEFINE_UNQUOTED([OFF_T_SIZE], [long],
              [Define int with storage size of off_t.])
            AC_DEFINE_UNQUOTED([OFF_T_FORMAT_STRING], ["ld"],
              [Define format string to print value of type off_t.])
        else
            AC_DEFINE_UNQUOTED([OFF_T_SIZE], [long long],
              [Define int with storage size of off_t.])
            AC_DEFINE_UNQUOTED([OFF_T_FORMAT_STRING], ["lld"],
              [Define format string to print value of type off_t.])
        fi
        ;;
    *)
        if test ."$ac_cv_sizeof_off_t" = ."$ac_cv_sizeof_long_long"; then
            AC_DEFINE_UNQUOTED([OFF_T_SIZE], [long long],
              [Define int with storage size of off_t.])
            AC_DEFINE_UNQUOTED([OFF_T_FORMAT_STRING], ["lld"],
              [Define format string to print value of type off_t.])
        else
            AC_DEFINE_UNQUOTED([OFF_T_SIZE], [long],
              [Define int with storage size of off_t.])
            AC_DEFINE_UNQUOTED([OFF_T_FORMAT_STRING], ["ld"],
              [Define format string to print value of type off_t.])
        fi
        ;;
esac
LT_INIT

AM_GNU_GETTEXT([external], [need-formatstring-macros])
AM_CONDITIONAL([LDADD_LIBINTL], [test ."$LTLIBINTL" != .''])
AM_GNU_GETTEXT_VERSION([0.21])

AX_CHECK_OPENSSL([ac_cv_have_openssl='yes'])

AC_FUNC_FORK
AC_FUNC_VFORK
AC_FUNC_FSEEKO
AC_PROG_GCC_TRADITIONAL

# NB: malloc.c from GNU libc is problematic, e.g., for cross-compiling
AC_FUNC_MALLOC
AC_FUNC_REALLOC

AC_FUNC_MBRTOWC
AC_DEFINE_UNQUOTED([RETSIGTYPE],[$ac_cv_type_signal],
  [Define as the return type of signal handlers (`int' or `void').])
AC_REPLACE_FUNCS([memset strspn strtok])

AC_CHECK_FUNCS_ONCE([arc4random_buf dup2 flock ftruncate mkstemp memmove pathconf \
  sigaction siglongjmp setbuffer setlocale stat64 strchr strerror strtol \
  strtoul strtoll])

#
# Checks feature requests.
#
AC_MSG_CHECKING([whether all ed extensions are requested])
AC_ARG_ENABLE([all-extensions],
  [AS_HELP_STRING([--enable-all-extensions],
    [enable all ed extensions.])], [], [])
AC_MSG_RESULT([${enable_all_extensions:-no}])

AC_MSG_CHECKING([whether ed command-line address argument support is requested])
AC_ARG_ENABLE([address-arguments],
  [AS_HELP_STRING([--enable-address-arguments],
    [enable command-line address arguments])], [], [])
if test ."$enable_address_arguments" = .'yes' \
        || { test ! ."$enable_address_arguments" = .'no' \
                  && test ."$enable_all_extensions" = .'yes'; }; then
    enable_address_arguments=yes
    AC_DEFINE([WANT_ADDRESS_ARGUMENTS], [1],
      [Define to 1 to enable support for command-line address arguments.])
fi
AC_MSG_RESULT([${enable_address_arguments:-no}])

AC_MSG_CHECKING([whether ED environment variable support is requested])
AC_ARG_ENABLE([ed-envar],
  [AS_HELP_STRING([--enable-ed-envar],
    [enable ED environment variable])], [], [])
if test ."$enable_ed_envar" = .'yes' \
        || { test ! ."$enable_ed_envar" = .'no' \
                  && test ."$enable_all_extensions" = .'yes'; }; then
    enable_ed_envar=yes
    AC_DEFINE([WANT_ED_ENVAR], [1],
      [Define to 1 to enable support for ED environment variable.])
fi
AC_MSG_RESULT([${enable_ed_envar:-no}])

AC_MSG_CHECKING([whether ed external filter support is requested])
AC_ARG_ENABLE([external-filter],
  [AS_HELP_STRING([--enable-external-filter],
    [enable external filtering of ed's buffer])], [], [])
if test ."$enable_external_filter" = .'yes' \
        || { test ! ."$enable_external_filter" = .'no' \
                  && test ."$enable_all_extensions" = .'yes'; }; then
    enable_external_filter=yes
    AC_DEFINE([WANT_EXTERNAL_FILTER], [1],
      [Define to 1 to enable modifying ed buffer via external filters.])
fi
AC_MSG_RESULT([${enable_external_filter:-no}])

AC_MSG_CHECKING([whether ed file globbing support is requested])
AC_ARG_ENABLE([file-glob],
  [AS_HELP_STRING([--enable-file-glob],
    [enable ed file globbing and multi-file support])], [], [])
if test ."$enable_file_glob" = .'yes' \
        || { test ! ."$enable_file_glob" = .'no' \
                  && test ."$enable_all_extensions" = .'yes'; }; then
    enable_file_glob=yes
    AC_DEFINE([WANT_FILE_GLOB], [1],
      [Define to 1 to enable ed file glob and multi-file support.])
fi
AC_MSG_RESULT([${enable_file_glob:-no}])

AC_MSG_CHECKING([whether ed file locking support is requested])
AC_ARG_ENABLE([file-lock],
  [AS_HELP_STRING([--enable-file-lock],
    [enable ed file locking support])], [], [])

# Enable file locking if requested, but only if available.
if test ."$enable_file_lock" = .'yes' \
        || { test ! ."$enable_file_lock" = .'no' \
                  && test ."$enable_all_extensions" = .'yes'; }; then
     AS_IF(
       [test ! ."$ac_cv_func_flock" = .'no' \
             || test ! ."$ac_cv_have_decl_f_setlk" = .'no'],
         [enable_file_lock=yes
          AC_DEFINE([WANT_FILE_LOCK], [1],
            [Define to 1 to enable ed file lock support.])],
       [test ."$ac_cv_func_flock" = .'no' \
          && test ."$ac_cv_have_decl_f_setlk" = .'no'],
         [enable_file_lock=no
          AC_MSG_WARN([Missing dependencies; disabling file lock])])
fi
AC_MSG_RESULT([${enable_file_lock:-no}])

AC_MSG_CHECKING([whether ed macro support is requested])
AC_ARG_ENABLE([ed-macro],
  [AS_HELP_STRING([--enable-ed-macro],
    [enable ed macro support (implies register and  script flags)])], [], [])
if test ."$enable_ed_macro" = .'yes' \
        || { test ! ."$enable_ed_macro" = .'no' \
                  && test ."$enable_all_extensions" = .'yes'; }; then
    enable_ed_macro=yes
    AC_DEFINE([WANT_ED_MACRO], [1],
      [Define to 1 to enable ed macro support.])
fi
AC_MSG_RESULT([${enable_ed_macro:-no}])

AC_MSG_CHECKING([whether ed register support is requested])
AC_ARG_ENABLE([ed-register],
  [AS_HELP_STRING([--enable-ed-register],
    [enable ed register support])], [] [])

# Override --disable-ed-register if macro support requested.
if test ! ."$enable_ed_register" = .'yes' \
        && test ! ."$enable_all_extensions" = .'yes' \
                && test ."$enable_ed_macro" = .'yes'; then
    enable_ed_register=yes
    AC_MSG_WARN([Macros require register support; enabling registers])
fi
if test ."$enable_ed_register" = .'yes' \
        || { test ! ."$enable_ed_register" = .'no' \
                  && test ."$enable_all_extensions" = .'yes'; }; then
    enable_ed_register=yes
    AC_DEFINE([WANT_ED_REGISTER], [1],
      [Define to 1 to enable ed register support.])
fi
AC_MSG_RESULT([${enable_ed_register:-no}])

AC_MSG_CHECKING([whether ed script flags are requested])
AC_ARG_ENABLE([script-flags],
  [AS_HELP_STRING([--enable-script-flags],
    [enable ed command-line script flags])], [], [])

# Override --disable-script-flags if macro support requested.
if test ! ."$enable_script_flags" = .'yes' \
        && test ! ."$enable_all_extensions" = .'yes' \
                && test ."$enable_ed_macro" = .'yes'; then
    enable_script_flags=yes
    AC_MSG_WARN([Macros require script-flags support; enabling script flags])
fi
if test ."$enable_script_flags" = .'yes' \
        || { test ! ."$enable_script_flags" = .'no' \
                  && test ."$enable_all_extensions" = .'yes'; }; then
    enable_script_flags=yes
    AC_DEFINE([WANT_SCRIPT_FLAGS], [1],
      [Define to 1 to enable ed command-line script flags.])
fi
AC_MSG_RESULT([${enable_script_flags:-no}])

AC_MSG_CHECKING([whether ed scrolling support is requested])
AC_ARG_ENABLE([ed-scroll],
  [AS_HELP_STRING([--enable-ed-scroll],
    [enable ed scrolling.])], [], [])
if test ."$enable_ed_scroll" = .'yes' \
        || { test ! ."$enable_ed_scroll" = .'no' \
                  && test ."$enable_all_extensions" = .'yes'; }; then
    enable_ed_scroll=yes
    AC_DEFINE([WANT_ED_SCROLL], [1],
      [Define to 1 to enable ed scrolling support.])
fi
AC_MSG_RESULT([${enable_ed_scroll:-no}])

AC_MSG_CHECKING([whether ed encryption support is requested])
AC_ARG_ENABLE([ed-encryption],
  [AS_HELP_STRING([--enable-ed-encryption],
    [enable ed encryption/decryption of files.])], [], [])
if test ."$enable_ed_encryption" = .'yes' \
        || { test ! ."$enable_ed_encryption" = .'no' \
                  && test ."$enable_all_extensions" = .'yes'; }; then
    if test ."$ac_cv_have_openssl" = .'no' \
            || { test ."$ac_cv_func_arc4random_buf" = .'no' \
                      && test ."$ac_cv_lib_bsd" = .'no'; }; then
        enable_ed_encryption=no
        AC_MSG_WARN([Missing dependencies; disabling encryption])
    else
        enable_ed_encryption=yes
    fi
fi
AC_MSG_RESULT([${enable_ed_encryption:-no}])

AM_CONDITIONAL([LDADD_LIBCRYPT], [test ."$enable_ed_encryption" = .'yes'])

AM_COND_IF([LDADD_LIBCRYPT],
  [AC_DEFINE([WANT_ED_ENCRYPTION], [1],
    [Define to 1 to enable ed encryption/decryption of files.])])

# Capture configured options for displaying in --version output.
[configure_options='
  enable_address_arguments
  enable_ed_encryption
  enable_ed_envar
  enable_ed_macro
  enable_ed_register
  enable_ed_scroll
  enable_external_filter
  enable_file_glob
  enable_file_lock
  enable_script_flags
  with_included_regex'

  ac_cv_configure_args=''
  for ac_var in $configure_options; do
    eval ac_value=\$$ac_var
    if test ."$ac_value" = .'yes'; then
      ac_option="--`echo $ac_var | tr _ -`"
      ac_cv_configure_args="$ac_cv_configure_args $ac_option\n"
    fi
  done]

if test ."$ac_cv_configure_args" = .''; then
    AC_DEFINE_UNQUOTED([CONFIGURE_ARGS], [""],
      [Options with which "${top_srcdir}/configure" was invoked.])
else
    AC_DEFINE_UNQUOTED([CONFIGURE_ARGS],
      ["Configuration:\n$ac_cv_configure_args"],
      [Options with which "${top_srcdir}/configure" was invoked.])
fi

# Generate autotest helpers.
AC_CONFIG_TESTDIR([testsuite])
AC_CONFIG_FILES([testsuite/ed], [chmod 755 testsuite/ed])

# Generate configure helper.
AM_MISSING_PROG([AUTOM4TE], [autom4te])

# Generate Makefiles and conditional headers.
AC_CONFIG_FILES([                                                             \
  Makefile                                                                    \
  doc/Makefile                                                                \
  doc/bwk/Makefile                                                            \
  lib/Makefile                                                                \
  po/Makefile.in                                                              \
  src/Makefile                                                                \
  testsuite/Makefile                                                          \
  testsuite/proto.ok-so-at.in])

AC_OUTPUT
