SUMMARY
=======

As `ed` approaches it's 50th birthday, it's our pleasure to introduce
an updated version that we hope makes it attractive to modern users.
Over the years, `ed`, the standard Unix text editor, has been
variously described as "the most user-hostile editor ever created", a
test of one's sense of humor and an editor for those "who can remember
what they're working on". To address these criticisms, a small
collection of extensions is introduced. Yet the extended editor is
still `ed`: Indeed, it's the only Free Software version currently
available to score 100% on POSIX compliance tests and is intended as a
drop-in replacement for all variants of `ed`. And though still
concise, it needn't be hostile: Every extension introduced in the
update is both optional and unobtrusive.

INTRODUCTION
============
`ed` is line oriented because it was developed before video monitors
were widespread. And while teleprinters that `ed` was developed for
have a history going back to the mid-nineteenth century, the interface
is still relevant in today's cloud-computing environments, which
typically deploy and manage resources on the command line. So in the
course of our own work, we settled on a minimal set of extensions to
help make `ed` more productive:

  * Forward and backward scrolling,
  * Cut-and-paste,
  * File globbing,
  * Processing with external filters,
  * File locking,
  * Executable registers, or "macros",
  * Command-line flags for extended regular expressions (`-E` or `-r`),
    in-place editing (`-i`), scripting expressions (`-e EXPRESSION`) and
    script files (`-f SCRIPT-FILE`),
  * Default command-line arguments in environment variable `ED`,

In the remainder of this article, we'll explore these extensions in
more detail. And since a whole generation has probably never been
exposed to `ed`, we'll offer some tips for successful scripting.

EXTENSIONS
==========
`ed` extensions are not enabled by default. When `configure` is run,
all extensions can be enabled by `configure` option
`--enable-all-extensions`. Alternatively, individual extensions can be
enabled as described below.

SCROLLING
---------
Scrolling is enabled by `configure` option `--enable-ed-scroll`.

SYNOPSIS

```
(.)zn - Display next n lines from given address (default n: window height)
(.)Zn - Display previous n lines from given address (default n: window height)
(.)]n - Display next n / 2 lines (default n: window height)
(.)[n - Display previous n / 2 lines (default n: window height)
```

A principal advantage of screen-oriented editors over traditional `ed`
is the ability to display the context of one's work. So `ed` has been
extended with forward and backward paging. For line-oriented
documents - i.e., those whose lines are shorter than the width of the
display - `ed` also supports half-page scrolling.

UTF-8 multibyte characters with East Asian Ambiguous width attribute
are displayed as narrow (i.e., occupying a single column) per
recommendation of Unicode technical report UAX #11. If `ed` should
support an option to display these characters as wide, please let us
know!

CUT-AND-PASTE
-------------
Cut-and-paste is enabled by `configure` option `--enable-ed-register`.

(BASIC) SYNOPSIS

```
(.,.)m>  - Move address range to unnamed register (default range: dot)
(.,.)t>  - Copy address range to unnamed register (default range: dot)
<m(.)    - Move unnamed register contents to after given address (default address: dot)
<t(.)    - Copy unnamed register contents to after given address (default address: dot)
```

Cut-and-paste is so fundamental to display editing that it compels
`ed` to support it too. Rather than introduce new commands,
cut-and-paste is implemented by means of `ed`'s move (`m`), copy (`t`)
and delete (`d`) commands. Comparing the syntax of cut-and-paste
commands with `ed`'s move and copy commands:

```
(.,.)m(.) - Move address range to after given address.
            (default range: dot, default destination: dot)
(.,.)t(.) - Copy address range to after given address.
            (default range: dot, default destination: dot)
```

evidently the redirection operator `<` reads from the unnamed register
and `>` writes to the unnamed register.

Deleted lines are automatically moved to the unnamed register, so to
cut-and-paste lines 1 through 10 to the end of buffer, one could use
the command sequence:

```
1,10d
<m$
```

`ed` also supports named registers: `<n` reads from register `n`, and
`>n` writes to register `n` for `n` in the range 0 ... 9. Finally,
registers can be appended to using the syntax `>>n`.

FILE GLOBBING
-------------
File globbing is enabled by `configure` option `--enable-file-glob`.

SYNOPSIS

```
~e [file-glob]     - Set file list to expansion of glob. Edit first file.
                     With no arguments, edit first file in list.
en                 - Edit next file in list.
(1,$)~w file-glob  - Write range to (single) file expansion of glob.
(1,$)wn            - Write range to current file, then edit next in list.
($)~r file-glob    - Read (single) expansion of glob to after addressed line.
~f                 - Display list of available files to edit
```

