# DO NOT EDIT! GENERATED AUTOMATICALLY!
# Copyright (C) 2002-2021 Free Software Foundation, Inc.
#
# This file is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# This file is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this file.  If not, see <https://www.gnu.org/licenses/>.
#
# As a special exception to the GNU General Public License,
# this file may be distributed as part of a program that
# contains a configuration script generated by Autoconf, under
# the same distribution terms as the rest of that program.
#
# Generated by gnulib-tool.
#
# This file represents the compiled summary of the specification in
# gnulib-cache.m4. It lists the computed macro invocations that need
# to be invoked from configure.ac.
# In projects that use version control, this file can be treated like
# other built files.


# This macro should be invoked from ./configure.ac, in the section
# "Checks for programs", right after AC_PROG_CC, and certainly before
# any checks for libraries, header files, types and library functions.
AC_DEFUN([gl_EARLY],
[
  m4_pattern_forbid([^gl_[A-Z]])dnl the gnulib macro namespace
  m4_pattern_allow([^gl_ES$])dnl a valid locale name
  m4_pattern_allow([^gl_LIBOBJS$])dnl a variable
  m4_pattern_allow([^gl_LTLIBOBJS$])dnl a variable

  # Pre-early section.
  AC_REQUIRE([gl_USE_SYSTEM_EXTENSIONS])
  AC_REQUIRE([gl_PROG_AR_RANLIB])

  AC_REQUIRE([AM_PROG_CC_C_O])
  # Code from module absolute-header:
  # Code from module attribute:
  # Code from module btowc:
  # Code from module builtin-expect:
  # Code from module c99:
  # Code from module dynarray:
  # Code from module extensions:
  # Code from module extern-inline:
  # Code from module gen-header:
  # Code from module getopt-gnu:
  # Code from module getopt-posix:
  # Code from module gettext-h:
  # Code from module hard-locale:
  # Code from module include_next:
  # Code from module intprops:
  # Code from module inttypes-incomplete:
  # Code from module langinfo:
  # Code from module libc-config:
  # Code from module limits-h:
  # Code from module localcharset:
  # Code from module locale:
  # Code from module localeconv:
  # Code from module lock:
  # Code from module mbrtowc:
  # Code from module mbsinit:
  # Code from module mbtowc:
  # Code from module multiarch:
  # Code from module nl_langinfo:
  # Code from module nocrash:
  # Code from module regex:
  # Code from module setlocale-null:
  # Code from module snippet/_Noreturn:
  # Code from module snippet/arg-nonnull:
  # Code from module snippet/c++defs:
  # Code from module snippet/warn-on-use:
  # Code from module ssize_t:
  # Code from module std-gnu11:
  # Code from module stdbool:
  # Code from module stddef:
  # Code from module stdint:
  # Code from module stdlib:
  # Code from module streq:
  # Code from module sys_types:
  # Code from module threadlib:
  gl_THREADLIB_EARLY
  # Code from module unistd:
  # Code from module vararrays:
  # Code from module verify:
  # Code from module wchar:
  # Code from module wcrtomb:
  # Code from module wctype-h:
  # Code from module windows-mutex:
  # Code from module windows-once:
  # Code from module windows-recmutex:
  # Code from module windows-rwlock:
])

