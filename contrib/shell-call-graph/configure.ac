# configure.ac: Configure autoconf template for the ed line editor.
#
# SPDX-FileCopyrightText: 1993-2024 Andrew L. Moore <slewsys@gmail.com>, SlewSys Research
#
# SPDX-License-Identifier: BSD-2-Clause OR GPL-2.0-or-later OR MIT
#
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([shell-call-graph],[1.0.1],[https://github.com/slewsys/ed/contrib/shell-call-graph])
AC_CONFIG_AUX_DIR([.])
AM_INIT_AUTOMAKE([foreign])

AC_PATH_PROGS([CAT_CMD], [gcat cat])

AC_PATH_PROG([ED_CMD], [ed])
AC_PATH_PROG([FILE_CMD], [file])

AC_PATH_PROGS([GREP_CMD], [ggrep grep])
AC_PATH_PROGS([MKTEMP_CMD], [gmktemp mktemp])

AC_PATH_PROG([PERL_CMD], [perl])
AC_PATH_PROG([PRDG_CMD], [prdg],             [], [${PATH}${PATH_SEPARATOR}../prdg])

AC_PATH_PROGS([READLINK_CMD], [greadlink readlink])


AC_PATH_PROGS([RM_CMD], [grm rm])
AC_PATH_PROGS([SED_CMD], [gsed sed])

if ! $ED_CMD --version 2>&1 | $GREP_CMD -q '^Configuration:'; then
   AC_MSG_ERROR([Missing program ed. Please install before proceeding.])
fi
if test ."$PRDG_CMD" = .''; then
   AC_MSG_ERROR([Missing program prdg. Please install before proceeding.])
fi

AM_MISSING_PROG([AUTOM4TE], [autom4te])

AC_CONFIG_FILES([Makefile])
AC_CONFIG_FILES([shell-call-graph], [chmod +x shell-call-graph])

AC_OUTPUT