File globbing allows `ed` to work with a list of files in a manner
similar to `vi`. When multiple files are given on the command line,
their names are stored in a list, the first of which `ed` opens for
editing. The command `~e` with no arguments reopens the first file in
the list - i.e., it's equivalent to `vi`'s `:rewind` command.

The tilde (`~`) prefix is used for compatibility with traditional
`ed`. So whereas the ed command `e hello world` opens a file named
_hello world_, the command `~e hello world` opens two files named
_hello_ and _world_. To include spaces in a file glob, escape the
spaces with a backslash character (`\`), e.g., `~e hello\ world`.

EXTERNAL FILTERING
------------------
External filtering is enabled by `configure` option `--enable-external-filter`.

SYNOPSIS

```
(.,.)! external-filter
```

External filtering is another carry over from `vi`. It writes a
range of lines to the standard input of an external filter, whose
standard output overwrites the given range in the editor buffer.

SCRIPTING provides an example of filtering a range of lines
through `sed` from within `ed`.

FILE LOCKING
------------
File locking is enabled by `configure` option `--enable-file-lock`.

SYNOPSIS

```
ed -v
```

With locking enabled, `ed` opens files for editing with an exclusive
advisory lock. If a file is already locked by another process when
`ed` opens it and `ed` is in help mode (either by invoking `ed` with
command-line option `-v` or by `ed` command `H`), a warning is printed
to standard error.

While the `nvi` editor uses advisory file locks, `Emacs` and `Vim`
appear to use nonstandard locking methods.

MACROS
------
`ed` macros are enabled by `configure` option `--enable-ed-macro`.

SYNOPSIS

```
<n@  - Evaluate ed script in register n for n in the range 0 ... 9.
       If n is not specified, evaluate the temporary register.
```

Note that macros are run against the editor buffer. When running `ed`
interactively, sometimes it's nice to be able to load and apply an
`ed` script. This is perhaps best illustrated with an example:

```
$ ed -p '*'          <-- Prompt for commands with '*'.
*r contrib/cats.ed   <-- Read in a script.
683
*,m>1                <-- Move it to register 1.
*r COPYING           <-- Read in another file with lots of blank lines.
55384
*<1@                 <-- Run the script in register 1.
...
*wq COPYING          <-- A `wq' command alone would overwrite contrib/cats.ed!
55380                <-- Saved file is smaller by four newlines.
$
```

If a truly programmable editor is called - one with loops and
conditionals, then `sed` is worth a look.

EXTENDED REGULAR EXPRESSIONS
----------------------------
Extended regular expression syntax is available if `ed` is invoked
with command-line flag `-E` or `-r`, following the syntax of modern
`sed` implementations. We'll use extended regular expressions in
SCRIPTING below.


IN-PLACE EDIT AND SCRIPT FLAGS
------------------------------
Command-line script flags are enabled by `configure` option
`--enable-script-flags`.

SYNOPSIS
```
ed [-i] [-e EXPRESSION] [-f FILE]
```

Just as modern implementations of  `sed` support in-place editing:

```
sed -i -e 's/old/new/' file
```

so now `ed` can do the same:

```
ed -i -e ',s/old/new/' file
```

Note the difference in syntax between `ed` and `sed`: `sed` commands
are applied to every input line by default, whereas with `ed` an
explicit range is required. Furthermore, the `ed` variant is slower
since it makes an internal, on-disk copy of its input before executing
commands.

Each `ed` expression argument is placed on a line by itself.  So the
command-line equivalent of the `ed` script:

```
a
hello
.
s/x*/!/gp
Q
```

would be:

```
ed -e 'a' -e 'hello' -e '.' -e 's/x*/!/gp' -e 'Q'
```

In addition to individual script expressions, script files can be
given on the command line with option `-f SCRIPT`. Using shebang
(`#!`) syntax, here's a script that, given a file argument, produces
the same output as `cat -s`:

```
#!/bin/ed -f
#
# @(#) cats.ed
#
# SYNOPSIS
#   cats.ed file >new
#
# DESCRIPTION
#   This script replaces a sequence of multiple newlines in a file with
#   a single newline and prints the result to the standard output.
#
# Append token (∴@∴) to end of each line.
,s/$/∴@∴/
# Join all lines
,j
# Substitue two newlines for sequences of multiple tokens not at EOF.
s;\(∴@∴\)\{2,\}\([^∴]\);\
\
\2;g
# Substitue one newline for sequences of one or more tokens.
,s;\(∴@∴\)\{1,\};\
;g
# Print the result to standard output.
,p
# Avoid  buffer-modified warning by quitting unconditionally.
Q

```

As an exercise, the script above could be improved by:

  * using extended regular expressions,
  * handling lines with only spaces the same as empty lines,
  * running the script in the context of a global command to mask
    substitution errors.

ED ENVIRONMENT VARIABLE
-----------------------
Support for environment variable `ED` is enabled by `configure` option
`--enable-ed-envar`.

SYNOPSIS

```
export ED='-vp *'
```

Since the ED environment variable is limited to command-line options,
it duplicates shell command aliasing, except that exported environment
variables are still visible in subshells where aliases are not.

SCRIPTING
=========
Does it really make sense to write `ed` scripts? It's not a general
purpose programming language. Indeed, `ed` dropped the programming
constructs of its ancestor, `QED`, that were later adopted by `sed`.
But `ed` has some distinct advantages over `sed` such as its REPL
(_Read_, _Execute_, _Print_, _Loop_) interface and random access
addressing. So arguably, it has its niche.

Furthermore, `ed` and `sed` used together are more powerful than
either alone. To illustrate, we'll start with an `awk` script that
takes a shell script as input and produces a series of function call
pairs suitable for generating a call graph. Then we'll reproduce the
`awk` script using only `ed` and `sed`.

```
#!/usr/bin/env bash
#
# @(#) call-graph.sh
#
# Shell functions are assumed to be of the form:
#
#   function-name ()
#   {
#     commands
#   }
#
# where the function name and opening and closing brackets are all on
# a line by themselves.
#
: ${AWK:='/usr/bin/awk'}
: ${FILE:='/usr/bin/file'}
: ${PRCG:='/usr/local/bin/prcg'}
: ${SED:='/usr/bin/sed'}

declare script_name=${0##*/}

declare input_file=$1
declare -a function_list
declare alternatives

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

$AWK '$0 ~ /^[^[:space:]]+ *\(\)$/ {
            caller = $1
            printf "%s\t%s\t{%s %d}\n", caller, caller, FILENAME, NR
            next
      }
      $0 ~ /#.*/ { sub(/#.*/, "") }
      $0 ~ /^}$/ {
            caller = ""
            next
      }
      $0 ~ /.*('$alternates').*/ {
          if (caller != "") {
              callee = gensub(/.*('$alternates').*/, "\\1", 1)
              printf "%s\t%s\t{}\n", caller, callee
          }
      }' $input_file |
    $PRCG
```

The utility `prcg` is available as part of a `cflow` implementation
[cflow-2.0](http://www.ibiblio.org/pub/Linux/devel/lang/c/cflow-2.0.tar.gz).
It prints a hierarchical graph given pairs of nodes representing the
edges of a directed acyclic graph.

The script `call-graph.sh` run against input file _o.sh_:
```
#!/bin/sh

func_a ()
{
  ...
}

func_b ()
{
  func_a
  ...
}

func_c ()
{
  func_b
  ...
}

func_c
```

prints to standard output:

```
1	func_c {o.sh:14}
2		func_b {o.sh:8}
3			func_a {o.sh:3}
```

The `awk` script implements a state machine that prints
function/function pairs when it encounters a function declaration and,
inside function bodies, calling-function/called-function pairs. These
are then piped to `prcg` to produce the resulting call graph.

The `ed`/`sed` version of the script follows:

```
#!/usr/bin/env bash
#
# @(#) call-graph-alt.sh
#
# Shell functions are assumed to be of the form:
#
#   function-name ()
#   {
#     commands
#   }
#
# where the function name and opening and closing brackets are all on
# a line by themselves.
#
: ${ED:='/usr/local/bin/ed'}
: ${FILE:='/usr/bin/file'}
: ${MKTEMP:='/usr/bin/mktemp'}
: ${PRCG:='/usr/local/bin/prcg'}
: ${RM:='/bin/rm'}
: ${SED:='/usr/bin/sed'}

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
```

The first takeaway from this exercise is that `ed` scripts aren't very
readable, so be generous with comments. Secondly, why pipe the scripts
to `ed`? Why not use heredocs? This is to avoid contention over line
continuation characters (`\`). Using `printf` and pipes for `ed`
scripts is recommended in general. Another benefit of piped scripts is
that they mask errors that would otherwise cause a script to exit.
This is a historical, though undocumented, idiom.

Another takeaway is that `ed`'s ability to harness the shell is very
powerful. One subtlety is that shell commands nested in global
commands must be on a single line. Outside of global commands, multi-line
shell scripts are okay:

```
$ ed -p '*'
*r !for i in 1 2 3; do \
      printf "\%02d\n" $i \
    done
!
9
*,p
01
02
03
```

Finally, we're not endorsing `ed` for writing state machines. But with
a small number of extensions, it does wield a lot power, so don't be
shy around the humble Unix line editor!