# This macro should be invoked from ./configure.ac, in the section
# "Check for header files, types and library functions".
AC_DEFUN([gl_INIT],
[
  AM_CONDITIONAL([GL_COND_LIBTOOL], [false])
  gl_cond_libtool=false
  gl_libdeps=
  gl_ltlibdeps=
  gl_m4_base='m4'
  m4_pushdef([AC_LIBOBJ], m4_defn([gl_LIBOBJ]))
  m4_pushdef([AC_REPLACE_FUNCS], m4_defn([gl_REPLACE_FUNCS]))
  m4_pushdef([AC_LIBSOURCES], m4_defn([gl_LIBSOURCES]))
  m4_pushdef([gl_LIBSOURCES_LIST], [])
  m4_pushdef([gl_LIBSOURCES_DIR], [])
  m4_pushdef([GL_MACRO_PREFIX], [gl])
  m4_pushdef([GL_MODULE_INDICATOR_PREFIX], [GL])
  gl_COMMON
  gl_source_base='lib'
  gl_source_base_prefix=
  AC_REQUIRE([gl_EXTERN_INLINE])
  gl_FUNC_GETOPT_GNU
  dnl Because of the way gl_FUNC_GETOPT_GNU is implemented (the gl_getopt_required
  dnl mechanism), there is no need to do any AC_LIBOBJ or AC_SUBST here; they are
  dnl done in the getopt-posix module.
  gl_FUNC_GETOPT_POSIX
  gl_CONDITIONAL_HEADER([getopt.h])
  gl_CONDITIONAL_HEADER([getopt-cdefs.h])
  AC_PROG_MKDIR_P
  if test $REPLACE_GETOPT = 1; then
    AC_LIBOBJ([getopt])
    AC_LIBOBJ([getopt1])
    dnl Define the substituted variable GNULIB_UNISTD_H_GETOPT to 1.
    gl_UNISTD_H_REQUIRE_DEFAULTS
    gl_MODULE_INDICATOR_INIT_VARIABLE([GNULIB_UNISTD_H_GETOPT], [1])
  fi
  gl_UNISTD_MODULE_INDICATOR([getopt-posix])
  gl_LIMITS_H
  gl_CONDITIONAL_HEADER([limits.h])
  AC_PROG_MKDIR_P
  gl_MULTIARCH
  gl_REGEX
  if test $ac_use_included_regex = yes; then
    AC_LIBOBJ([regex])
    gl_PREREQ_REGEX
  fi
  gt_TYPE_SSIZE_T
  gl_STDBOOL_H
  gl_CONDITIONAL_HEADER([stdbool.h])
  AC_PROG_MKDIR_P
  gl_STDDEF_H
  gl_STDDEF_H_REQUIRE_DEFAULTS
  gl_CONDITIONAL_HEADER([stddef.h])
  AC_PROG_MKDIR_P
  gl_STDINT_H
  gl_CONDITIONAL_HEADER([stdint.h])
  dnl Because of gl_REPLACE_LIMITS_H:
  gl_CONDITIONAL_HEADER([limits.h])
  AC_PROG_MKDIR_P
  gl_SYS_TYPES_H
  gl_SYS_TYPES_H_REQUIRE_DEFAULTS
  AC_PROG_MKDIR_P
  gl_UNISTD_H
  gl_UNISTD_H_REQUIRE_DEFAULTS
  AC_PROG_MKDIR_P
  AC_C_VARARRAYS
  gl_gnulib_enabled_attribute=false
  gl_gnulib_enabled_btowc=false
  gl_gnulib_enabled_37f71b604aa9c54446783d80f42fe547=false
  gl_gnulib_enabled_dynarray=false
  gl_gnulib_enabled_be453cec5eecf5731a274f2de7f2db36=false
  gl_gnulib_enabled_30838f5439487421042f2225bed3af76=false
  gl_gnulib_enabled_intprops=false
  gl_gnulib_enabled_67241cf2e9b9409f43aeebaa5c55e7f2=false
  gl_gnulib_enabled_langinfo=false
  gl_gnulib_enabled_21ee726a3540c09237a8e70c0baf7467=false
  gl_gnulib_enabled_localcharset=false
  gl_gnulib_enabled_locale=false
  gl_gnulib_enabled_localeconv=false
  gl_gnulib_enabled_lock=false
  gl_gnulib_enabled_mbrtowc=false
  gl_gnulib_enabled_mbsinit=false
  gl_gnulib_enabled_mbtowc=false
  gl_gnulib_enabled_nl_langinfo=false
  gl_gnulib_enabled_e7e881d32ca02f1c997b13c737c64bbd=false
  gl_gnulib_enabled_b3ae4a413a1340415f34a52d1dafb147=false
  gl_gnulib_enabled_stdlib=false
  gl_gnulib_enabled_streq=false
  gl_gnulib_enabled_threadlib=false
  gl_gnulib_enabled_verify=false
  gl_gnulib_enabled_wchar=false
  gl_gnulib_enabled_wcrtomb=false
  gl_gnulib_enabled_3dcce957eadc896e63ab5f137947b410=false
  gl_gnulib_enabled_503a4cb75d69c787103d0aa2ab7d8440=false
  gl_gnulib_enabled_68a4501daeca58988392c7e60b4917ab=false
  gl_gnulib_enabled_f0efff84a70f4afba30902bb8ffe9354=false
  gl_gnulib_enabled_8bb827fe37eaccf1b97feb0c87bc92ef=false
  func_gl_gnulib_m4code_attribute ()
  {
    if ! $gl_gnulib_enabled_attribute; then
      gl_gnulib_enabled_attribute=true
    fi
  }
  func_gl_gnulib_m4code_btowc ()
  {
    if ! $gl_gnulib_enabled_btowc; then
      gl_FUNC_BTOWC
      if test $HAVE_BTOWC = 0 || test $REPLACE_BTOWC = 1; then
        AC_LIBOBJ([btowc])
        gl_PREREQ_BTOWC
      fi
      gl_WCHAR_MODULE_INDICATOR([btowc])
      gl_gnulib_enabled_btowc=true
      if test $HAVE_BTOWC = 0 || test $REPLACE_BTOWC = 1; then
        func_gl_gnulib_m4code_mbtowc
      fi
      func_gl_gnulib_m4code_wchar
    fi
  }
  func_gl_gnulib_m4code_37f71b604aa9c54446783d80f42fe547 ()
  {
    if ! $gl_gnulib_enabled_37f71b604aa9c54446783d80f42fe547; then
      gl___BUILTIN_EXPECT
      gl_gnulib_enabled_37f71b604aa9c54446783d80f42fe547=true
    fi
  }
  func_gl_gnulib_m4code_dynarray ()
  {
    if ! $gl_gnulib_enabled_dynarray; then
      AC_PROG_MKDIR_P
      gl_gnulib_enabled_dynarray=true
      func_gl_gnulib_m4code_37f71b604aa9c54446783d80f42fe547
      func_gl_gnulib_m4code_intprops
      func_gl_gnulib_m4code_21ee726a3540c09237a8e70c0baf7467
    fi
  }
  func_gl_gnulib_m4code_be453cec5eecf5731a274f2de7f2db36 ()
  {
    if ! $gl_gnulib_enabled_be453cec5eecf5731a274f2de7f2db36; then
      AC_SUBST([LIBINTL])
      AC_SUBST([LTLIBINTL])
      gl_gnulib_enabled_be453cec5eecf5731a274f2de7f2db36=true
    fi
  }
  func_gl_gnulib_m4code_30838f5439487421042f2225bed3af76 ()
  {
    if ! $gl_gnulib_enabled_30838f5439487421042f2225bed3af76; then
      AC_REQUIRE([gl_FUNC_SETLOCALE_NULL])
      LIB_HARD_LOCALE="$LIB_SETLOCALE_NULL"
      AC_SUBST([LIB_HARD_LOCALE])
      gl_gnulib_enabled_30838f5439487421042f2225bed3af76=true
      func_gl_gnulib_m4code_e7e881d32ca02f1c997b13c737c64bbd
    fi
  }
  func_gl_gnulib_m4code_intprops ()
  {
    if ! $gl_gnulib_enabled_intprops; then
      gl_gnulib_enabled_intprops=true
    fi
  }
  func_gl_gnulib_m4code_67241cf2e9b9409f43aeebaa5c55e7f2 ()
  {
    if ! $gl_gnulib_enabled_67241cf2e9b9409f43aeebaa5c55e7f2; then
      gl_INTTYPES_INCOMPLETE
      gl_INTTYPES_H_REQUIRE_DEFAULTS
      AC_PROG_MKDIR_P
      gl_gnulib_enabled_67241cf2e9b9409f43aeebaa5c55e7f2=true
    fi
  }
  func_gl_gnulib_m4code_langinfo ()
  {
    if ! $gl_gnulib_enabled_langinfo; then
      gl_LANGINFO_H
      gl_LANGINFO_H_REQUIRE_DEFAULTS
      AC_PROG_MKDIR_P
      gl_gnulib_enabled_langinfo=true
    fi
  }
  func_gl_gnulib_m4code_21ee726a3540c09237a8e70c0baf7467 ()
  {
    if ! $gl_gnulib_enabled_21ee726a3540c09237a8e70c0baf7467; then
      gl___INLINE
      gl_gnulib_enabled_21ee726a3540c09237a8e70c0baf7467=true
    fi
  }
  func_gl_gnulib_m4code_localcharset ()
  {
    if ! $gl_gnulib_enabled_localcharset; then
      gl_LOCALCHARSET
      dnl For backward compatibility. Some packages still use this.
      LOCALCHARSET_TESTS_ENVIRONMENT=
      AC_SUBST([LOCALCHARSET_TESTS_ENVIRONMENT])
      gl_gnulib_enabled_localcharset=true
    fi
  }
  func_gl_gnulib_m4code_locale ()
  {
    if ! $gl_gnulib_enabled_locale; then
      gl_LOCALE_H
      gl_LOCALE_H_REQUIRE_DEFAULTS
      AC_PROG_MKDIR_P
      gl_gnulib_enabled_locale=true
    fi
  }
  func_gl_gnulib_m4code_localeconv ()
  {
    if ! $gl_gnulib_enabled_localeconv; then
      gl_FUNC_LOCALECONV
      if test $REPLACE_LOCALECONV = 1; then
        AC_LIBOBJ([localeconv])
        gl_PREREQ_LOCALECONV
      fi
      gl_LOCALE_MODULE_INDICATOR([localeconv])
      gl_gnulib_enabled_localeconv=true
      func_gl_gnulib_m4code_locale
    fi
  }
  func_gl_gnulib_m4code_lock ()
  {
    if ! $gl_gnulib_enabled_lock; then
      gl_LOCK
      gl_MODULE_INDICATOR([lock])
      gl_gnulib_enabled_lock=true
      func_gl_gnulib_m4code_threadlib
      if test $gl_threads_api = windows; then
        func_gl_gnulib_m4code_503a4cb75d69c787103d0aa2ab7d8440
      fi
      if test $gl_threads_api = windows; then
        func_gl_gnulib_m4code_68a4501daeca58988392c7e60b4917ab
      fi
      if test $gl_threads_api = windows; then
        func_gl_gnulib_m4code_f0efff84a70f4afba30902bb8ffe9354
      fi
      if test $gl_threads_api = windows; then
        func_gl_gnulib_m4code_8bb827fe37eaccf1b97feb0c87bc92ef
      fi
    fi
  }
  func_gl_gnulib_m4code_mbrtowc ()
  {
    if ! $gl_gnulib_enabled_mbrtowc; then
      gl_FUNC_MBRTOWC
      if test $HAVE_MBRTOWC = 0 || test $REPLACE_MBRTOWC = 1; then
        AC_LIBOBJ([mbrtowc])
        if test $REPLACE_MBSTATE_T = 1; then
          AC_LIBOBJ([lc-charset-dispatch])
          AC_LIBOBJ([mbtowc-lock])
          gl_PREREQ_MBTOWC_LOCK
        fi
        gl_PREREQ_MBRTOWC
      fi
      gl_WCHAR_MODULE_INDICATOR([mbrtowc])
      gl_gnulib_enabled_mbrtowc=true
      if test $HAVE_MBRTOWC = 0 || test $REPLACE_MBRTOWC = 1; then
        func_gl_gnulib_m4code_attribute
      fi
      if { test $HAVE_MBRTOWC = 0 || test $REPLACE_MBRTOWC = 1; } && test $REPLACE_MBSTATE_T = 0; then
        func_gl_gnulib_m4code_30838f5439487421042f2225bed3af76
      fi
      if test $HAVE_MBRTOWC = 0 || test $REPLACE_MBRTOWC = 1; then
        func_gl_gnulib_m4code_localcharset
      fi
      if { test $HAVE_MBRTOWC = 0 || test $REPLACE_MBRTOWC = 1; } && test $REPLACE_MBSTATE_T = 0; then
        func_gl_gnulib_m4code_mbsinit
      fi
      if test $HAVE_MBRTOWC = 0 || test $REPLACE_MBRTOWC = 1; then
        func_gl_gnulib_m4code_streq
      fi
      if test $HAVE_MBRTOWC = 0 || test $REPLACE_MBRTOWC = 1; then
        func_gl_gnulib_m4code_verify
      fi
      func_gl_gnulib_m4code_wchar
    fi
  }
  func_gl_gnulib_m4code_mbsinit ()
  {
    if ! $gl_gnulib_enabled_mbsinit; then
      gl_FUNC_MBSINIT
      if test $HAVE_MBSINIT = 0 || test $REPLACE_MBSINIT = 1; then
        AC_LIBOBJ([mbsinit])
        gl_PREREQ_MBSINIT
      fi
      gl_WCHAR_MODULE_INDICATOR([mbsinit])
      gl_gnulib_enabled_mbsinit=true
      if test $HAVE_MBSINIT = 0 || test $REPLACE_MBSINIT = 1; then
        func_gl_gnulib_m4code_mbrtowc
      fi
      if test $HAVE_MBSINIT = 0 || test $REPLACE_MBSINIT = 1; then
        func_gl_gnulib_m4code_verify
      fi
      func_gl_gnulib_m4code_wchar
    fi
  }
  func_gl_gnulib_m4code_mbtowc ()
  {
    if ! $gl_gnulib_enabled_mbtowc; then
      gl_FUNC_MBTOWC
      if test $HAVE_MBTOWC = 0 || test $REPLACE_MBTOWC = 1; then
        AC_LIBOBJ([mbtowc])
        gl_PREREQ_MBTOWC
      fi
      gl_STDLIB_MODULE_INDICATOR([mbtowc])
      gl_gnulib_enabled_mbtowc=true
      if test $HAVE_MBTOWC = 0 || test $REPLACE_MBTOWC = 1; then
        func_gl_gnulib_m4code_mbrtowc
      fi
      func_gl_gnulib_m4code_stdlib
      if test $HAVE_MBTOWC = 0 || test $REPLACE_MBTOWC = 1; then
        func_gl_gnulib_m4code_wchar
      fi
    fi
  }
  func_gl_gnulib_m4code_nl_langinfo ()
  {
    if ! $gl_gnulib_enabled_nl_langinfo; then
      gl_FUNC_NL_LANGINFO
      if test $HAVE_NL_LANGINFO = 0 || test $REPLACE_NL_LANGINFO = 1; then
        AC_LIBOBJ([nl_langinfo])
      fi
      if test $REPLACE_NL_LANGINFO = 1 && test $NL_LANGINFO_MTSAFE = 0; then
        AC_LIBOBJ([nl_langinfo-lock])
        gl_PREREQ_NL_LANGINFO_LOCK
      fi
      gl_LANGINFO_MODULE_INDICATOR([nl_langinfo])
      gl_gnulib_enabled_nl_langinfo=true
      func_gl_gnulib_m4code_langinfo
      if test $HAVE_NL_LANGINFO = 0 || test $REPLACE_NL_LANGINFO = 1; then
        func_gl_gnulib_m4code_localeconv
      fi
      if test $HAVE_NL_LANGINFO = 0 || test $HAVE_LANGINFO_CODESET = 0; then
        func_gl_gnulib_m4code_e7e881d32ca02f1c997b13c737c64bbd
      fi
    fi
  }
  func_gl_gnulib_m4code_e7e881d32ca02f1c997b13c737c64bbd ()
  {
    if ! $gl_gnulib_enabled_e7e881d32ca02f1c997b13c737c64bbd; then
      gl_FUNC_SETLOCALE_NULL
      if test $SETLOCALE_NULL_ALL_MTSAFE = 0 || test $SETLOCALE_NULL_ONE_MTSAFE = 0; then
        AC_LIBOBJ([setlocale-lock])
        gl_PREREQ_SETLOCALE_LOCK
      fi
      gl_LOCALE_MODULE_INDICATOR([setlocale_null])
      gl_gnulib_enabled_e7e881d32ca02f1c997b13c737c64bbd=true
      func_gl_gnulib_m4code_locale
    fi
  }
  func_gl_gnulib_m4code_b3ae4a413a1340415f34a52d1dafb147 ()
  {
    if ! $gl_gnulib_enabled_b3ae4a413a1340415f34a52d1dafb147; then
      gl_gnulib_enabled_b3ae4a413a1340415f34a52d1dafb147=true
    fi
  }
  func_gl_gnulib_m4code_stdlib ()
  {
    if ! $gl_gnulib_enabled_stdlib; then
      gl_STDLIB_H
      gl_STDLIB_H_REQUIRE_DEFAULTS
      AC_PROG_MKDIR_P
      gl_gnulib_enabled_stdlib=true
      func_gl_gnulib_m4code_b3ae4a413a1340415f34a52d1dafb147
    fi
  }
  func_gl_gnulib_m4code_streq ()
  {
    if ! $gl_gnulib_enabled_streq; then
      gl_gnulib_enabled_streq=true
    fi
  }
  func_gl_gnulib_m4code_threadlib ()
  {
    if ! $gl_gnulib_enabled_threadlib; then
      AC_REQUIRE([gl_THREADLIB])
      gl_gnulib_enabled_threadlib=true
    fi
  }
  func_gl_gnulib_m4code_verify ()
  {
    if ! $gl_gnulib_enabled_verify; then
      gl_gnulib_enabled_verify=true
    fi
  }
  func_gl_gnulib_m4code_wchar ()
  {
    if ! $gl_gnulib_enabled_wchar; then
      gl_WCHAR_H
      gl_WCHAR_H_REQUIRE_DEFAULTS
      AC_PROG_MKDIR_P
      gl_gnulib_enabled_wchar=true
      func_gl_gnulib_m4code_67241cf2e9b9409f43aeebaa5c55e7f2
      func_gl_gnulib_m4code_stdlib
    fi
  }
  func_gl_gnulib_m4code_wcrtomb ()
  {
    if ! $gl_gnulib_enabled_wcrtomb; then
      gl_FUNC_WCRTOMB
      if test $HAVE_WCRTOMB = 0 || test $REPLACE_WCRTOMB = 1; then
        AC_LIBOBJ([wcrtomb])
        gl_PREREQ_WCRTOMB
      fi
      gl_WCHAR_MODULE_INDICATOR([wcrtomb])
      gl_gnulib_enabled_wcrtomb=true
      if test $HAVE_WCRTOMB = 0 || test $REPLACE_WCRTOMB = 1; then
        func_gl_gnulib_m4code_mbsinit
      fi
      func_gl_gnulib_m4code_wchar
    fi
  }
  func_gl_gnulib_m4code_3dcce957eadc896e63ab5f137947b410 ()
  {
    if ! $gl_gnulib_enabled_3dcce957eadc896e63ab5f137947b410; then
      gl_WCTYPE_H
      gl_WCTYPE_H_REQUIRE_DEFAULTS
      AC_PROG_MKDIR_P
      gl_gnulib_enabled_3dcce957eadc896e63ab5f137947b410=true
    fi
  }
  func_gl_gnulib_m4code_503a4cb75d69c787103d0aa2ab7d8440 ()
  {
    if ! $gl_gnulib_enabled_503a4cb75d69c787103d0aa2ab7d8440; then
      AC_REQUIRE([AC_CANONICAL_HOST])
      case "$host_os" in
        mingw*)
          AC_LIBOBJ([windows-mutex])
          ;;
      esac
      gl_gnulib_enabled_503a4cb75d69c787103d0aa2ab7d8440=true
    fi
  }
  func_gl_gnulib_m4code_68a4501daeca58988392c7e60b4917ab ()
  {
    if ! $gl_gnulib_enabled_68a4501daeca58988392c7e60b4917ab; then
      AC_REQUIRE([AC_CANONICAL_HOST])
      case "$host_os" in
        mingw*)
          AC_LIBOBJ([windows-once])
          ;;
      esac
      gl_gnulib_enabled_68a4501daeca58988392c7e60b4917ab=true
    fi
  }
  func_gl_gnulib_m4code_f0efff84a70f4afba30902bb8ffe9354 ()
  {
    if ! $gl_gnulib_enabled_f0efff84a70f4afba30902bb8ffe9354; then
      AC_REQUIRE([AC_CANONICAL_HOST])
      case "$host_os" in
        mingw*)
          AC_LIBOBJ([windows-recmutex])
          ;;
      esac
      gl_gnulib_enabled_f0efff84a70f4afba30902bb8ffe9354=true
    fi
  }
  func_gl_gnulib_m4code_8bb827fe37eaccf1b97feb0c87bc92ef ()
  {
    if ! $gl_gnulib_enabled_8bb827fe37eaccf1b97feb0c87bc92ef; then
      AC_REQUIRE([AC_CANONICAL_HOST])
      case "$host_os" in
        mingw*)
          AC_LIBOBJ([windows-rwlock])
          ;;
      esac
      gl_gnulib_enabled_8bb827fe37eaccf1b97feb0c87bc92ef=true
    fi
  }
  if test $REPLACE_GETOPT = 1; then
    func_gl_gnulib_m4code_be453cec5eecf5731a274f2de7f2db36
  fi
  if test $ac_use_included_regex = yes; then
    func_gl_gnulib_m4code_attribute
  fi
  if test $ac_use_included_regex = yes; then
    func_gl_gnulib_m4code_btowc
  fi
  if test $ac_use_included_regex = yes; then
    func_gl_gnulib_m4code_37f71b604aa9c54446783d80f42fe547
  fi
  if test $ac_use_included_regex = yes; then
    func_gl_gnulib_m4code_dynarray
  fi
  if test $ac_use_included_regex = yes; then
    func_gl_gnulib_m4code_intprops
  fi
  if test $ac_use_included_regex = yes; then
    func_gl_gnulib_m4code_langinfo
  fi
  if test $ac_use_included_regex = yes; then
    func_gl_gnulib_m4code_21ee726a3540c09237a8e70c0baf7467
  fi
  if test $ac_use_included_regex = yes; then
    func_gl_gnulib_m4code_lock
  fi
  if test $ac_use_included_regex = yes; then
    func_gl_gnulib_m4code_mbrtowc
  fi
  if test $ac_use_included_regex = yes; then
    func_gl_gnulib_m4code_mbsinit
  fi
  if test $ac_use_included_regex = yes; then
    func_gl_gnulib_m4code_nl_langinfo
  fi
  if test $ac_use_included_regex = yes; then
    func_gl_gnulib_m4code_verify
  fi
  if test $ac_use_included_regex = yes; then
    func_gl_gnulib_m4code_wchar
  fi
  if test $ac_use_included_regex = yes; then
    func_gl_gnulib_m4code_wcrtomb
  fi
  if test $ac_use_included_regex = yes; then
    func_gl_gnulib_m4code_3dcce957eadc896e63ab5f137947b410
  fi
  m4_pattern_allow([^gl_GNULIB_ENABLED_])
  AM_CONDITIONAL([gl_GNULIB_ENABLED_attribute], [$gl_gnulib_enabled_attribute])
  AM_CONDITIONAL([gl_GNULIB_ENABLED_btowc], [$gl_gnulib_enabled_btowc])
  AM_CONDITIONAL([gl_GNULIB_ENABLED_37f71b604aa9c54446783d80f42fe547], [$gl_gnulib_enabled_37f71b604aa9c54446783d80f42fe547])
  AM_CONDITIONAL([gl_GNULIB_ENABLED_dynarray], [$gl_gnulib_enabled_dynarray])
  AM_CONDITIONAL([gl_GNULIB_ENABLED_be453cec5eecf5731a274f2de7f2db36], [$gl_gnulib_enabled_be453cec5eecf5731a274f2de7f2db36])
  AM_CONDITIONAL([gl_GNULIB_ENABLED_30838f5439487421042f2225bed3af76], [$gl_gnulib_enabled_30838f5439487421042f2225bed3af76])
  AM_CONDITIONAL([gl_GNULIB_ENABLED_intprops], [$gl_gnulib_enabled_intprops])
  AM_CONDITIONAL([gl_GNULIB_ENABLED_67241cf2e9b9409f43aeebaa5c55e7f2], [$gl_gnulib_enabled_67241cf2e9b9409f43aeebaa5c55e7f2])
  AM_CONDITIONAL([gl_GNULIB_ENABLED_langinfo], [$gl_gnulib_enabled_langinfo])
  AM_CONDITIONAL([gl_GNULIB_ENABLED_21ee726a3540c09237a8e70c0baf7467], [$gl_gnulib_enabled_21ee726a3540c09237a8e70c0baf7467])
  AM_CONDITIONAL([gl_GNULIB_ENABLED_localcharset], [$gl_gnulib_enabled_localcharset])
  AM_CONDITIONAL([gl_GNULIB_ENABLED_locale], [$gl_gnulib_enabled_locale])
  AM_CONDITIONAL([gl_GNULIB_ENABLED_localeconv], [$gl_gnulib_enabled_localeconv])
  AM_CONDITIONAL([gl_GNULIB_ENABLED_lock], [$gl_gnulib_enabled_lock])
  AM_CONDITIONAL([gl_GNULIB_ENABLED_mbrtowc], [$gl_gnulib_enabled_mbrtowc])
  AM_CONDITIONAL([gl_GNULIB_ENABLED_mbsinit], [$gl_gnulib_enabled_mbsinit])
  AM_CONDITIONAL([gl_GNULIB_ENABLED_mbtowc], [$gl_gnulib_enabled_mbtowc])
  AM_CONDITIONAL([gl_GNULIB_ENABLED_nl_langinfo], [$gl_gnulib_enabled_nl_langinfo])
  AM_CONDITIONAL([gl_GNULIB_ENABLED_e7e881d32ca02f1c997b13c737c64bbd], [$gl_gnulib_enabled_e7e881d32ca02f1c997b13c737c64bbd])
  AM_CONDITIONAL([gl_GNULIB_ENABLED_b3ae4a413a1340415f34a52d1dafb147], [$gl_gnulib_enabled_b3ae4a413a1340415f34a52d1dafb147])
  AM_CONDITIONAL([gl_GNULIB_ENABLED_stdlib], [$gl_gnulib_enabled_stdlib])
  AM_CONDITIONAL([gl_GNULIB_ENABLED_streq], [$gl_gnulib_enabled_streq])
  AM_CONDITIONAL([gl_GNULIB_ENABLED_threadlib], [$gl_gnulib_enabled_threadlib])
  AM_CONDITIONAL([gl_GNULIB_ENABLED_verify], [$gl_gnulib_enabled_verify])
  AM_CONDITIONAL([gl_GNULIB_ENABLED_wchar], [$gl_gnulib_enabled_wchar])
  AM_CONDITIONAL([gl_GNULIB_ENABLED_wcrtomb], [$gl_gnulib_enabled_wcrtomb])
  AM_CONDITIONAL([gl_GNULIB_ENABLED_3dcce957eadc896e63ab5f137947b410], [$gl_gnulib_enabled_3dcce957eadc896e63ab5f137947b410])
  AM_CONDITIONAL([gl_GNULIB_ENABLED_503a4cb75d69c787103d0aa2ab7d8440], [$gl_gnulib_enabled_503a4cb75d69c787103d0aa2ab7d8440])
  AM_CONDITIONAL([gl_GNULIB_ENABLED_68a4501daeca58988392c7e60b4917ab], [$gl_gnulib_enabled_68a4501daeca58988392c7e60b4917ab])
  AM_CONDITIONAL([gl_GNULIB_ENABLED_f0efff84a70f4afba30902bb8ffe9354], [$gl_gnulib_enabled_f0efff84a70f4afba30902bb8ffe9354])
  AM_CONDITIONAL([gl_GNULIB_ENABLED_8bb827fe37eaccf1b97feb0c87bc92ef], [$gl_gnulib_enabled_8bb827fe37eaccf1b97feb0c87bc92ef])
  # End of code from modules
  m4_ifval(gl_LIBSOURCES_LIST, [
    m4_syscmd([test ! -d ]m4_defn([gl_LIBSOURCES_DIR])[ ||
      for gl_file in ]gl_LIBSOURCES_LIST[ ; do
        if test ! -r ]m4_defn([gl_LIBSOURCES_DIR])[/$gl_file ; then
          echo "missing file ]m4_defn([gl_LIBSOURCES_DIR])[/$gl_file" >&2
          exit 1
        fi
      done])dnl
      m4_if(m4_sysval, [0], [],
        [AC_FATAL([expected source file, required through AC_LIBSOURCES, not found])])
  ])
  m4_popdef([GL_MODULE_INDICATOR_PREFIX])
  m4_popdef([GL_MACRO_PREFIX])
  m4_popdef([gl_LIBSOURCES_DIR])
  m4_popdef([gl_LIBSOURCES_LIST])
  m4_popdef([AC_LIBSOURCES])
  m4_popdef([AC_REPLACE_FUNCS])
  m4_popdef([AC_LIBOBJ])
  AC_CONFIG_COMMANDS_PRE([
    gl_libobjs=
    gl_ltlibobjs=
    if test -n "$gl_LIBOBJS"; then
      # Remove the extension.
      sed_drop_objext='s/\.o$//;s/\.obj$//'
      for i in `for i in $gl_LIBOBJS; do echo "$i"; done | sed -e "$sed_drop_objext" | sort | uniq`; do
        gl_libobjs="$gl_libobjs $i.$ac_objext"
        gl_ltlibobjs="$gl_ltlibobjs $i.lo"
      done
    fi
    AC_SUBST([gl_LIBOBJS], [$gl_libobjs])
    AC_SUBST([gl_LTLIBOBJS], [$gl_ltlibobjs])
  ])
  gltests_libdeps=
  gltests_ltlibdeps=
  m4_pushdef([AC_LIBOBJ], m4_defn([gltests_LIBOBJ]))
  m4_pushdef([AC_REPLACE_FUNCS], m4_defn([gltests_REPLACE_FUNCS]))
  m4_pushdef([AC_LIBSOURCES], m4_defn([gltests_LIBSOURCES]))
  m4_pushdef([gltests_LIBSOURCES_LIST], [])
  m4_pushdef([gltests_LIBSOURCES_DIR], [])
  m4_pushdef([GL_MACRO_PREFIX], [gltests])
  m4_pushdef([GL_MODULE_INDICATOR_PREFIX], [GL])
  gl_COMMON
  gl_source_base='tests'
  gl_source_base_prefix=
