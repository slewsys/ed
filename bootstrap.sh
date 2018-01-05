#!/usr/bin/env bash
#
# @(#)bootstrap.sh
#
# Copyright © 2006-2016 Andrew L. Moore, SlewSys Research
#
# This script generates a GNU Autoconf configure script for the ed
# line editor.
#
PGM="${0##*/}"

if [[ ."$1" = .-h* ]] || [[ ."$1" = .--h* ]]; then
    echo "Usage: $PGM [-h|--help] [-s|--silent] [maintainer-update-dist]"
    exit 1
fi

verbose='true'
if [[ ."$1" = .-s* ]] || [[ ."$1" = .--s* ]]; then
    verbose='false'
    shift
fi

srcdir="${0%/*}"
abs_srcdir="$(cd "$srcdir" && pwd)"
if [ ! -w "$abs_srcdir" ]; then
    echo "$PGM: $abs_srcdir: Permission denied"
    exit 2
fi

AUTORECONF=$(which autoreconf)
exit_status=$?
if (( exit_status != 0 )); then
    cat <<EOF
$PGM: autoreconf: File not found
Please verify installation of GNU Autoconf, Automake, Gettext and
Libtool before running this script.
EOF
    exit $exit_status
fi

$verbose && cat <<EOF
$PGM: Running:
  cd "$abs_srcdir" &&
  $AUTORECONF --verbose --force --install -I ./m4 >&2

EOF


autoconf_output="$(cd "$abs_srcdir" && $AUTORECONF --verbose --force --install -I ./m4 2>&1)"
exit_status=$?
if (( exit_status != 0 )); then
    cat <<EOF
$PGM:
$autoconf_output
EOF
    exit $exit_status
fi

if $verbose; then
    echo "$PGM:" >&2
    cat >&2 <<'EOF'
========================================================================

     Autoreconf appears to have completed successfully. To continue,
     optionally create and cd to a build directory, then run:

             $ $top_srcdir/configure
             $ make

      The distributed testsuite, `$top_srcdir/testsuite/testsuite-dist',
      must be run from $top_srcdir.  It is invoked via:

             $ make quick-check

      To run testsuite from another build directory, GNU autoconf(1)
      and GNU make(1) are required. In this case, use:

             $ gnumake check

      Report bugs to: bug-ed@gnu.org

------------------------------------------------------------------------
EOF
fi