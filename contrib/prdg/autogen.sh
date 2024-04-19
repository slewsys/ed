#!/bin/sh
#
# @(#)autogen.sh
#
# This script generates a GNU Autoconf configure script for prcg.
#
script_name=$(basename $0)

case "$1" in
    -h*|--h*)
        echo "Usage: $script_name [-h|--help] [-s|--silent] [maintainer-update-dist]"
        exitn
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

for cmd in aclocal autoheader automake autoreconf; do
    eval ${cmd}_cmd='$(which '$cmd' 2>/dev/null)'
    exit_status=$?
    if test $exit_status -ne 0; then
        cat <<EOF
$script_name: $cmd: Command not found
Please verify installation of GNU Autoconf, Automake, Autopoint,
Libtool, Gettext and Texinfo before running this script.
EOF
        exit $exit_status
    fi
done

$verbose && cat <<EOF
$script_name: Running:
  cd "$abs_srcdir" &&
  $aclocal_cmd -I./m4 --verbose --force --install >&2

EOF

aclocal_output=$(
    cd "$abs_srcdir" &&
        $aclocal_cmd -I./m4 --force --install 2>&1
               )
exit_status=$?
if test $exit_status -ne 0; then
    cat >&2 <<EOF
$script_name:
$aclocal_output
EOF
    exit $exit_status
fi

$verbose && cat <<EOF
$script_name: Running:
  cd "$abs_srcdir" &&
  $autoheader_cmd --verbose --force --warnings=gnu >&2

EOF

autoheader_output=$(
    cd "$abs_srcdir" &&
        $autoheader_cmd --verbose --force --warnings=gnu 2>&1
               )
exit_status=$?
if test $exit_status -ne 0; then
    cat >&2 <<EOF
$script_name:
$autoheader_output
EOF
    exit $exit_status
fi

$verbose && cat <<EOF
$script_name: Running:
  cd "$abs_srcdir" &&
  $autoreconf_cmd -I./m4 --verbose --install >&2

EOF

autoreconf_output=$(
    cd "$abs_srcdir" &&
        $autoreconf_cmd -I./m4 --verbose --install 2>&1
               )
exit_status=$?
if test $exit_status -ne 0; then
    cat >&2 <<EOF
$script_name:
$autoreconf_output
EOF
    exit $exit_status
fi

$verbose && cat <<EOF
$script_name: Running:
  cd "$abs_srcdir" &&
  $automake_cmd --verbose --force-missing --copy --gnu >&2

EOF

automake_output=$(
    cd "$abs_srcdir" &&
        $automake_cmd --verbose --force-missing --copy 2>&1
               )
exit_status=$?
if test $exit_status -ne 0; then
    cat >&2 <<EOF
$script_name:
$automake_output
EOF
    exit $exit_status
fi

$verbose && cat <<EOF
$script_name: Running:
  cd "$abs_srcdir" &&
  $autoreconf_cmd -I./m4 --verbose --install >&2

EOF

autoreconf_output=$(
    cd "$abs_srcdir" &&
        $autoreconf_cmd -I./m4 --verbose --install 2>&1
               )
exit_status=$?
if test $exit_status -ne 0; then
    cat >&2 <<EOF
$script_name:
$autoreconf_output
EOF
    exit $exit_status
fi

if $verbose; then
    echo "$script_name:" >&2
    cat >&2 <<'EOF'
========================================================================

     Autotools appears to have completed successfully. To continue,
     optionally create and cd to a build directory, then run:

             $ $top_srcdir/configure
             $ make

      Please submit issues or pull requests to: <https://github.com/slewsys/ed>

------------------------------------------------------------------------
EOF
fi
