![Build Status](https://github.com/slewsys/ed/actions/workflows/ed.yml/badge.svg)

# The standard Unix text editor

<!-- markdown-toc start - Don't edit this section. Run M-x markdown-toc-refresh-toc -->
**Table of Contents**

- [The standard Unix text editor](#the-standard-unix-text-editor)
- [Installation](#installation)
   - [Binary Distributions](#binary-distributions)
   - [Compiling from source](#compiling-from-source)
   - [Building from Git](#building-from-git)
   - [Building a Debian package](#building-a-debian-package)
- [Tutorials](#tutorials)
- [Extensions to the SUSv4 standard](#extensions-to-the-susv4-standard)
   - [Scrolling](#scrolling)
   - [Cut-and-Paste](#cut-and-paste)
   - [File Globbing](#file-globbing)
   - [External Filtering](#external-filtering)
   - [File Locking](#file-locking)
   - [Macros](#macros)
   - [Script Flags](#script-flags)
   - [ED Environment Variable](#ed-environment-variable)
   - [Binary Files](#binary-files)
   - [BSD Dialect](#bsd-dialect)
   - [Global Search](#global-search)
   - [Piped Input](#piped-input)
   - [SunOS Dialect](#sunos-dialect)
- [Deviations from the SUSv4 standard](#deviations-from-the-susv4-standard)
   - [Extended Regular Expressions](#extended-regular-expressions)
   - [Pattern delimiters](#pattern-delimiters)
   - [Undo within global command](#undo-within-global-command)
   - [Move within global command](#move-within-global-command)
   - [Shell command arguments](#shell-command-arguments)
- [Examples](#examples)
   - [Repeated Substitution Modifiers](#repeated-substitution-modifiers)
- [References](#references)

<!-- markdown-toc end -->

Ed is an implementation of the Unix line editor. It is 100% POSIX
compatible, 8-bit clean with 64-bit addressing. It includes the GNU
regular expression library, but can be linked against any
POSIX-compatilbe alternative. Natural Language support
requires an external _libintl_ library.

Several optional extensions to the SUSv4 standard are described
[below](#extensions-to-the-susv4-standard).
The extensions are careful not to alter `ed`'s standard behavior and
so can be safely enabled by default.

# Installation
## Binary Distributions
Some binary packages are available - see
[Releases](https://github.com/slewsys/ed/releases).

## Compiling from source
The easiest way to build from source is to run:

```shell
curl -L https://github.com/slewsys/ed/releases/download/v2.0.13/ed-2.0.13.tar.gz |
    gzip -cd |
    tar -xf -
cd ./ed-2.0.13
./configure --enable-all-extensions --with-included-regex
make
sudo make install
```

## Building from Git
Updating Natural Language translation files requires:

 - _GNU gettext_ tools and
 - _GNU libintl_ library.

Generating documentation requires:

 - a typesetting system (e.g., `groff` or `troff`),
 - _GNU texinfo_ and
 - additional tools for producing PDFs (.e.g, `texi2pdf` and `ps2pdf`).

Ruunning tests requires:

 - _GNU automake_,
 - _GNU autoconf_ and
 - _GNU libtool_.

Assuming these are available, run:

```shell
git clone https://github.com/slewsys/ed
cd ./ed
./autogen.sh
./configure --enable-all-extensions --with-included-regex
make
make check
sudo make install
```
## Building a Debian package
To build a Debian package with `gbp`:

```shell
mkdir build
cd ./build
git clone https://github.com/slewsys/ed ed-2.0.13
cd ./ed-2.0.13
git branch upstream
gbp buildpackage --git-debian-branch=main --git-upstream-tree=branch
```

# Tutorials

Brian W. Kernighan's `ed` tutorials are included as PDFs, info
documents and NROFF manuscripts. See _doc/bwk/_ or, from within `ed`,
type:

```ed
!info ed RET m tutorial RET
```

# Extensions to and deviations from the SUSv4 standard.

This implementation of `ed` scores 100% on *The Open Group Shell and
Utilities Verification Suite of IEEE Std 1003.1-2017* when either the
environment variable **POSIXLY_CORRECT** is defined or `ed` is invoked
with commnad-line option **-G**.

## Extensions to the SUSv4 standard

None of the `ed` extensions discussed below are enabled by default.
They can all be enabled with `configure` option
**--enable-all-extensions**. Alternatively, individual extensions can
be enabled as described below.

### Scrolling

Extended scrolling capabilities are enabled with the `configure`
option **--enable-ed-scroll**.  These are summarized as follows:

     (.)zn - Displays next n lines from given address
     (.)Zn - Displays previous n lines from given address
     (.)]n - Displays next n / 2 lines
     (.)[n - Displays previous n / 2 lines

where *n* is a number which, if not specified, defaults to the current
window height. Half-page scrolling is presently limited to
line-oriented documents - i.e., those whose lines are shorter than the
window width.

The environment variables COLUMNS and LINES are used, if available, to
set the default window dimensions.

UTF-8 multibyte characters with East Asian Ambiguous width
attribute are displayed as narrow (i.e., occupying a single column)
per recommendation of *Unicode technical report UAX #11*.

### Cut-and-Paste

Cut-and-paste is enabled by `configure` option **--enable-ed-register**.

It is implemented by means of `ed`'s move (**m**), copy (**t**) and
delete (**d**) commands:

     (.,.)m>  - Moves address range to unnamed register (overwriting
                any previous contents).
     (.,.)t>  - Copies address range to unnamed register (overwriting
                any previous contents).
     <m(.)    - Moves unnamed register contents to after given address.
     <t(.)    - Copies unnamed register contents to after given address.

Comparing the syntax of cut-and-paste commands with `ed`'s move and
copy commands:

     (.,.)m(.) - Moves address range to after given address.
     (.,.)t(.) - Copies address range to after given address.

evidently the redirection operator **<** reads from the unnamed
register and **>** writes to the unnamed register.

Deleted lines are automatically moved to - and overwrite the contents
of - the unnamed register. So, for example, after deleting lines 1
through 10, they can be restored from the unnamed register to the end
of buffer via the `ed` command sequence:

     1,10d
     <m$

Named registers are also supported: **<n** reads from register *n*,
where *n* is an integer in the range [**0** ... **9**], and **>n** writes to
register *n*.

Lines can be appended to registers using the syntax **>>n** or **>>**,
in the case of the unnamed register.

Finally, it's possible to move and copy the contents of registers
directly to other registers. For instance, to expand on the example
above, after deleting lines 1 through 10, let's move them from the
unnamed register to register 5 and then later restore the lines from
register 5 to the end of the buffer as follows:

    1,10d
    <m>5
    ...
    <5m$

### File Globbing

File globbing is enabled by the `configure` option
**--enable-file-globbing**. This doesn't turn `ed` into a multi-file
editor, but it allows `ed` to be invoked with multiple file arguments
which are maintained in a list and introduces variants of existing
commands accepting glob(3) file patterns. File globs are constructed
with the following symbols:

    *      - matches any part of file name, except for a leading
             dot (**.**).
    ?      - matches any single character in a file name, except for
             a leading dot (**.**).
    [...]  - matches any character in the string represented by the
             ellipsis (**...**).
    [!...] or [^...]
           - matches any character not in the string represented by
             the ellipsis (**...**).
    [x-y]  - matches any character in the range bounded by characters
             x and y.
    [^x-y] - matches any character not in the range bounded by
             characters x and y.
    ~/     - expands to the current user's home directory, but only
             if used as a prefix.

For example, the file glob __~/*.txt__ expands to files in the current
user's home directory with suffix **.txt**.

The new commands are summarized as follows:

    ~e file-glob [...]
               - Sets the file list to the expansion of file-glob and
                 the default file to the first in the list, then
                 edits that file and prints its name to standard
                 output. Any previous file list and/or buffer contents
                 are discarded.

                 If file-glob is not specified, then sets the default
                 file to the first in the current file list and edits
                 that file.
    ~E file-glob [...]
               - Unconditionally edits the first file in the file
                 list. Similar the **~e** command except that
                 unwritten changes are discarded without warning.
    ~en        - Edits the "next" file in the file list and prints
                 its name to standard output. Any previous buffer
                 contents are discarded.
    ~ep        - Edits the file that comes "previous" in the file
                 list  and prints its name to standard output. Any
                 previous buffer contents are discarded.
    ($)~r file-glob
               - Reads the file (uniquely) matching file-glob to after
                 the addressed line. If file-glob is not specified,
                 then the default file is used.
    (1,$)~w file-glob
               - Writes the addressed lines to the file (uniquely)
                 matching file-glob. If file-glob is not specified,
                 then the default file is used. The default file name
                 is unchanged.
    (1,$)-W file-glob
               - Appends the addressed lines to the file (uniquely)
                 matching file-glob. If file-glob is not specified,
                 then the default file is used. The default file name
                 is unchanged.
    (1,$)~wn   - Writes the addressed lines to the default file, then
                 edits the file that comes "next" in the file list.
    (1,$)~wp   - Writes the addressed lines to the default file, then
                 edits the file that comes "previous" in the file list.
    ~f file-glob [...]
               - Sets the file list to the expansion of file-glob,
                 sets the default file to the first file in list, and
                 prints the file list to standard output. The editor
                 buffer is unchanged.
    ~fn        - Sets the default file name to the "next" in the file
                 list and prints the name to standard output. The
                 contents of the editor buffer are unchanged.
    ~fn        - Sets the default file name to the "previous" in the
                 file list and prints the name to standard output.
                 The contents of the editor buffer are unchanged.

When a file-glob is used in the above commands, if no files match,
then the unexpanded file-glob is used instead.

For the read (**~r**) and write (**~w**) variants to succeed,
file-glob must expand to at most one file. Otherwise, these fail with
diagnostic *Too many file names*.

If the first character of a file argument is exclamation mark (**!**),
then the rest of the line is interpreted as a shell command. In this
case, backslash (**\\**) escape processing is limited to protecting
percent signs (**%**) from being expanded to the default file name.

In contrast, when opening a file, backslash escape processing is
limited to protecting an initial exclamation mark (**!**), e.g., `ed
\!date` opens the file named _!date_, whereas `ed '!date'` reads the
output of the Unix `date` command into the editor buffer. A more
portable way of opening a file whose name begins with an exclamation
marks is to specify all or part of the pathname, e.g. `ed ./'!date'`,
or within ed: `~e ./!date`.

### External Filtering

The ability to filter lines through an external filter is enabled with
the `configure` option **--enable-external-filtering**. This is
summarized as follows:

     (n, m)!shell-command

where lines **n** through **m** are written to the standard input of
any Unix command, **shell-command**, whose standard output replaces
the range of lines in the editor buffer. Unlike most other `ed` commands,
at least one address must be specified. Otherwise a shell command is
run, but no filtering is done.

For example, to use the Unix transpose utility, `tr`, to convert a
line to uppercase:

```shell
$ ed -p '*'
*a
hello, world
.
*.! tr a-z A-Z
13
*p
HELLO, WORLD
```

### File Locking

File locking is enabled by `configure` option **--enable-file-lock**.
Advisory locking is provided by _flock (2)_, if available, otherwise
_fcntl (2)_. If help mode is enabled (i.e., if either `ed` is invoked
with command-line option **-v** or, within `ed`, command **H** is
issued), then reading or writing a locked file prints a diagnostic to
standard error. For historical compatibility, no errors are flagged.

### Macros

Macros are collections of `ed` scripts stored in _registers_ that can
be run against the editor buffer using the syntax: **@n**
where **n** is a register number. Macros are enabled with the
`configure` option **--enable-ed-macro**.

Here's an example session that loads and executes an ed script:

```shell
$ ed -p '*'          <-- Prompt for commands with '*'.
*r contrib/cats.ed   <-- Read a script from the ./contrib directory.
683
*,m>1                <-- Move it to register 1.
*r COPYING           <-- Read in another file with lots of blank lines.
55935
*@1                  <-- Run the script in register 1.
...
*wq COPYING          <-- A wq command alone would overwrite cats.ed!
55932                <-- Saved file is smaller by three newlines.
$
```

### Script Flags

`ed` dropped the programming constructs of its ancestor, `QED`, that
were later adopted by `sed`, but it's REPL interface and random access
addressing still prove useful on occasion. Additional command-line
flags for scripting are enabled by `configure` option
***--enable-script-flags***. These are summarized as follows:

    -i, --in-place[=SUFFIX]  Write file before closing, optionally
                             back up the original.
    -e, -expression=COMMAND  Add COMMAND to script input - implies -s.
    -f, --file=SCRIPT        Read commands from file SCRIPT - implies -s.

The flag **-f** enables stand-alone `ed` scripts.  For example:

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
# Avoid buffer-modified warning by quitting unconditionally.
Q
```

Flags **-i** and **-e** are also borrowed from  `sed`.  The `sed` command:

```shell
sed -i -e 's/old/new/' file
```

in `ed` dialect becomes:

```shell
ed -i -e ',s/old/new/' file
```

Note the difference here: `sed` commands are applied to every input
line by default, whereas `ed` requires an explicit range.

Each `ed` expression argument is placed on a line by itself. So the
`ed` script:

    a
    hello
    .
    s/x*/!/gp
    Q

could be written on the command line as:

```
ed -e 'a' -e 'hello' -e '.' -e 's/x*/!/gp' -e 'Q'
```

### ED Environment Variable

The `ed` environment variable, **ED**, is enabled with the `configure`
option **--enable-ed-envar**. Command-line options can then be enabled
automatically. For example, adding a line to one's shell profile such
as:

    export ED='-vp *'

provides `ed` with a command prompt (__*__) and enables help mode.

### Binary Files

When a file containing at least one ASCII **NUL** character is written,
a newline is not appended if it did not already contain one upon
reading. In particular, reading */dev/null* prior to writing prevents
appending a newline to a binary file since */dev/null* contains no
newline.

For example, to create a file with `ed` containing a single **NUL**
character:

```shell
$ ed -p '*'
*a
^@
.
*r /dev/null
0
*wq junk
1
$
```

Similarly, to remove a newline from the end of a 1k binary file *bin*:

```shell
$ ed -p '*' bin
1024
*r /dev/null
*wq
1023
$
```

### BSD Dialect

BSD dialect has been implemented wherever it does not conflict
with the SUSv4 standard. This includes the following commands:

```shell
(.,.)s[rgpn]     - to repeat a previous substitution,
(1,$)W           - for appending text to an existing file,
(1,$)wq          - for exiting after a write,
(.)z[n]          - for scrolling through the buffer, and
```

BSD line-addressing syntax - i.e., **^** as synonym for **+** and
**%** as synonym for **1,$**.

### Global Search

The SUSv4 interactive global commands **G** and **V** are extended to
support multiple commands, including **a**, **i** and **c**. The command
format is the same as for the global commands **g** and **v**, i.e., one
command per line with each line, except for the last, ending in
backslash (**\\**).

### Piped Input

For backwards compatibility, errors in piped scripts do not force ed
to exit. SUSv4 only specifies `ed`'s response for input via regular
files (including here documents) or standard input.

### SunOS Dialect

For SunOS `ed` compatibility, `ed` runs in restricted mode if
invoked as red. This limits editing of files in the local directory
only and prohibits shell commands.

## Deviations from the SUSv4 standard

### Extended Regular Expressions

Extended regular expression syntax is available if `ed` is invoked
with command-line flag `-E` or `-r`.

### Pattern delimiters

To support the BSD **s** command (see
[Repeated Substitution Modifiers](#repeated-substitution-modifiers)
below), substitution
patterns cannot be delimited by numbers or the characters **r**, **g**
and **p**. In contrast, SUSv4 specifies that any character other than
space or newline can used as a delimiter.

### Undo within global command

Since the behavior of undo (**u**) within a global (**g**) command
list is not specified by SUSv4, `ed` follows the behavior of the SunOS
`ed`: undo forces a global command list to be executed only once,
rather than for each line matching a global pattern. In addtion, each
instance of **u** within a global command undoes all previous commands
(including undo's) in the command list. Alternative approaches seem
either too complicated to implement or too confusing to use.

### Move within global command

The move (**m**) command within a global (**g**) command list also
follows the SunOS `ed` implementation: any moved lines are removed
from the global command's _active_ list.

### Shell command arguments

If `ed` is invoked with a name argument prefixed by exclamation mark
(**!**), then the remainder of the argument is interpreted as a shell
command. To protect the command from interpretation by the shell, it
should be quoted. For example,

```shell
$ ed -p '*' '!echo "hello, world"'
12
*,p
hello world
*
```

In the previous example, note that the default file name is not set, i.e.,

```shell
*f
*
```

## Examples

### Repeated Substitution Modifiers

 | Sequence | Effect | Explanation                              |
 | :--      |:--     |:--                                       |
 | s;a;x    |        | Repeated substitution command (**s**)    |
 | s;b;y    |        | always repeats most recent substitution. |
 | s        | s;b;y  |                                          |


| Sequence | Effect | Explanation                           |
| :--      |:--     |:--                                    |
| s;a;x    |        | Intermediate search commands (**/b**) |
| /b       |        | do not affect regexp of repeated      |
| s        | s;a;x  | substitution command (**s**).         |


| Sequence | Effect | Explanation                                |
| :--      |:--     |:--                                         |
| s;a;x    |        | Repeated substitution with regexp modifier |
| /b       |        | (**r**) uses most recent regexp, i.e., of  |
| sr       | s;b;x  | intermediate search (**b**).               |


| Sequence | Effect | Explanation                                |
| :--      |:--     |:--                                         |
| /a       |        | Repeated substitution with regexp modifier |
| s;b;x    | s;b;x  | (**r**) uses most recent regexp, i.e., of  |
| sr       | s;b;x  | last substitution (**b**).                 |


| Sequence | Effect | Explanation                                   |
| :--      |:--     |:--                                            |
| s;a;x    |        | Repeated substitution with regexp modifier    |
| s;b;%    | s;b;x  | (**r**) picks up regexp from last search      |
| /c       |        | (**c**), not from repeated substitution       |
| s        | s;b;x  | command (**s**). Effect of modifier preserved |
| sr       | s;c;x  | by subsequent repeated substitution.          |
| s        | s;c;x  |                                               |


| Sequence | Effect    | Explanation                               |
| :--      |:--        |:--                                        |
| s;a;x;g  |           | Toggling effect of repeated substitution  |
| s        | s;a;x;g   | modifier (**g**) on repeated substitution |
| sg       | s;a;x;    | command (**s**).                          |
| sg       | s;a;x;g   |                                           |


| Sequence  | Effect    | Explanation                                |
| :--       |:--        |:--                                         |
| s;a;x;2   |           | Repeated substitution with match selection |
| s         | s;a;x;2   | modifier (**3**) overrides any previous    |
| s3        | s;a;x;3   | match selection (**2**).                   |


| Sequence  | Effect    | Explanation                                 |
| :--       |:--        |:--                                          |
| s;a;x;2g  |           | Repeated substitution with global and       |
| sg        | s;a;x;2   | match selection modifiers (**g** and **3**) |
| sg        | s;a;x;2g  | operate independently of each other.        |
| s3        | s;a;x;3g  | NB: **s;a;x;2g** substitutes globally after |
| s4g       | s;a;x;4   | the second match, whereas **s;a;x;g2**      |
| sg        | s;a;x;4g  | substitutes every other match.              |


| Sequence  | Effect    | Explanation                                 |
| :--       |:--        |:--                                          |
| s;a;x;4g3 |           | Repeated substitution with global modifier  |
| sg        | s;a;x;4   | (**g**) toggles a global modulus (**g3**),  |
| sg        | s;a;x;4g3 | but a new global modulus modifier (**g2**)  |
| sg2       | s;a;x;4g2 | overrides the old (**g3**).                 |
| s3        | s;a;x;3g2 |                                             |


| Sequence  | Effect    | Explanation                                 |
| :--       |:--        |:--                                          |
| s;a;x;gl  |           | Print suffix (**l**) is toggled by repeated |
| sp        | s;a;x;g   | substitution print modifier (**p**).        |
| sp        | s;a;x;gl  |                                             |

# References

The `ed` algorithm is described in Kernighan and Plauger's book
_Software Tools in Pascal_, Addison-Wesley, 1981.

Brian W. Kernighan's `ed` tutorials are included courtesy of _Lucent
Laboratories_.

Please submit issues or pull requests to: <https://github.com/slewsys/ed>
