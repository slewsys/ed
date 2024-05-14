# Makefile.am: Doc subdirectory automake template for the ed line editor.
#
# Process this file with automake to create Makefile.in

PROG		= ed
TEXI		= $(PROG:=.texi)
INTRO_TEXI	= $(PROG:=-intro.texi)
ADV_TEXI	= $(PROG:=-adv.texi)

PDF		= $(PROG:=.pdf)
INTRO_PDF	= $(PROG:=-intro.pdf)
ADV_PDF		= $(PROG:=-adv.pdf)

INFO		= $(PROG:=.info)
INTRO_INFO	= $(PROG:=-intro.info)
ADV_INFO	= $(PROG:=-adv.info)

TEX_AUX		= $(PROG:=.aux)
TEX_CP		= $(PROG:=.cp)
TEX_FN		= $(PROG:=.fn)
TEX_KY		= $(PROG:=.ky)
TEX_LOG		= $(PROG:=.log)
TEX_PG		= $(PROG:=.pg)
TEX_TOC		= $(PROG:=.toc)
TEX_TP		= $(PROG:=.tp)
TEX_VR		= $(PROG:=.vr)

TEX_EXTRA	= $(TEX_AUX) $(TEX_CP) $(TEX_FN) $(TEX_KY)		     \
		  $(TEX_LOG) $(TEX_PG) $(TEX_TOC) $(TEX_TP) $(TEX_VR)


# Munged versions ed-intro. and ed-adv.texi for inclusion in ed.info.
INTRO_INCL_TEXI = $(INTRO_TEXI:.texi=-incl.texi)
INTRO_INFO_TEXI = $(INTRO_TEXI:.texi=-info.texi)
ADV_INCL_TEXI   = $(ADV_TEXI:.texi=-incl.texi)
ADV_INFO_TEXI   = $(ADV_TEXI:.texi=-info.texi)

# Automake doesn't like absolute paths in TEXINOFS decls.
info_TEXINFOS =								      \
	$(TEXI)								      \
	$(INTRO_TEXI)							      \
	$(ADV_TEXI)

ed_TEXINFOS =								      \
	$(INTRO_INCL_TEXI)						      \
	$(ADV_INCL_TEXI)						      \
	gpl.texi							      \
	fdl.texi

man_MANS = $(PROG:=.1)

SUBDIRS = bwk

# For compatibility with GNU and BSD makes, don't use implied source
# variables (e.g., $<, $^, $?, ${.ALLSRC}, etc.), if not defining
# implicit rules.

.PHONY: all-local

all-local: $(INFO) $(PDF)

commented:
	for f in  $(TEXI)						      \
		  $(INTRO_TEXI)						      \
		  $(ADV_TEXI)						      \
		  version.texi						      \
		  gpl.texi						      \
		  fdl.texi;						      \
	do test -f "$$f" || cp "$(srcdir)/$$f" .; done
	$(MAKEINFO) $(INFO)

# $(INFO): $(INTRO_INCL_TEXI) $(ADV_INCL_TEXI)
#	$(MAKEINFO) -I . -o $@ $(TEXI) $(INTRO_INCL_TEXI) $(ADV_INCL_TEXI)

$(INTRO_INFO): $(INTRO_INFO_TEXI)
	$(MAKEINFO) -I . -o $@ $(INTRO_INFO_TEXI)

$(ADV_INFO): $(ADV_INFO_TEXI)
	$(MAKEINFO) -I . -o $@ $(ADV_INFO_TEXI)

$(PDF): $(TEXI) $(INTRO_PDF) $(ADV_PDF)

# Substitute `Top' node pointers for inclusion in ed.info
$(INTRO_INCL_TEXI): $(INTRO_INFO_TEXI)
	sed -e '1,/^@end ifclear/d'                                           \
	    -e 's/, Top/, A Tutorial Introduction to the UNIX Text Editor/g'  \
	    -e '/^@bye/d'  $(INTRO_INFO_TEXI) >$@.tmp
	mv $@.tmp $@

# Strip hardcopy formatting prior to running makeinfo.
$(INTRO_INFO_TEXI): $(INTRO_TEXI)
	sed -e 's/`@kbd{\([^'\'']*\)}'\''/`\1'\''/g'                          \
	    -e 's/@code{\([^}]*\)}/\1/g'                                      \
	    -e 's/@math{_\\sqcup}/ /g'                                        \
	    -e 's/@strong{@code{\([^}]*\)}}/\1/g'                             \
	    -e 's/@strong{@kbd{\([^}]*\)}}/\1/g'                              \
	    -e 's/@strong{\([^}]*\)}/\1/g'                                    \
	    $(srcdir)/$(INTRO_TEXI) >$@.tmp
	mv $@.tmp $@

# Substitute `Top' node pointers for inclusion in ed.info
$(ADV_INCL_TEXI): $(ADV_INFO_TEXI)
	sed -e '1,/^@end ifclear/d'                                           \
	    -e 's/, Top/, Advanced Editing on UNIX/g'                         \
	    -e '/^@bye/d'  $(ADV_INFO_TEXI) >$@.tmp
	mv $@.tmp $@

# Strip hardcopy formatting prior to running makeinfo.
$(ADV_INFO_TEXI): $(ADV_TEXI)
	sed -e 's/`@kbd{\([^'\'']*\)}'\''/`\1'\''/g'                          \
	    -e 's/@code{\([^}]*\)}/\1/g'                                      \
	    -e 's/@math{_\\sqcup}/ /g'                                        \
	    -e 's/@strong{@code{\([^}]*\)}}/\1/g'                             \
	    -e 's/@strong{@kbd{\([^}]*\)}}/\1/g'                              \
	    -e 's/@strong{\([^}]*\)}/\1/g'                                    \
	    $(srcdir)/$(ADV_TEXI) >$@.tmp
	mv $@.tmp $@

$(INTRO_PDF):
	ln -sf bwk/$(INTRO_PDF)	$(abs_top_builddir)/doc/$(INTRO_PDF)

$(ADV_PDF):
	ln -sf bwk/$(ADV_PDF)	$(abs_top_builddir)/doc/$(ADV_PDF)

clean-local:
	rm -f $(INTRO_INCL_TEXI) $(ADV_INCL_TEXI)			      \
	    $(INTRO_INFO_TEXI) $(ADV_INFO_TEXI)				      \
	    $(TEX_EXTRA)

distclean-local:
	rm -f ed-adv.info ed-intro.info ed.info stamp-vti version.texi