changequote(,)dnl
  gltests_WITNESS=IN_`echo "${PACKAGE-$PACKAGE_TARNAME}" | LC_ALL=C tr abcdefghijklmnopqrstuvwxyz ABCDEFGHIJKLMNOPQRSTUVWXYZ | LC_ALL=C sed -e 's/[^A-Z0-9_]/_/g'`_GNULIB_TESTS
changequote([, ])dnl
  AC_SUBST([gltests_WITNESS])
  gl_module_indicator_condition=$gltests_WITNESS
  m4_pushdef([gl_MODULE_INDICATOR_CONDITION], [$gl_module_indicator_condition])
  m4_pattern_allow([^gl_GNULIB_ENABLED_])
  m4_popdef([gl_MODULE_INDICATOR_CONDITION])
  m4_ifval(gltests_LIBSOURCES_LIST, [
    m4_syscmd([test ! -d ]m4_defn([gltests_LIBSOURCES_DIR])[ ||
      for gl_file in ]gltests_LIBSOURCES_LIST[ ; do
        if test ! -r ]m4_defn([gltests_LIBSOURCES_DIR])[/$gl_file ; then
          echo "missing file ]m4_defn([gltests_LIBSOURCES_DIR])[/$gl_file" >&2
          exit 1
        fi
      done])dnl
      m4_if(m4_sysval, [0], [],
        [AC_FATAL([expected source file, required through AC_LIBSOURCES, not found])])
  ])
  m4_popdef([GL_MODULE_INDICATOR_PREFIX])
  m4_popdef([GL_MACRO_PREFIX])
  m4_popdef([gltests_LIBSOURCES_DIR])
  m4_popdef([gltests_LIBSOURCES_LIST])
  m4_popdef([AC_LIBSOURCES])
  m4_popdef([AC_REPLACE_FUNCS])
  m4_popdef([AC_LIBOBJ])
  AC_CONFIG_COMMANDS_PRE([
    gltests_libobjs=
    gltests_ltlibobjs=
    if test -n "$gltests_LIBOBJS"; then
      # Remove the extension.
      sed_drop_objext='s/\.o$//;s/\.obj$//'
      for i in `for i in $gltests_LIBOBJS; do echo "$i"; done | sed -e "$sed_drop_objext" | sort | uniq`; do
        gltests_libobjs="$gltests_libobjs $i.$ac_objext"
        gltests_ltlibobjs="$gltests_ltlibobjs $i.lo"
      done
    fi
    AC_SUBST([gltests_LIBOBJS], [$gltests_libobjs])
    AC_SUBST([gltests_LTLIBOBJS], [$gltests_ltlibobjs])
  ])
  LIBGNU_LIBDEPS="$gl_libdeps"
  AC_SUBST([LIBGNU_LIBDEPS])
  LIBGNU_LTLIBDEPS="$gl_ltlibdeps"
  AC_SUBST([LIBGNU_LTLIBDEPS])
])

