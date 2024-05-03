bin_PROGRAMS = prdg

prdg_SOURCES = prdg.c

AM_CPPFLAGS = -D_FORTIFY_SOURCE=3

test_SCRIPT = ${top_builddir}/testsuite/generate-random-graph

all: $(test_SCRIPT)

check-valgrind-local: clean
	$(MAKE) $(AM_MAKEFLAGS) CFLAGS='-g -O0'						\
	    -C $(abs_top_builddir) all
	$(top_builddir)/testsuite/generate-random-graph
	$(VALGRIND) $(top_builddir)/prdg -w 80 <$(top_builddir)/testsuite/test-data.txt
	$(VALGRIND) $(top_builddir)/prdg -w 80						\
		<$(top_builddir)/testsuite/shuffled-test-data.txt

check-local:
	$(top_builddir)/testsuite/generate-random-graph
	$(AWK_CMD) '/:[0-9]/ { print gensub(/.*:([0-9]+).*/, "\\1", 1) }'		\
		<$(top_builddir)/testsuite/test-data.txt |				\
		$(SORT_CMD) -n >$(top_builddir)/testsuite/data-results.txt
	$(top_builddir)/prdg <$(top_builddir)/testsuite/test-data.txt |			\
		$(AWK_CMD) '/:[0-9]/ { print gensub(/.*:([0-9]+).*/, "\\1", 1) }' |	\
		$(SORT_CMD) -n >$(top_builddir)/testsuite/graph-results.txt
	$(top_builddir)/prdg <$(top_builddir)/testsuite/shuffled-test-data.txt |	\
		$(AWK_CMD) '/:[0-9]/ { print gensub(/.*:([0-9]+).*/, "\\1", 1) }' |	\
		$(SORT_CMD) -n >$(top_builddir)/testsuite/shuffled-graph-results.txt
	$(top_builddir)/prdg -w 132  <$(top_builddir)/testsuite/test-data.txt |		\
		$(AWK_CMD) '/:[0-9]/ { print gensub(/.*:([0-9]+).*/, "\\1", 1) }' |	\
		$(SORT_CMD) -n >$(top_builddir)/testsuite/narrow-expanded-results.txt
	cmp $(top_builddir)/testsuite/data-results.txt					\
		$(top_builddir)/testsuite/graph-results.txt
	cmp $(top_builddir)/testsuite/data-results.txt					\
		$(top_builddir)/testsuite/shuffled-graph-results.txt
	cmp $(top_builddir)/testsuite/data-results.txt					\
		$(top_builddir)/testsuite/narrow-expanded-results.txt

clean-local:
	rm -f $(top_builddir)/testsuite/generate-random-graph \
		$(top_builddir)/testsuite/*.txt
