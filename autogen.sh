#!/bin/sh
#
# @(#)autogen.sh
#
# This script generates a GNU Autoconf configure script for the ed
# line editor.
#
script_name=$(basename $0)

case "$1" in
    -h*|--h*)
        echo "Usage: $script_name [-h|--help] [-s|--silent] [maintainer-update-dist]"
        exit
        ;;
esac

verbose='true'
case "$1" in
    -s*|--s*)
        verbose='false'
        shift
        ;;
esac

srcdir=$(dirname $0)
abs_srcdir="$(cd "$srcdir" && pwd)"
if [ ! -w "$abs_srcdir" ]; then
    echo "$script_name: $abs_srcdir: Permission denied"
    exit 2
fi

autoreconf_cmd=$(which autoreconf)
exit_status=$?
if test $exit_status -ne 0; then
    cat <<EOF
$script_name: autoreconf: File not found
Please verify installation of GNU Autoconf, Automake, Gettext and
Libtool before running this script.
EOF
    exit $exit_status
fi

$verbose && cat <<EOF
$script_name: Running:
  cd "$abs_srcdir" &&
  autoreconf --verbose --force --install -I ./m4 >&2

EOF


autoconf_output=$(cd "$abs_srcdir" && autoreconf --verbose --force --install -I ./m4 2>&1)
exit_status=$?
if (( exit_status != 0 )); then
    cat <<EOF
$script_name:
$autoconf_output
EOF
    exit $exit_status
fi

if $verbose; then
    echo "$script_name:" >&2
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

      Submit issues or pull requests to: <bug-ed@gnu.org>.

------------------------------------------------------------------------
EOF
fi
