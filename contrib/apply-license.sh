#!/bin/sh
#
# @(#) apply-license
#
# This script inserts a requested license in ed source files.
#
declare script_name=${0##*/}
declare usage="Usage: apply-license bsd|mit|gpl2|gpl3"

if (( $# != 1 )); then
    echo "$usage"
    exit 0
fi

# Change to top_srcdir.
while test ! -f COPYING && test ."$PWD" != .'/'; do
    cd ..
done
if test ! -f COPYING; then
    echo "$script_name: COPYING: No such file or directory"
    exit 1
fi

case "$1" in
    bsd)
        license_requested='BSD-2-Clause'
        ;;
    mit)
        license_requested='MIT'
        ;;
    gpl2)
        license_requested='GPL-2.0'
        ;;
    gpl3)
        license_requested='GPL-3.0'
        ;;
    *)
        echo "$1: Unrecognized license"
        echo "$usage"
        exit 1
        ;;
esac

license_text=$(ed - COPYING <<EOF
/^===.*$license_requested/;/Copyright/;/^===/-p
Q
EOF
            )

#for src in src/*.[ch]; do
for src in src/o.c; do
    ed - $src <<EOF
/Copyright.*SlewSys/-ka
/This file is part of ed/+kb
'a+,'b-c
$license_text
.
'a+,'b-s/^/ * /
a
 *
 * This file is part of ed.
.
w
q
EOF
done
