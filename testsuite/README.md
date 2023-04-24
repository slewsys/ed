# Ed Testsuite

The files in this directory with suffixes _.t_, _.d_, _.r_, _.rr_ and
_.err_ are used for testing **ed**. To run the tests, set the *ED*
variable in the Makefile to the path name of the program to be tested
(e.g., _/bin/ed_), and run `make`. The tests do not exhaustively
verify POSIX compliance nor do they verify correct 8-bit or long line
support.

The test file suffixes have the following meanings:
| Suffix | Description                                                             |
|:-------|:------------------------------------------------------------------------|
| .t     | Template - a list of ed commands from which an ed script is constructed |
| .d     | Data - read by an ed script                                             |
| .r     | Result - the expected output after processing data via an ed script.    |
| .rr    | Result from a piped ed script.                                          |
| .err   | Error - invalid ed commands that should generate an error               |

**Autotest** is used for testing. At the top-level build directory,
run `make check`. The results of the tests may be viewed in
_testsuite/*.dir_.

The POSIX requirement that an address range not be used where at most
a single address is expected has been relaxed in this version of **ed**.
Therefore, the  following script templates are disabled:

- _=-err.t.posix_
- _a1-err.posix_
- _i1-err.posix_
- _k1-err.posix_
- _r1-err.posix_

To use these, remove the _.posix_ suffix and run the tests as
described above.