# Like AC_LIBOBJ, except that the module name goes
# into gl_LIBOBJS instead of into LIBOBJS.
AC_DEFUN([gl_LIBOBJ], [
  AS_LITERAL_IF([$1], [gl_LIBSOURCES([$1.c])])dnl
  gl_LIBOBJS="$gl_LIBOBJS $1.$ac_objext"
])

# Like AC_REPLACE_FUNCS, except that the module name goes
# into gl_LIBOBJS instead of into LIBOBJS.
AC_DEFUN([gl_REPLACE_FUNCS], [
  m4_foreach_w([gl_NAME], [$1], [AC_LIBSOURCES(gl_NAME[.c])])dnl
  AC_CHECK_FUNCS([$1], , [gl_LIBOBJ($ac_func)])
])

# Like AC_LIBSOURCES, except the directory where the source file is
# expected is derived from the gnulib-tool parameterization,
# and alloca is special cased (for the alloca-opt module).
# We could also entirely rely on EXTRA_lib..._SOURCES.
AC_DEFUN([gl_LIBSOURCES], [
  m4_foreach([_gl_NAME], [$1], [
    m4_if(_gl_NAME, [alloca.c], [], [
      m4_define([gl_LIBSOURCES_DIR], [lib])
      m4_append([gl_LIBSOURCES_LIST], _gl_NAME, [ ])
    ])
  ])
])

