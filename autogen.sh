#!/bin/sh
#
# @(#)autogen.sh
#
# This script generates a GNU Autoconf configure script for the ed
# line editor.
#
script_name=$(basename $0)

if set +o 2>/dev/null | grep -q 'pipefail$'; then
    set -o pipefail
fi

case "$1" in
    -h*|--h*)
        echo "Usage: $script_name [-h|--help] [-s|--silent] [maintainer-update-dist]"
        exit
        ;;
esac

verbose='true'
very_verbose='false'
case "$1" in
    -s*|--s*)
        verbose='false'
        shift
        ;;
    -v*|--v*)
        very_verbose='true'
        shift
        ;;
esac

srcdir=$(dirname $0)
abs_srcdir="$(cd "$srcdir" && pwd)"
if [ ! -w "$abs_srcdir" ]; then
    echo "$script_name: $abs_srcdir: Permission denied"
    exit 2
fi

for cmd in aclocal autoheader autopoint automake autoreconf libtoolize; do
    eval ${cmd}_cmd="\$(command -v '$cmd')"
    exit_status=$?
    if test $exit_status -ne 0 || eval test ."\$${cmd}_cmd" = .''; then
        eval ${cmd}_cmd="\$(command -v 'g$cmd')"
        exit_status=$?
    fi
    if test $exit_status -ne 0 || eval test ."\$${cmd}_cmd" = .''; then
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
elif $very_verbose; then
    echo "$aclocal_output"
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
elif $very_verbose; then
    echo "$autoheader_output"
fi

$verbose && cat <<EOF
$script_name: Running:
  cd "$abs_srcdir" &&
  $autopoint_cmd --force >&2

EOF

autopoint_output=$(
    cd "$abs_srcdir" &&
        $autopoint_cmd --force 2>&1
               )
exit_status=$?
if test $exit_status -ne 0; then
    cat >&2 <<EOF
$script_name:
$autopoint_output
EOF
    exit $exit_status
elif $very_verbose; then
    echo "$autopoint_output"
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
elif $very_verbose; then
    echo "$autoreconf_output"
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
elif $very_verbose; then
    echo "$automake_output"
fi

$verbose && cat <<EOF
$script_name: Running:
  cd "$abs_srcdir" &&
  $libtoolize_cmd --verbose --force --install --copy  >&2

EOF

libtoolize_output=$(
    cd "$abs_srcdir" &&
        $libtoolize_cmd --verbose --force --install --copy  2>&1
               )
exit_status=$?
if test $exit_status -ne 0; then
    cat >&2 <<EOF
$script_name:
$libtoolize_output
EOF
    exit $exit_status
elif $very_verbose; then
    echo "$libtoolize_output"
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
elif $very_verbose; then
    echo "$autoreconf_output"
fi

if $verbose; then
    echo "$script_name:" >&2
    cat >&2 <<'EOF'
===============================================================================

     Autotools appears to have completed successfully. To continue,
     run:

             $ ./configure [--enable-EXTENSION ...|--enable-all-extensions]
             $ gmake

     where available EXTENSIONs are

             address-arguments - enable command-line address arguments.
             ed-envar          - enable ED environment variable.
             external-filter   - enable external filtering of ed's buffer.
             file-glob         - enable file globbing and multi-file support.
             file-lock         - enable file locking support.
             ed-macro          - enable macro support.
             ed-register       - enable register support.
             script-flags      - enable command-line script flags.
             ed-scroll         - enable scrolling.
             ed-encryption     - enable encryption/decryption of files.

      The testsuite is generated with GNU AutoTest and requires
      GNU make. It is run as follows:

             $ gmake check

      Please submit issues or pull requests to: <https://github.com/slewsys/ed>

-------------------------------------------------------------------------------
EOF
fi
