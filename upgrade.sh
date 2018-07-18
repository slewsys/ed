#!/bin/sh
#
# @(#)upgrade-sh
#
# This script invokes `configure' with options listed by `ed --version'.
#
script_name=$(basename $0)

_exit_status=''
_process_output=''
verbose='true'

if ! ed  --version >/dev/null 2>&1; then
    echo "$script_name: ed: --version: Unrecognized option." >&2
    exit 1
fi

usage() {
    echo "Usage: $script_name [-h|--help] [-s|--silent] [maintainer-update-dist]" >&2
    exit
}

#
# check_exit_status: Print diagnostic and exit on errors.
#
check_exit_status() {
    if test $_exit_status -ne 0; then
        cat >&2 <<EOF
$script_name: Process terminated with errors.
$_process_output
EOF

        exit $_exit_status
    fi
}

case "$1" in
    -h*|--h*)
        usage
        ;;
esac

case "$1" in
    -s*|--s*)
        verbose='false'
        shift
        ;;
esac

case "$1" in
    -*)
        usage
        ;;
esac

srcdir=$(dirname $0)

$verbose && cat >&2 <<EOF
$script_name: Running:
  ( cd "$srcdir" && ./autogen.sh --silent )

EOF

_process_output="$( cd "$srcdir" && ./autogen.sh --silent )"
_exit_status=$?

$verbose && cat >&2 <<EOF
$script_name: Running:
  $(eval echo "$srcdir/configure" $(ed --version | tail -n +3 | tr -d '\n'))

EOF

_process_output=$(eval \"$srcdir/configure\" $(ed --info | tail -n +3 | tr -d '\n'))
_exit_status=$?
check_exit_status


# If no args, try just compiling.
if test $# -eq 0; then
    $verbose && cat <<EOF
$script_name: Running:
  make all

EOF

    _process_output=$(make all 2>&1)
    _exit_status=$?

# Otherwise, update distribution.
else
    $verbose && cat >&2 <<EOF
$script_name: Running:
  make maintainer-update-dist

EOF

    _process_output=$(make maintainer-update-dist)
    _exit_status=$?
    check_exit_status

    # Update ed.pot.in
    ed - po/ed.pot <<'EOF'
/^\("Project-Id-Version:\).*/s;;\1 @PACKAGE@ @VERSION@\\n";
wq po/ed.pot.in
EOF

    _process_output=$(eval \"${srcdir}/configure\" $(ed --version | tail -n +3 | tr -d '\n') 2>&1)
    _exit_status=$?
fi
check_exit_status


# Try Invoking testsuite.
abs_srcdir=$(cd "$srcdir" >/dev/null 2>&1 && pwd)

# If builddir = srcdir, use quick-check.
if test ."$abs_srcdir" = ."$(pwd)"; then
    $verbose && cat >&2 <<EOF
$script_name: Running:
  make quick-check

EOF

    _process_output=$(make quick-check 2>&1)
    _exit_status=$?

# Otherwise, try invoking GNU make.
elif test -x "$(which gmake)"; then
    $verbose && cat >&2 <<EOF
$script_name: Running:
  gmake check

EOF

    _process_output=$(gmake check 2>&1)
    _exit_status=$?
elif test -x "$(which gnumake)"; then
    $verbose && cat >&2 <<EOF
$script_name: Running:
  gnumake check

EOF

    _process_output=$(gnumake check 2>&1)
    _exit_status=$?
elif test ."$(expr "$(make --version)"  : '\(GNU Make\)')" = .'GNU Make'; then
    $verbose && cat >&2  <<EOF
$script_name: Running:
  make check

EOF

    _process_output=$(make check 2>&1)
    _exit_status=$?
else
    cat >&2 <<EOF
To run testsuite, invoke \`check' target with GNU Make

EOF

fi
check_exit_status
