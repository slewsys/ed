#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([prdg], [1.0.1], [https://github.com/slewsys/ed/contrib/prdg])
AC_CONFIG_SRCDIR([prdg.h])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_AUX_DIR([.])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL

# Enable memory/heap diagnostics and profiling.
AX_VALGRIND_DFLT([helgrind], [off])
AX_VALGRIND_DFLT([drd], [off])
AX_VALGRIND_CHECK

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([unistd.h])
AC_CHECK_HEADERS([sys/param.h])

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([strerror])

AC_MSG_CHECKING([whether to search optional path])
AC_ARG_WITH([opt-path],
  [AS_HELP_STRING([--with-opt-path@<:@=PATH@:>@],
    [search optional PATH @{:@default: /opt/local/bin@:}@])],
  [], [with_opt_path=check])
if test ."$with_opt_path" = .'yes' || test ."$with_opt_path" = .'check'; then
    if test -d /opt/local/bin; then
        OPT_PREFIX=/opt/local/sbin${PATH_SEPARATOR}/opt/local/bin
    fi
elif test ! ."$with_opt_path" = .'no'; then
    if test -d "$with_opt_path"; then
        OPT_PREFIX=${with_opt_path}
    else
        AC_MSG_WARN([${with_opt_path}: No such file or directory])
    fi
fi
AC_MSG_RESULT([${OPT_PREFIX:-no}])
OPT_PATHS=$(echo "$OPT_PREFIX" | tr "${PATH_SEPARATOR}" ' ')
AC_SUBST([OPT_PATHS], ["$OPT_PATHS"])
OPT_PREFIX=${OPT_PREFIX:+"${OPT_PREFIX}${PATH_SEPARATOR}"}

AC_PATH_PROGS([AWK_CMD],        [gawk awk],           [], [${OPT_PREFIX}${PATH}])
AC_PATH_PROGS([GREP_CMD],       [ggrep grep],         [], [${OPT_PREFIX}${PATH}])
AC_PATH_PROGS([READLINK_CMD],   [greadlink readlink], [], [${OPT_PREFIX}${PATH}])
AC_PATH_PROGS([SED_CMD],        [gsed sed],           [], [${OPT_PREFIX}${PATH}])
AC_PATH_PROGS([SORT_CMD],       [gsort sort],         [], [${OPT_PREFIX}${PATH}])
AC_PATH_PROGS([TEE_CMD],        [gtee tee],           [], [${OPT_PREFIX}${PATH}])

AC_CONFIG_FILES([Makefile])
AC_CONFIG_FILES([testsuite/generate-random-graph], [chmod +x testsuite/generate-random-graph])
AC_OUTPUT
