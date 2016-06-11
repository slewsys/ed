# Makefile.am: Doc/tutorial automake template for the ed line editor.
#
# Copyright © 2006-2016 Andrew L. Moore, SlewSys Research
#
# Process this file with automake to create Makefile.in.

SUFFIXES = .ps .pdf

.ps.pdf:
	if test -f $< &&                                                      \
	   test -n "$(PS2PDF)" &&                                             \
	   test -x "$$(which $(PS2PDF))"; then                                \
	    $(PS2PDF) $< $@;                                                  \
	fi


# "A Tutorial Introduction to the Unix Text Editor", by Brian W. Kernighan.
INTRO = ed-intro

# "Advanced Editing on Unix", by Brian W. Kernighan.
ADV = ed-adv

# Top-level tutorial source directory.
# tut_srcdir = $(top_srcdir)/doc/bwk
tut_srcdir = $(srcdir)

# Original source for "A Tutorial Introduction to the Unix Text Editor".
INTRO_SRC = 								      \
    $(tut_srcdir)/intro/e.mac						      \
    $(tut_srcdir)/intro/e0						      \
    $(tut_srcdir)/intro/e1						      \
    $(tut_srcdir)/intro/e2						      \
    $(tut_srcdir)/intro/e3						      \
    $(tut_srcdir)/intro/e4						      \
    $(tut_srcdir)/intro/e5						      \
    $(tut_srcdir)/intro/e6						      \
    $(tut_srcdir)/intro/e7

# Original source for "Advanced Editing on Unix".
ADV_SRC =								      \
    $(tut_srcdir)/adv/ae.mac						      \
    $(tut_srcdir)/adv/ae0						      \
    $(tut_srcdir)/adv/ae1						      \
    $(tut_srcdir)/adv/ae2						      \
    $(tut_srcdir)/adv/ae3						      \
    $(tut_srcdir)/adv/ae4						      \
    $(tut_srcdir)/adv/ae5						      \
    $(tut_srcdir)/adv/ae6						      \
    $(tut_srcdir)/adv/ae7						      \
    $(tut_srcdir)/adv/ae9


all: $(INTRO).pdf $(ADV).pdf

$(INTRO).ps: $(INTRO_SRC)
	if test -n "$(GROFF)" && test -x $$(which $(GROFF)); then             \
	  $(GROFF) -Tps -M$(tut_srcdir)/tmac -ms $(INTRO_SRC) >$@;            \
	elif test -n "$(TROFF)" && test -x $$(which $(TROFF)); then           \
	  if test -n "$(DPOST)" && test -x $$(which $(DPOST)); then           \
	    $(TROFF) -Tpost -ms $(INTRO_SRC) | $(DPOST) >$@;                  \
	  fi;                                                                 \
	fi

$(ADV).ps: $(ADV_SRC)
	if test -n "$(GROFF)" && test -x $$(which $(GROFF)); then             \
	  $(GROFF) -Tps -M$(tut_srcdir)/tmac -ms $(ADV_SRC) >$@;              \
	elif test -n "$(TROFF)" && test -x $$(which $(TROFF)); then           \
	  if test -n "$(DPOST)" && test -x $$(which $(DPOST)); then           \
	    $(TROFF) -Tpost -ms $(ADV_SRC) | $(DPOST) >$@;                    \
	  fi;                                                                 \
	fi

clean-local:
	-rm -f *.ps *.pdf
