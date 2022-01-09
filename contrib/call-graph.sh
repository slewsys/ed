#!/usr/bin/env bash
#
# @(#) call-graph-alt.sh
#
# This script generates a call graph from a shell script. The script is
# given on the command line, and its functions are assumed to be
# defined in the format:
#
#   function-name ()
#   {
#     commands
#   }
#
# where the function name and opening and closing brackets are all on
# a line by themselves, and only these occur in the first column of
# the script.
#
: ${ED:='/usr/local/bin/ed'}
: ${FILE:='/usr/bin/file'}
: ${MKTEMP:='/usr/bin/mktemp'}
: ${PRCG:='/usr/local/bin/prcg'}
: ${RM:='/bin/rm'}
: ${SED:='/bin/sed'}

declare script_name=${0##*/}

declare input_file=$1
declare -a function_list
declare alternatives
declare tmpfile

case "$($FILE "$input_file")" in
    *"shell script"*)
        : okay
        ;;
    *)
        echo "Usage: $script_name shell-script" >&2
        exit 1
        ;;
esac

function_list=( $($SED -ne '/^[^[:space:]{}#].*()/s/ *()//p' "$input_file") )
alternates=$(IFS='|'; echo "${function_list[*]}")
tmpfile=$($MKTEMP -t $script_name)

trap '$RM -f $tmpfile; exit' 0 1 2 15

printf '# Remove comments
        g/./s/#.*//
        # For every function declaration, print the function name,
        # then script name and line number in curly brackets.
        # Filter the function body through sed, which searches for
        # and prints indented function calls.
        g/^('$alternates') *\(\)/s; *\(\);;p\
        !echo " {%%:"\
        .=\
        !echo "}"\
        +;/^}$/w !'$SED' -nE -e "s/.*('$alternates').*/  \\1/p"
        Q
'  | $ED -Esv $input_file >$tmpfile

printf '# For each line containing a calling function, make a copy (t)
        # of that line. Below the copy, combine function name,
        # script name and line number on a single line.
        g/^[^[:space:]]/t\
        .,+3j
        # For each indented function, mark the line preceding it (-ka).
        # Copy the line preceding the previous occurence of a curly bracket
        # to after the marked line.
        g/^[[:space:]]/-ka\
        ?{?-t'\''a
        # Join every pair of lines to produce function/function pairs.
        g/./+s;^; ;\
        -,.j
        # Replace sequences of spaces with tabs that prcg expects.
        ,s;[[:space:]]\{1,\};\t;g
        # Append curly brackets to lines that do not already have them.
        v/\t.*\t/s;$;\t{};
        ,p
' | $ED - $tmpfile |
    $PRCG
