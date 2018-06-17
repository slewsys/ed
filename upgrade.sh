#!/bin/bash -
#
# @(#)upgrade-sh
#
# Copyright © 2006-2016 Andrew L. Moore, SlewSys Research
#
# This script invokes `configure' with options listed by `ed --info'.
#
declare -i _exit_status=''
declare _process_output=''
declare verbose='true'
declare pgm="${0##*/}"

ed  --info >/dev/null 2>&1 || {
    echo "ed: --info: Unrecognized option." >&2
    exit 1
}

usage() {
    echo "Usage: $pgm [-h|--help] [-s|--silent] [maintainer-update-dist]" >&2
    exit 1
}

#
# check_exit_status: Print diagnostic and exit on errors.
#
check_exit_status() {
    if (( _exit_status != 0 )); then
        cat >&2 <<EOF
$pgm: Process terminated with errors.
$process_output
EOF

        exit $_exit_status
    fi
}

if [[ ."$1" = .-h* ]] || [[ ."$1" = .--h* ]]; then
    usage
fi

if [[ ."$1" = .-s* ]] || [[ ."$1" = .--s* ]]; then
    verbose='false'
    shift
fi

if [[ ."$1" = .-* ]]; then
    usage
fi

srcdir="${0%/*}"

$verbose && cat >&2 <<EOF
$pgm: Running:
  ( cd "$srcdir" && ./bootstrap-sh --silent )

EOF

process_output="$( cd "$srcdir" && ./bootstrap-sh --silent )"
_exit_status=$?

$verbose && cat >&2 <<EOF
$pgm: Running:
  "$srcdir/configure" $(ed --info | tail -n +3 | tr -d '\n')

EOF

process_output=$("${srcdir}/configure" $(ed --info | tail -n +3 | tr -d '\n'))
_exit_status=$?
check_exit_status


# If no args, try just compiling.
if [ $# -eq 0 ]; then
    $verbose && cat <<EOF
$pgm: Running:
  make all

EOF

    process_output=$(make all 2>&1)
    _exit_status=$?

# Otherwise, update distribution.
else
    $verbose && cat >&2 <<EOF
$pgm: Running:
  make maintainer-update-dist

EOF

    process_output=$(make maintainer-update-dist)
    _exit_status=$?
    check_exit_status

    # Update ed.pot.in
    ed - po/ed.pot <<'EOF'
/^\("Project-Id-Version:\).*/s;;\1 @PACKAGE@ @VERSION@\\n";
wq po/ed.pot.in
EOF

    process_output=$("${srcdir}/configure" $(ed --info | tail -n +3 | tr -d '\n') 2>&1)
    _exit_status=$?
fi
check_exit_status


# Try Invoking testsuite.
abs_srcdir="$(cd "$srcdir" && pwd)"

# If builddir = srcdir, use quick-check.
if [ ."$abs_srcdir" = ."$(pwd)" ]; then
    $verbose && cat >&2 <<EOF
$pgm: Running:
  make quick-check

EOF

    process_output=$(make quick-check 2>&1)
    _exit_status=$?

# Otherwise, try invoking GNU make.
elif [ -x "$(which gmake)" ]; then
    $verbose && cat >&2 <<EOF
$pgm: Running:
  gmake check

EOF

    process_output=$(gmake check 2>&1)
    _exit_status=$?
elif [ -x "$(which gnumake)" ]; then
    $verbose && cat >&2 <<EOF
$pgm: Running:
  gnumake check

EOF

    process_output=$(gnumake check 2>&1)
    _exit_status=$?
elif [[ "$(make --version | head -1)" =~ "GNU Make" ]]; then
    $verbose && cat >&2  <<EOF
$pgm: Running:
  make check

EOF

    process_output=$(make check 2>&1)
    _exit_status=$?
else
    cat >&2 <<EOF
To run testsuite, invoke \`check' target with GNU Make

EOF

fi
check_exit_status
