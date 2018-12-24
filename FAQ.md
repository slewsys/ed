# Questions and Answers

## Q. Why is `ed` failing tests?

A. On OpenIndiana, after running `./configure`, try applying patch
_contrib/oi-hipster_config.h.diff_ to correct some settings that
`configure` gets wrong.

In general, `ed` supports several `regex` libraries. To avoid a
collision between these, before switching to an alternative `regex`
library, run:

```bash
rm -f lib/regex.h
configure
```

This relinks _lib/regex.h_ as appropriate per confiigure options.

## Q. Why do `make` and/or `./configure` fail?

A. Running `./configure` or `make` on a cloned repository often
   fails with an error such as:

```
WARNING: 'automake-1.xx' is missing on your system
```

If touching _aclocal.m4_, _configure_ and _Makefile.in_ does not
resolve the problem, then run `autotools` before `configure`:

```bash
./autogen.sh
./configure
make && sudo make install
```

If `autotools` is not available, then try downloading a distribution
archive, whose build system has fewer external dependencies.
