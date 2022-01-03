![Build Status](https://github.com/slewsys/ed/actions/workflows/ed.yml/badge.svg)

Ed is an implementation of the Unix line editor. It is 8-bit clean
with 64-bit addressing and the only open source implementation that
scores 100% on The Open Group's POSIX-compliance tests.

Ed optionally provides several extensions to the SUSv4 standard
as described in [doc/SUSv4](https://github.com/slewsys/ed/blob/main/doc/SUSv4.md).

It builds and tests successfully across many systems and hardware
platforms.

Brian W. Kernighan's `ed` tutorials are included courtesy of _Lucent
Laboratories_. These are available as PDFs, info documents and NROFF
manuscripts.  See _doc/bwk/_ or, from within `ed`, type:

```ed
!info ed RET m tutorial RET
```

The `ed` algorithm is described in Kernighan and Plauger's book
_Software Tools in Pascal_, Addison-Wesley, 1981.

Please submit issues or pull requests to: <https://github.com/slewsys/ed>
