bin_SCRIPTS = shell-call-graph

EXTRA_DIST =								\
	autogen.sh							\
	configure.ac							\
	Makefile.am							\
	shell-call-graph.in						\
	testsuite

check-local:
	$(abs_builddir)/shell-call-graph $(top_srcdir)/testsuite/fit-image
	$(abs_builddir)/shell-call-graph $(top_srcdir)/testsuite/flash-diskimage

clean-local:
	rm -f shell-call-graph

distclean-local: clean-local
	rm -f config.h config.status Makefile
