EXTRA_DIST =								\
	cats.ed								\
	ed-mode.el							\
	import-gnulib.sh						\
	Makefile							\
	Makefile.am							\
	Makefile.in							\
	prdg								\
	shell-call-graph

.PHONY: all-local prdg shell-call-graph

all-local: prdg shell-call-graph

prdg:
	@cd prdg;							\
	./autogen.sh;							\
	./configure;							\
	$(MAKE)

shell-call-graph:
	@cd shell-call-graph;						\
	./autogen.sh;							\
	./configure;							\
	$(MAKE)

check-local:
	if test ! -f prdg/Makefile; then				\
	    $(MAKE) prdg;						\
	fi
	$(MAKE) -C prdg check
	if test ! -f shell-call-graph/Makefile; then			\
	    $(MAKE) shell-call-graph;					\
	fi
	$(MAKE) -C shell-call-graph check

install-exec-local:
	if test ! -f prdg/Makefile; then				\
	    $(MAKE) prdg;						\
	fi
	$(MAKE) -C prdg install
	if test ! -f shell-call-graph/Makefile; then			\
	    $(MAKE) shell-call-graph;					\
	fi
	$(MAKE) -C shell-call-graph install

clean-local:
	if test -f shell-call-graph/Makefile; then			\
	    $(MAKE) -C shell-call-graph clean;				\
	fi
	if test -f prdg/Makefile; then					\
	    $(MAKE) -C prdg clean;					\
	fi

distclean-local: clean-local
	if test -f shell-call-graph/Makefile; then			\
	    $(MAKE) -C shell-call-graph distclean;			\
	fi
	if test -f prdg/Makefile; then					\
	    $(MAKE) -C prdg distclean;					\
	fi
	rm -f Makefile
