![Build Status](https://github.com/slewsys/ed/actions/workflows/ed.yml/badge.svg)

# The standard Unix text editor

- [Description](#description)
- [Installation](#installation)
   - [Binary Distributions](#binary-distributions)
   - [Prerequisites for building from source](#prerequisites-for-building-from-source)
      - [AlmaLinux/RockyLinux](#almalinuxrockylinux)
      - [Fedora](#fedora)
      - [RHEL](#rhel)
      - [OmniOS](#omnios)
      - [OpenSUSE](#opensuse)
      - [Debian/Ubuntu](#debianubuntu)
      - [FreeBSD](#freebsd)
      - [OpenBSD](#openbsd)
      - [macOS](#macos)
   - [Building from Source](#building-from-source)
   - [Building from Git](#building-from-git)
   - [Building RPM and Debian packages](#building-rpm-and-debian-packages)
- [Tutorials](#tutorials)
- [Extensions to the POSIX.1-2024 standard](#extensions-to-the-posix1-2024-standard)
   - [Command-line address arguments](#command-line-address-arguments)
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
- [Deviations from the POSIX.1-2024 standard](#deviations-from-the-posix1-2024-standard)
   - [Extended Regular Expressions](#extended-regular-expressions)
   - [Pattern delimiters](#pattern-delimiters)
   - [Undo within global command](#undo-within-global-command)
   - [Move within global command](#move-within-global-command)
   - [Shell command arguments](#shell-command-arguments)
- [Examples](#examples)
   - [Repeated Substitution Modifiers](#repeated-substitution-modifiers)
   - [Reworked Sed Examples](#reworked-sed-examples)
      - [Joining Lines](#joining-lines)
      - [Centering Lines](#centering-lines)
      - [Incrementing Numbers](#incrementing-numbers)
      - [Printing Shell Environment](#printing-shell-environment)
      - [Reversing Characters of Lines](#reversing-characters-of-lines)
      - [Adding Headers](#adding-headers)
      - [Reversing Lines of Files](#reversing-lines-of-files)
      - [Numbering Lines](#numbering-lines)
      - [Squeezing Blank Lines](#squeezing-blank-lines)
   - [Integrating Ed, Sed and Perl](#integrating-ed-sed-and-perl)
- [References](#references)

## Description

Ed is an implementation of the standard Unix editor. Several
(optional) extensions to the POSIX.1-2024 standard are available as described
[below](#extensions-to-the-susv4-standard). The extensions are careful
not to alter `ed`'s traditional behavior and so can be safely enabled
by default.

## Installation
### Binary Distributions

Some binary packages are available - see
[Releases](https://github.com/slewsys/ed/releases).

### Prerequisites for building from source

To build `ed` from source, the following packages are needed:

 - **GNU** `autoconf`,
 - **GNU** `automake`,
 - **GNU** `autopoint` (if not provided by gettext),
 - **GNU** `gettext`,
 - **GNU** `libtool`,
 - **GNU** `make`, and
 - `openssl`.

Additional packages used for testing and generating PDFs of Brian W.
Kernighan's `ed` tutorials are:

 - **GNU** `Texinfo`,
 - **TeXLive** or **MacTeX**,
 - **GNU** `groff`,
 - `ncal`,
 - `valgrind`, and
 - `zstd`.

#### AlmaLinux/RockyLinux

On Cenots-based systems, prerequisite packages can be installed by
running (with root privileges):

```shell
dnf config-manager --set-enabled crb
dnf install -y --refresh autoconf automake gawk gcc gettext-devel git \
    glibc-gconv-extra groff libtool openssl-devel texinfo \
    texinfo-tex zstd
```

#### Fedora

On Fedora, prerequisite packages can be installed by running (with
root privileges):

```shell
dnf update --refresh
dnf install -y autoconf automake gcc gettext-devel gawk git \
    glibc-gconv-extra groff libtool openssl-devel texinfo \
    texinfo-tex zstd
```

#### RHEL

Red Hat Enterprise Linux 9 and 10 (beta) present hurdles due to a
compromised `perl` binary and (presumably therefore) no `Texinfo` package.
Begin by installing what is available (with root privileges):

```
dnf install -y \
    https://dl.fedoraproject.org/pub/epel/epel-release-latest-9.noarch.rpm
dnf install -y --refresh autoconf automake curl gawk gcc gettext-devel git \
    glibc-gconv-extra groff libtool ncurses-devel openssl-devel \
    texlive-scheme-basic zstd
```

Now build `perl` and `Texinfo` (preferably) as a non-privileged user.
In the following, this is done by installing the asdf version manager.
If golang is already installed, run:

```shell
go install github.com/asdf-vm/asdf/cmd/asdf@latest
export PATH=$(go env GOBIN):${PATH}
```

Otherwise, run:

```shell
asdf_url=https://github.com/asdf-vm/asdf
latest_version=$(
    git ls-remote --sort="-v:refname" "$asdf_url" |
        sed -e '1{s^.*/^^; q}'
        )
release_url=${asdf_url}/releases/download/${latest_version}/asdf-${latest_version}-linux-amd64.tar.gz
mkdir -p "${HOME}/bin"
curl -sSL "$release_url" | tar -C "${HOME}/bin" -zxf -
export PATH=${HOME}/.asdf/shims:${PATH}:${HOME}/bin
```

Then use `asdf` to install `perl`:

```
asdf plugin add perl
latest=$(asdf latest perl)
asdf install perl $latest
(cd ~; asdf set perl $latest)
```

To build `Texinfo`, run:

```
git clone https://git.savannah.gnu.org/git/texinfo.git
cd ./texinfo
./autogen.sh
./configure
make -j$(nproc)
```

Finally, as user root, install `Texinfo`:

```
make install
```

#### OmniOS

On OmniOS, prerequisite packages can be installed by running (with root privileges):

```shell
pkg install \
    developer/build/autoconf \
    developer/build/automake \
    developer/gcc14 \
    developer/build/gnu-make \
    developer/build/libtool \
    text/gnu-gettext \
    system/library/iconv/unicode \
    system/library/iconv/extra \
    library/security/openssl-3 \
    text/groff \
    ooce/text/texinfo \
    ooce/application/texlive \
    versioning/git \
    compress/zstd
```

To expose `TeX` and `Texinfo` binaries, extend PATH variable with, e.g.:

```
export PATH=${PATH}:/opt/ooce/bin:/opt/ooce/texlive/bin
```

#### OpenSUSE

On OpenSUSE, the prerequisite packages can be installed by running (with root privileges):

```shell
zypper --non-interactive install -y autoconf automake gcc \
    gettext-tools groff libopenssl-3-devel libtool make \
    makeinfo texinfo texlive-texinfo zstd
```

#### Debian/Ubuntu

On Debian/Ubuntu systems, the prerequisite packages can be installed
by running (with root privileges):

```shell
apt update
apt install -y  autoconf automake autopoint gcc gettext git \
    groff libssl-dev libtool make ncal texinfo texlive-binaries zstd
```

#### FreeBSD

On FreeBSD, the prerequisite packages can be installed by running (with root privileges):

```shell
pkg install -y autoconf automake gmake libtool gettext-tools \
    openssl groff texinfo texlive-full git
```

#### OpenBSD

On OpenBSD, the prerequisite packages can be installed by running (with root privileges):

```shell
pkg_add autoconf automake gmake gettext-tools git libtool \
    openssl  groff texinfo
```

If autoconf, say, v2.72, and automake v1.16 were selected, run:

```
export AUTOCONF_VERSION=2.72
export AUTOMAKE_VERSION=1.16
```

#### macOS

With Apple's Xcode and Command-line Utilities installed, along with a
package manager like [MacPorts](https://www.macports.org/) (for
multi-user systems) or [Homebrew](https://brew.sh/), run:

```
PKGMGR install autoconf automake bash gettext groff openssl \
    libtool pkgconf texinfo texlive zstd
hash -r
```

where **PKGMGR** should be replaced with `sudo port` for MacPorts and
`brew` for HomeBrew.

### Building from Source

GitHub's provided "Source code" downloads are missing the GNU
Autotools-generated files. So with these, it's necessary to run
`./autogen.sh` before `./configure`. If GNU Autotools is not
available, then go with the `zstd` archive instead.

With prerequisites installed per above, download and extract the `ed` source archive from
[Releases](https://github.com/slewsys/ed/releases), e.g.:

```
curl -sSL https://github.com/slewsys/ed/releases/download/v2.1.1/ed-2.1.1.tar.zst |
   zstd -dc - |
   tar -xf -
```

On systems other than macOS, configure, compile and test, e.g.:

```
cd ./ed-2.1.1
./configure --enable-all-extensions
gmake
gmake check
```

On macOS, change the `./configure ..` line above to:

```
./configure --enable-all-extensions CFLAGS="$(pkgconf --cflags libssl)" \
    LDFLAGS="$(pkgconf --libs libssl)"
```

Extensions can be individually enabled or disabled. To enable all
extensions except encryption, for example, run `configure` as follows:

```
./configure --enable-all-extensions \
    --disable-ed-encryption
```

### Building from Git

With prerequisites installed per above, on systems other than macOS, run:

```shell
git clone https://github.com/slewsys/ed.git
cd ./ed
./autogen.sh
./configure --enable-all-extensions
gmake
gmake check
```

On macOS, change the `./configure ..` line above to:

```
./configure --enable-all-extensions CFLAGS="$(pkgconf --cflags libssl)" \
    LDFLAGS="$(pkgconf --libs libssl)"
```

Install with root privileges:

```
gmake install
```

### Building RPM and Debian packages

If the requisite OCI container infrastructure is available (currently,
`podman` and `buildah` are required along with `qemu-user-static` and
`qemu-system` for non-native architectures), the top-level Makefile
has targets for building RPM and Debian packages as follows. After
configuring the source per above, specify the desired architecture and
package type, e.g. for RPMs:

```
make amd64-rpm
```

and for Debian packages:

```
make amd64-deb
```

RPMs can be built with architectures *amd64* and *arm46*.

Deb packages can be built for *amd64*, *arm64* and *arm*.

Distribution packages are saved under the *pkgs* subdirectory, e.g.,
*pkgs/Feodra/amd64*.

Debian packages ~~are signed with the host ${USER}'s GNU gpg secret~~
are no longer signed by default since GNU gpg `keyboxd` blocks access
to the host's ~/.gnupg inside a container. There is a workaround: export
public keys, disable keyboxd and then import public keys (see:
[no automatic migration](https://github.com/gpg/gnupg/blob/42ee84197695aca44f5f909a0b1eb59298497da0/README#L134C2-L145)
for details). Then remove `gbp` argument `--no-sign` in
*${top_srcdir}/pkgs/Debian/Containerfile.in*. Name and email address,
as provided by `git config get`, are passed as arguments to the build
container.

All [ed v2.1.1 GitHub assets](https://github.com/slewsys/ed/releases/tag/v2.1.1)
are created using GNU `parallel`:

```
parallel make ::: {amd64,arm64}-{rpm,deb} arm-deb
```

## Tutorials

Brian W. Kernighan's `ed` tutorials are included as PDFs, info
documents and NROFF manuscripts. See _doc/bwk/_ or, from within `ed`,
type:

```ed
! info ed <RET> m tutorial <RET>
```

## Extensions to the POSIX.1-2024 standard

This implementation of `ed` scores 100% on *The Open Group Shell and
Utilities Verification Suite of IEEE Std 1003.1-2017* when either the
environment variable **POSIXLY_CORRECT** is defined or `ed` is invoked
with commnad-line option **-G**.

With the exception of some command-line switches, none of the `ed`
extensions discussed below are enabled by default. They can all be
enabled with `configure` option **--enable-all-extensions**.
Alternatively, individual extensions can be enabled (or if preceded by
**--enable-all-extensions**, disabled) as described below.

### Command-line address arguments

Command-line address arguments are enabled with the `configure` option
**--enable-address-arguments**. Valid address arguments are of the
form:

| Command-line Argument | Action |
| --- | --- |
| `+N` | Set the current line (dot) to line number `N`. |
| `+/RE` | Set dot to next line matching regular expression `RE`. |
| `+?RE` | Set dot to the previous line matching regular expression `RE`. |

Address arguments can be combined, e.g.,

```shell
ed +3 +/RE1 +?RE2 FILE
```

searches *FILE* forward from line 3 for `RE1` and, from there,
backward for `RE2`.

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

Cut-and-paste is enabled by the `configure` option **--enable-ed-register**.

It is implemented by means of `ed`'s move (**m**) and copy (**t**)
commands:

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

evidently the redirection operator **<** reads from the default
register and **>** writes to the default register.

Numbered registers are also supported: **<n** reads from register *n*,
where *n* is an integer in the range [**0** ... **9**], and **>n** writes to
register *n*.

Lines can be appended to registers using the syntax **>>n** or **>>**,
in the case of the default register.

Finally, it's possible to move and copy the contents of registers
directly to other registers. For instance, after deleting lines 1
through 10 by moving them to the default register, move them from the
default register to register 5 and then later restore the lines from
register 5 to the end of the buffer as follows:

    1,10m>
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
                 list. Similar to the **~e** command except that
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
percent signs (**%**) from being expanded to the default file name or
script name (**%-**).

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

File locking is enabled by the `configure` option **--enable-file-lock**.
Advisory locking is provided by _flock (2)_, if available, otherwise
_fcntl (2)_. If help mode is enabled (i.e., if either `ed` is invoked
with command-line option **-v** or, within `ed`, command **H** is
issued), then reading or writing a locked file prints a diagnostic to
standard error. For historical compatibility, no errors are flagged.

### Macros

Macros are collections of `ed` scripts stored in _registers_ that can
be run against the editor buffer using the syntax: **(.,.)@n**
where **n** is a register number. Macros are enabled with the
`configure` option **--enable-ed-macro**.

Here's an ed session that loads a script, saves it to a register, and
then runs the script as a macro:

```shell
$ ed -p '*' src/main.c  <-- Prompt for commands with '*'.
25901                   <-- Size of src/main.c in bytes.
*kz                     <-- Mark end of file.
*r contrib/cats.ed      <-- Read a script from the ./contrib directory.
828
*-2s/^/#/               <-- Comment out the script's print statement.
*'z+,$m>                <-- Move script to the default register.
*@                      <-- Run the script in the default register.
898
*wq /dev/null           <-- Save to /dev/null and quit.
25894                   <-- Saved-file size reduced by seven bytes (newlines).
$
```

Macros can operate over a range of addresses, e.g.:

```
ed -e 'a' -e 's;_*;.;gp' -e . -e 'm>' -e ',@'  <(cal 1 395)
```


> <samp>. . . . .J.a.n.u.a.r.y. .0.3.9.5. . . . .</samp> \
> <samp>.S.u. .M.o. .T.u. .W.e. .T.h. .F.r. .S.a.</samp> \
> <samp>. . . . .1. . .2. . .3. . .4. . .5. . .6.</samp> \
> <samp>. .7. . .8. . .9. .1.0. .1.1. .1.2. .1.3.</samp> \
> <samp>.1.4. .1.5. .1.6. .1.7. .1.8. .1.9. .2.0.</samp> \
> <samp>.2.1. .2.2. .2.3. .2.4. .2.5. .2.6. .2.7.</samp> \
> <samp>.2.8. .2.9. .3.0. .3.1. . . . . . . . . .</samp> \
> <samp>. . . . . . . . . . . . . . . . . . . . .</samp>

This particular example is equivalent to the global command:

```
ed -e 'g;.;s;_*;.;gp'  <(cal 1 395)
```

However, global commands (`G`, `g`, `V`, or `v`) cannot be nested,
whereas macros can. Global commands can call macros, though. See the
[Examples](#examples) section for more demonstrations.

### Script Flags

Command-line flags adopted from `sed` for scripting are enabled by the
`configure` option ***--enable-script-flags***. These are summarized
as follows:

    --in-place, -i [SUFFIX]  Write file before closing, optionally
                             back up the original if SUFFIX provided.
    --expression, -e COMMAND Add COMMAND to script input - implies -s.
    --file, -f SCRIPT        Read commands from file SCRIPT - implies -s.
                             If SCRIPT is `-`, then commands are read
                             from standard input.

The flag **-f** enables stand-alone `ed` scripts.  For example:

```
#!/usr/bin/ed -f
#
# @(#) tac.ed
#
# DESCRIPTION
#   Reverse the order of lines in given file(s) and print to standard output.
#
g/./m0
,p
Q
```

If saved to a file, say *tac.ed*, and made executable, it can be
used as a Unix command, e.g.:

```
./tac.ed tac.ed
```

> Q \
> ,p \
> g/./m0 \
> \# \
> \#   Reverse the order of lines in given file(s) and print to standard output. \
> \# DESCRIPTION \
> \# \
> \# @(#) tac.ed \
> \# \
> \#!/usr/bin/ed -f

Note that ed scripts are not Unix *filters*, e.g., the following does
not work:

```
cat tac.ed | ./tac.ed
```

The reason is that `ed` interprets input from pipes as `ed` scripts,
not text to edited (like `sed`).  In fact, the following are equivalent:

```
./tac.ed tac.ed
cat tac.ed | ed - tac.ed
ed -f tac.ed tac.ed
```

To run `ed` in a filter context, use process substitution instead, e.g.:

```
./tac.ed <(cat tac.ed)
```


The `ed` flags **-i** and **-e** are used in the same manner as with
`sed`. The `sed` command:

```shell
sed -i -e 's/old/new/' file
```

in `ed` dialect becomes:

```shell
ed -i -e ',s/old/new/' file
```

Note the difference: `sed` commands are applied to every input
line by default, whereas `ed` requires an explicit range.

Each `ed` expression argument is placed on a line by itself. So the
`ed` script:

```
a
hello
world
.
g/./s//&\
/g
,p
```

could be written on the command line as:

```shell
ed -e 'a' -e 'hello' -e 'world' -e '.' -e 'g/./s//&\' -e '/g' -e ',p'
```

or, using the Bash shell construct $'string' to decode
backslash-escaped characters in *string*:

```shell
ed -e $'a\nhello\nworld\n.\ng/./s//&\\\n/g\n,p\n'
```

Note that this last example is equivalent to the more traditional (and
equally unreadable):

```shell
printf 'a\nhello\nworld\n.\ng/./s//&\\\n/g\n,p\n' | ed -
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

BSD dialect has been implemented wherever it does not conflict with
the POSIX.1-2024 standard. This includes the following commands:

```shell
(.,.)s[rgpn]     - to repeat a previous substitution,
(1,$)W           - for appending text to an existing file,
(1,$)wq          - for exiting after a write, and
(.)z[n]          - for scrolling through the buffer.
```

BSD line-addressing syntax - i.e., **^** as synonym for **+** and
**%** as synonym for **1,$**.

### Global Search

The POSIX.1-2024 interactive global commands **G** and **V** are
extended to support multiple commands, including **a**, **i** and
**c** as well as macros (i.e., **@n** for optional non-negative
integer n < 10). The command format is the same as for the global
commands **g** and **v**, i.e., one command per line with each line,
except for the last, ending in backslash (**\\**).

### Piped Input

For backward compatibility, errors in piped scripts do not force `ed`
to exit. POSIX.1-2024 only specifies `ed`'s response for input via regular
files (including here documents) or standard input.

### Named Pipes

Ed reads the output of named pipes in read-only mode, e.g., the
following session is valid:

```shell
ed -p '*' <(echo hello)
6
*p
hello
*f
/dev/fd/63
*
```

### SunOS Dialect

For SunOS `ed` compatibility, `ed` runs in restricted mode if
invoked as `red`. This limits editing of files in the local directory
only and prohibits shell commands.

## Deviations from the POSIX.1-2024 standard

### Extended Regular Expressions

Extended regular expression syntax is available if `ed` is invoked
with command-line flag `-E` or `-r`.

### Pattern Delimiters

POSIX.1-2024 specifies that, in substitution commands, any character
other than space or newline can used to delimit the pattern and
replacement. To support the BSD **s** command (see
[Repeated Substitution Modifiers](#repeated-substitution-modifiers)
below), substitution patterns ~~cannot~~ can be delimited by numbers
and the characters **r**, **g** and **p**, with a few exceptions.
Consider, for instance, the command:

```
ed -e 's/1/_/g7p' -e 's2_202$p' -e 's2g' <(echo 1111111111111111111111)
```
> _111111_111111_111111_ \
> _111111_111111_1111110 \
> _111111011111101111110

Here, the second expression, `s2_202$p`, is a normal substitution
using `2` as delimiter. The third expression, `s2g`, on the other
hand, is a repeated substitution which uses `2` as a match-selection
modifier. It is equivalent to the expression: `s/_/0/2gp`, i.e.,
replace every underscore (\_) after the second with zero (0), and
print the result.

There are still conflicts with POSIX where disambiguation is more
difficult. For instance:

```
POSIXLY_CORRECT=1 ed -e 's2g2' <(echo abcdefg)
```
> abcdef

whereas, without environment variable POSIXLY_CORRECT set, this
produces an error because `s2g2` is interpreted as a repeated
substitution:

```
ed -ve 's2g2' <(echo abcdefg)
```
> /dev/fd/63: File is read-only \
> ? \
> script, line 1: No previous substitution

### Undo within Global Command

Since the behavior of undo (**u**) within a global (**g**) command
list is not specified by POSIX.1-2024, `ed` follows the behavior of the SunOS
`ed`: undo forces a global command list to be executed only once,
rather than for each line matching a global pattern. In addtion, each
instance of **u** within a global command undoes all previous commands
(including undo's) in the command list. Alternative approaches seem
either too complicated to implement or too confusing to use.

### Move within Global Command

The move (**m**) command within a global (**g**) command list also
follows the SunOS `ed` implementation: any moved lines are removed
from the global command's _active_ list.

### Shell Command Arguments

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

In the previous example, note that the default file name is not set,
i.e.,

```shell
*f
*
```

## Examples

### Repeated Substitution Modifiers

| Sequence  | Effect     | Explanation                                   |
| :--       |:--         |:--                                            |
| s;a;x     |            | Repeated substitution command (**s**) always  |
| s;b;y     |            | repeats most recent substitution.             |
| s         | s;b;y      |                                               |

| Sequence  | Effect     | Explanation                                   |
| :--       |:--         |:--                                            |
| s;a;x     |            | Intermediate search commands (**/b**) do not  |
| /b        |            | affect regexp of repeated substitution        |
| s         | s;a;x      | command (**s**). Substitutions don't affect   |
| //s       | /b/s;a;x   | repeated searches either.                     |

| Sequence  | Effect     | Explanation                                   |
| :--       |:--         |:--                                            |
| s;a;x     |            | Repeated substitution with regexp modifier    |
| /b        |            | (**r**) uses most recent regexp, i.e., of     |
| sr        | s;b;x      | intermediate search (**b**).                  |

| Sequence  | Effect     | Explanation                                   |
| :--       |:--         |:--                                            |
| /a        |            | Repeated substitution with regexp modifier    |
| s;b;x     | s;b;x      | (**r**) uses most recent regexp, i.e., of     |
| sr        | s;b;x      | last substitution (**b**).                    |

| Sequence  | Effect     | Explanation                                   |
| :--       |:--         |:--                                            |
| s;a;x     |            | Repeated substitution with regexp modifier    |
| s;b;%     | s;b;x      | (**r**) picks up regexp from last search      |
| /c        |            | (**c**), not from repeated substitution       |
| s         | s;b;x      | command (**s**). Effect of modifier preserved |
| sr        | s;c;x      | by subsequent repeated substitution.          |
| s         | s;c;x      |                                               |

| Sequence  | Effect     | Explanation                                       |
| :--       |:--         |:--                                                |
| s;a;x;g   |            | Toggling effect of repeated substitution          |
| s         | s;a;x;g    | modifier (**g**) on repeated substitution         |
| sg        | s;a;x;     | command (**s**).                                  |
| sg        | s;a;x;g    |                                                   |

| Sequence  | Effect     | Explanation                                   |
| :--       |:--         |:--                                            |
| s;a;x;2   |            | Repeated substitution with match selection    |
| s         | s;a;x;2    | modifier (**3**) overrides any previous       |
| s3        | s;a;x;3    | match selection (**2**).                      |

| Sequence  | Effect     | Explanation                                   |
| :--       |:--         |:--                                            |
| s;a;x;2g  |            | Repeated substitution with global and match   |
| sg        | s;a;x;2    | selection modifiers (**g** and **3**) operate |
| sg        | s;a;x;2g   | independently of each other.                  |
| s3        | s;a;x;3g   | NB: **s;a;x;2g** substitutes every match      |
| s4g       | s;a;x;4    | after the second, whereas **s;a;x;g2**        |
| sg        | s;a;x;4g   | substitutes every other match.                |

| Sequence  | Effect     | Explanation                                      |
| :--       |:--         |:--                                               |
| s;a;x;$   |            | Match selection modifier (**$**) selects last    |
| s$-3g     | s;a;x;$-3g | match. Repeated modifer (**$-3g**) selects       |
|           |            | every match after the third from last.           |

| Sequence  | Effect     | Explanation                                   |
| :--       |:--         |:--                                            |
| s;a;x;4g3 |            | Match selection modifier (**4g3**) selects    |
| sg        | s;a;x;4    | every third match after the fourth. Repeated  |
| sg        | s;a;x;4g3  | substitution with global modifier (**g**)     |
| sg2       | s;a;x;4g2  | toggles a global modulus (**g3**), but a new  |
| s3        | s;a;x;3g2  | global modulus modifier (**g2**) overrides    |
|           |            | the old (**g3**).                             |

| Sequence  | Effect     | Explanation                                                      |
| :--       |:--         |:--                                                               |
| s;a;x;gl  |            | Print suffix (**l**) is toggled by repeated                      |
| sp        | s;a;x;g    | substitution print modifier (**p**).                             |
| sp        | s;a;x;gl   |                                                                  |

### Reworked Sed Examples

The GNU `sed` manual provides examples of scripting with `sed`. This
section reworks some of those in `ed`. Whereas the `sed` manual uses
traditional syntax, the following `ed` scripts use many of the
extensions described above. The intent of this section is to offer
some justification for the extensions as well as to illustrate their
utility.

#### Joining Lines

Goal: Join the second and third lines of *lines.txt*.

`cat lines.txt`

> hello \
> hel \
> lo \
> hello

`ed -e '2,3j' -e ',p' lines.txt`

> hello \
> hello \
> hello

---

Goal: For each line ending with a backslash, join it with all
subsequent lines until one that does not end with a backslash. Then
remove the backslashes.

`cat 1.txt`

> this \\\
> is \\\
> a \\\
> long \\\
> line \
> and another \\\
> line

`ed -e 'g;\\$;.,/[^\]$/j\' -e 's;\\;;g' -e ',p' 1.txt`

> this is a long line \
> and another line

To avoid removing backslashes within lines, ending backslashes can
first be replaced with an arbitrary token, e.g., **<@>**.

---

Goal: For all lines beginning with the a space, join them to the
previous line that does not begin with a space.

`cat 2.txt`

> Subject: Hello \
>     World \
> Content-Type: multipart/alternative; \
>     boundary=94eb2c190cc6370f06054535da6a \
> Date: Tue, 3 Jan 2017 19:41:16 +0000 (GMT) \
> Authentication-Results: mx.gnu.org; \
>        dkim=pass header.i=@gnu.org; \
>        spf=pass \
> Message-ID: <abcdef@gnu.org> \
> From: John Doe <jdoe@gnu.org> \
> To: Jane Smith <jsmith@gnu.org>

`ed -e 'g;^  *;s;; ;\'  -e '-,.j' -e ',p' 2.txt `

> Subject: Hello World \
> Content-Type: multipart/alternative; boundary=94eb2c190cc6370f06054535da6a \
> Date: Tue, 3 Jan 2017 19:41:16 +0000 (GMT) \
> Authentication-Results: mx.gnu.org; dkim=pass header.i=@gnu.org; spf=pass \
> Message-ID: <abcdef@gnu.org> \
> From: John Doe <jdoe@gnu.org> \
> To: Jane Smith <jsmith@gnu.org>

#### Centering Lines

Goal: Center lines in given files within 80-columns, i.e., prepend
each line with a number of spaces given by:

```
(80 - strlen(line)) / 2
```

```
#!/usr/bin/ed -f
#
# @(#) center-lines
#
# SYNOPSIS
#   center-lines file [...]
#
# Mark end of file to be centered.
kx
#
# Append an ed script to center a line of text.
a
#
# Trim spaces; copy line, then replace first (-) copy with resulting length.
s;^[[:space:]]*;;
s;[[:space:]]*$;;
t
-! tr -d '\n' | wc -c
#
# Wrap length in printf command that generates spaces for centering.
s;^;-r ! printf "\\%$(( (80 - ;
s;$;) / 2 ))s\\n" ''
#
# Move printf command to register No. 1 and run it.
m>1
@1
#
# Join the line of spaces and trimmed text.
.,+j
#
# End of line-centering script. Move it to register No. 2.
.
'x+,$m>2
#
# Run script in register No. 2 for each line in the file.
,@2
#
# Save the result with suffix .centered
w ! cat >%.centered
```

#### Incrementing Numbers

Goal: Increment each number in given files.

Command-line option -E is used to enable extended regular expression syntax.

```
#!/usr/bin/ed -Ef
#
# @(#) increment-numbers
#
# SYNOPSIS
#     increment-numbers file [...]
#
# Isolate each number and wrap it in a command that increments it.
g/([^0-9]*)([0-9]+([.][0-9]*)?|[.][0-9]+)/s//\1\
-r ! bc -l <<<\2+1\
/g
#
# For each command, move it to the default register, execute it, and
# then join the lines above and below.
g/^-r !/m>\
@\
-,+j
# Save the result with suffix .incremented.
w ! cat >%.incremented
```

#### Printing Shell Environment

Goal: Remove function definitions in the output of the `set` command.

`ed -e 'g/^[^ ]* () $/.,/^}/d' -e ',p' <(set)`

The shell idiom `<(command)`, called *process substitution*, creates a
named piped (i.e., */dev/fd/n*) that `ed` can open as a read-only
file.

#### Reversing Characters of Lines

Goal: Reverse lines characterwise.

```
#!/usr/bin/ed -f
,! rev
```

That's cheating, but it also happens to be three times faster
than `sed`. Actually `ed` can reverse strings the "intuitive" way:

```
ed -e 's;.;&\' -e ';g' -e 'g/./m0' -e ',jp' <(echo 'hello, world!')
```

> !dlrow ,olleh

but this is admittedly very slow even compared to `sed`'s version.

#### Adding Headers

Like GNU `sed`, `ed` can be used to safely modify multiple files at
once:

```
ed -i -e '1i' -e '/* Copyright © FOO BAR */ -e '.' *.c
```

To include multiple lines, either make each line a separate expression
or use the shell's special string syntax $'any ANSI C escape'.  The
following produce the same result:

```
ed -i -e '1i' -e '/*'  \
              -e ' * Copyright © FOO BAR' \
              -e ' * Created by ...' \
              -e ' */' *.c
```

and

```
ed -i -e '1i' -e $'/*\n * Copyright (C) FOO BAR\n * Created by ...\n */' *.c
```

Similarly, to insert an existing text file, say, LICENSE.txt:

```
ed -i -e '0r LICENSE.txt' *.c *.h
```

#### Reversing Lines of Files

Goal: Reverse the order of lines in given files.

```
#!/usr/bin/ed -f
g/./m0
w ! cat >%.tac
```

#### Numbering Lines

Goal: Add line numbers to files.

```
#!/usr/bin/ed -f
,n
```

To enumerate and view a file one page at a time in an interactive `ed`
session, use the command `1zn` and then `zn` for each page thereafter.
To go backward one page, use `Zn`. To scroll by half pages, use `]n`
(forward) and `[n` (backward). To scroll forward a single line, use
`+Zn`. To scroll backward a single line, use `-Zn`. And, of course,
`+5Zn` scrolls down five lines, etc.

#### Squeezing Blank Lines

Goal: Replace sequences of blank lines files with a single blank.

An `ed` implementation is provided in the source repository (cf.
[cats.ed](https://github.com/slewsys/ed/blob/main/contrib/cats.ed)).
It demonstrates the use of tokens in place of newlines when working
with multi-line constructs.

#### Integrating Ed, Sed and Perl

Goal: Generate an adjacency list of shell function calls.

An implementation is available as
[shell-call-graph](https://github.com/slewsys/ed/blob/main/contrib/shell-call-graph/shell-call-graph.in).
This script demonstrates `ed` running shell one-liners and editing
the intermediate results. Input scripts are assumed to be structured
as follows:

```
#!/usr/bin/env bash
#
func1 ()
{
    ...
}

func2 ()
{
    ...
}

...

funcN ()
{
    ...
}

if test ."$0" = ."${BASH_SOURCE[0]}"; then
    ...
fi
```

In particular:

- The keyword `function` is omitted.
- Function declarations are on lines by themselves.
- Function-closing curly braces (`}`) are on lines by themselves.
- An `if` statement in column 1 at the end of the file  referencing
  `$0` serves as the script's "main" function.
- Statements within the `if` statement are on lines by themselves.
- The closing `fi` keyword is in column 1 as well.

To see the script in action, from the source repository run:

```
make -C contrib
make -C contrib check
```

## References

The `ed` algorithm is described in Kernighan and Plauger's book
_Software Tools in Pascal_, Addison-Wesley, 1981.

Brian W. Kernighan's `ed` tutorials are included courtesy of _Lucent
Laboratories_.

Please submit issues or pull requests to: <https://github.com/slewsys/ed>