# Like AC_LIBOBJ, except that the module name goes
# into gltests_LIBOBJS instead of into LIBOBJS.
AC_DEFUN([gltests_LIBOBJ], [
  AS_LITERAL_IF([$1], [gltests_LIBSOURCES([$1.c])])dnl
  gltests_LIBOBJS="$gltests_LIBOBJS $1.$ac_objext"
])

# Like AC_REPLACE_FUNCS, except that the module name goes
# into gltests_LIBOBJS instead of into LIBOBJS.
AC_DEFUN([gltests_REPLACE_FUNCS], [
  m4_foreach_w([gl_NAME], [$1], [AC_LIBSOURCES(gl_NAME[.c])])dnl
  AC_CHECK_FUNCS([$1], , [gltests_LIBOBJ($ac_func)])
])

# Like AC_LIBSOURCES, except the directory where the source file is
# expected is derived from the gnulib-tool parameterization,
# and alloca is special cased (for the alloca-opt module).
# We could also entirely rely on EXTRA_lib..._SOURCES.
AC_DEFUN([gltests_LIBSOURCES], [
  m4_foreach([_gl_NAME], [$1], [
    m4_if(_gl_NAME, [alloca.c], [], [
      m4_define([gltests_LIBSOURCES_DIR], [tests])
      m4_append([gltests_LIBSOURCES_LIST], _gl_NAME, [ ])
    ])
  ])
])

