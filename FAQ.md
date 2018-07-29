# Questions and Answers

## Q. Why is `ed(1)` failing tests?

A. `ed(1)` supports several `regex(3)` libraries. To avoid a collision
between these, run:

```bash
make distclean
./autogen.sh
configure
```

before switching to an alternative `regex(3)` library, . This relinks
lib/regex.h as appropriate per confiigure options.

## Q. Why do `make` and/or `./configure` fail?

A. After cloning a repository, running `./configure` or `make` might
   fail with an error such as:

```
WARNING: 'automake-1.xx' is missing on your system
```

because `git(1)` does not preserve timestamps, which causes
`autoconf(1)` to try to rebuild _aclocal.m4_ and friends. To prevent
this, run `touch(1)` first:

```bash
touch configure.ac aclocal.m4 configure Makefile.am Makefile.in
./configure
make && sudo make install
```

If `autotools(1)` is still falling over and trying the above commands,
try reconstructing the build system with `./autogen.sh`.

For details on why preserving timestamps with `git(1)` would be
problematic, see
[GitWiki](https://git.wiki.kernel.org/index.php/Git_FAQ#Why_isn.27t_Git_preserving_modification_time_on_files.3F).