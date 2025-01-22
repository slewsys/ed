bin_PROGRAMS = prdg

prdg_SOURCES = prdg.c

AM_CPPFLAGS = -D_FORTIFY_SOURCE=3

test_SCRIPT = ${top_builddir}/testsuite/generate-random-graph

EXTRA_DIST =								\
	autogen.sh							\
	config.h.in							\
	configure.ac							\
	m4								\
	Makefile.am							\
	prdg.c								\
	prdg.h								\
	README.md							\
	testsuite


all: $(test_SCRIPT)

check-valgrind-local: clean
	$(MAKE) $(AM_MAKEFLAGS) CFLAGS='-g -O0' -C $(abs_top_builddir) all
	testsuite/generate-random-graph
	$(VALGRIND) ./prdg -w 80 <testsuite/test-data.txt
	$(VALGRIND) ./prdg -w 80 <testsuite/shuffled-test-data.txt

check-local:
	testsuite/generate-random-graph
	$(AWK_CMD) '/:[0-9]/ { print gensub(/.*:([0-9]+).*/, "\\1", 1) }'	\
		<testsuite/test-data.txt |					\
		$(SORT_CMD) -n >testsuite/data-results.txt
	./prdg <testsuite/test-data.txt |						\
		$(AWK_CMD) '/:[0-9]/ { print gensub(/.*:([0-9]+).*/, "\\1", 1) }' |	\
		$(SORT_CMD) -n >testsuite/graph-results.txt
	./prdg <testsuite/shuffled-test-data.txt |					\
		$(AWK_CMD) '/:[0-9]/ { print gensub(/.*:([0-9]+).*/, "\\1", 1) }' |	\
		$(SORT_CMD) -n >testsuite/shuffled-graph-results.txt
	./prdg -w 132  <testsuite/test-data.txt |					\
		$(AWK_CMD) '/:[0-9]/ { print gensub(/.*:([0-9]+).*/, "\\1", 1) }' |	\
		$(SORT_CMD) -n >testsuite/narrow-expanded-results.txt
	cmp testsuite/data-results.txt testsuite/graph-results.txt
	cmp testsuite/data-results.txt testsuite/shuffled-graph-results.txt
	cmp testsuite/data-results.txt testsuite/narrow-expanded-results.txt

clean-local:
	rm -rf .deps prdg prdg.o
	rm -f testsuite/generate-random-graph testsuite/*.txt

distclean-local: clean-local
	rm -f Makefile config.h config.status testsuite/generate-random-graph