# This macro records the list of files which have been installed by
# gnulib-tool and may be removed by future gnulib-tool invocations.
AC_DEFUN([gl_FILE_LIST], [
  lib/_Noreturn.h
  lib/arg-nonnull.h
  lib/attribute.h
  lib/btowc.c
  lib/c++defs.h
  lib/cdefs.h
  lib/dynarray.h
  lib/getopt-cdefs.in.h
  lib/getopt-core.h
  lib/getopt-ext.h
  lib/getopt-pfx-core.h
  lib/getopt-pfx-ext.h
  lib/getopt.c
  lib/getopt.in.h
  lib/getopt1.c
  lib/getopt_int.h
  lib/gettext.h
  lib/glthread/lock.c
  lib/glthread/lock.h
  lib/glthread/threadlib.c
  lib/hard-locale.c
  lib/hard-locale.h
  lib/intprops.h
  lib/inttypes.in.h
  lib/langinfo.in.h
  lib/lc-charset-dispatch.c
  lib/lc-charset-dispatch.h
  lib/libc-config.h
  lib/limits.in.h
  lib/localcharset.c
  lib/localcharset.h
  lib/locale.in.h
  lib/localeconv.c
  lib/malloc/dynarray-skeleton.c
  lib/malloc/dynarray.h
  lib/malloc/dynarray_at_failure.c
  lib/malloc/dynarray_emplace_enlarge.c
  lib/malloc/dynarray_finalize.c
  lib/malloc/dynarray_resize.c
  lib/malloc/dynarray_resize_clear.c
  lib/mbrtowc-impl-utf8.h
  lib/mbrtowc-impl.h
  lib/mbrtowc.c
  lib/mbsinit.c
  lib/mbtowc-impl.h
  lib/mbtowc-lock.c
  lib/mbtowc-lock.h
  lib/mbtowc.c
  lib/nl_langinfo-lock.c
  lib/nl_langinfo.c
  lib/regcomp.c
  lib/regex.c
  lib/regex.h
  lib/regex_internal.c
  lib/regex_internal.h
  lib/regexec.c
  lib/setlocale-lock.c
  lib/setlocale_null.c
  lib/setlocale_null.h
  lib/stdbool.in.h
  lib/stddef.in.h
  lib/stdint.in.h
  lib/stdlib.in.h
  lib/streq.h
  lib/sys_types.in.h
  lib/unistd.c
  lib/unistd.in.h
  lib/verify.h
  lib/warn-on-use.h
  lib/wchar.in.h
  lib/wcrtomb.c
  lib/wctype-h.c
  lib/wctype.in.h
  lib/windows-initguard.h
  lib/windows-mutex.c
  lib/windows-mutex.h
  lib/windows-once.c
  lib/windows-once.h
  lib/windows-recmutex.c
  lib/windows-recmutex.h
  lib/windows-rwlock.c
  lib/windows-rwlock.h
  m4/00gnulib.m4
  m4/__inline.m4
  m4/absolute-header.m4
  m4/btowc.m4
  m4/builtin-expect.m4
  m4/codeset.m4
  m4/eealloc.m4
  m4/extensions.m4
  m4/extern-inline.m4
  m4/getopt.m4
  m4/gnulib-common.m4
  m4/include_next.m4
  m4/inttypes.m4
  m4/langinfo_h.m4
  m4/limits-h.m4
  m4/localcharset.m4
  m4/locale-fr.m4
  m4/locale-ja.m4
  m4/locale-zh.m4
  m4/locale_h.m4
  m4/localeconv.m4
  m4/lock.m4
  m4/mbrtowc.m4
  m4/mbsinit.m4
  m4/mbstate_t.m4
  m4/mbtowc.m4
  m4/multiarch.m4
  m4/nl_langinfo.m4
  m4/nocrash.m4
  m4/off_t.m4
  m4/pid_t.m4
  m4/pthread_rwlock_rdlock.m4
  m4/regex.m4
  m4/setlocale_null.m4
  m4/ssize_t.m4
  m4/std-gnu11.m4
  m4/stdbool.m4
  m4/stddef_h.m4
  m4/stdint.m4
  m4/stdlib_h.m4
  m4/sys_types_h.m4
  m4/threadlib.m4
  m4/unistd_h.m4
  m4/vararrays.m4
  m4/visibility.m4
  m4/warn-on-use.m4
  m4/wchar_h.m4
  m4/wchar_t.m4
  m4/wcrtomb.m4
  m4/wctype_h.m4
  m4/wint_t.m4
  m4/zzgnulib.m4
])
