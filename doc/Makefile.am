# Makefile.am: Doc subdirectory automake template for the ed line editor.
#
# Process this file with automake to create Makefile.in
if !HAVE_MAKEINFO
.texi.info:
	@echo "*** Missing makeinfo ***: Cannot build $@ from $<." >&2
endif

if !HAVE_TEXI2DVI
.texi.pdf:
	@echo "*** Missing texi2dvi ***: Cannot build $@ from $<." >&2
endif

PROG		= ed
ED_TEXI		= $(PROG:=.texi)
INTRO_TEXI	= $(PROG:=-intro.texi)
ADV_TEXI	= $(PROG:=-adv.texi)

ED_PDF		= $(PROG:=.pdf)
INTRO_PDF	= $(PROG:=-intro.pdf)
ADV_PDF		= $(PROG:=-adv.pdf)

ED_INFO		= $(PROG:=.info)
INTRO_INFO	= $(PROG:=-intro.info)
ADV_INFO	= $(PROG:=-adv.info)

ED_TEX_AUX	= $(PROG:=.aux)
ED_TEX_CP	= $(PROG:=.cp)
ED_TEX_FN	= $(PROG:=.fn)
ED_TEX_KY	= $(PROG:=.ky)
ED_TEX_LOG	= $(PROG:=.log)
ED_TEX_PG	= $(PROG:=.pg)
ED_TEX_TOC	= $(PROG:=.toc)
ED_TEX_TP	= $(PROG:=.tp)
ED_TEX_VR	= $(PROG:=.vr)

ED_TEX_EXTRA	= $(ED_TEX_AUX) $(ED_TEX_CP) $(ED_TEX_FN) $(ED_TEX_KY)	     \
		  $(ED_TEX_LOG) $(ED_TEX_PG) $(ED_TEX_TOC) $(ED_TEX_TP)      \
		  $(ED_TEX_VR)


# Munged versions ed-intro. and ed-adv.texi for inclusion in ed.info.
INTRO_INCL_TEXI = $(INTRO_TEXI:.texi=-incl.texi)
INTRO_INFO_TEXI = $(INTRO_TEXI:.texi=-info.texi)
ADV_INCL_TEXI   = $(ADV_TEXI:.texi=-incl.texi)
ADV_INFO_TEXI   = $(ADV_TEXI:.texi=-info.texi)

# Automake doesn't like absolute paths in TEXINOFS decls.
info_TEXINFOS =								      \
	$(ED_TEXI)							      \
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

all-local: $(ED_INFO) $(ED_PDF)

if !HAVE_MAKEINFO
commented:
$(ADV_INFO):
$(INTRO_INFO):
	@echo "*** Missing makeinfo ***: Cannot build $@ from $<." >&2

else
commented:
	for f in  $(ED_TEXI)						      \
		  $(INTRO_TEXI)						      \
		  $(ADV_TEXI)						      \
		  version.texi						      \
		  gpl.texi						      \
		  fdl.texi;						      \
	do test -f "$$f" || cp "$(srcdir)/$$f" .; done
	$(MAKEINFO) $(ED_INFO)

$(INTRO_INFO): $(INTRO_INFO_TEXI)
	$(MAKEINFO) -I . -o $@ $(INTRO_INFO_TEXI)

$(ADV_INFO): $(ADV_INFO_TEXI)
	$(MAKEINFO) -I . -o $@ $(ADV_INFO_TEXI)
endif

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

# Provide explicit rule to work around OpenBSD (?) issue:
#     texi2dvi: Can't use option `--output' with more than one argument.
if !HAVE_TEXI2DVI
$(ED_PDF):
	@echo "*** Missing texi2dvi ***: Cannot build $@ from $<." >&2

else
$(ED_PDF): $(ED_TEXI) $(INTRO_PDF) $(ADV_PDF)
	TEXINPUTS="..:$(TEXINPUTS)" MAKEINFO='/usr/bin/makeinfo   -I .'	     \
	$(TEXI2DVI) --pdf --build-dir=ed.t2p $(ED_TEXI)
endif

clean-local:
	rm -f $(INTRO_INCL_TEXI) $(ADV_INCL_TEXI)			     \
	    $(INTRO_INFO_TEXI) $(ADV_INFO_TEXI)				     \
	    $(ED_TEX_EXTRA) $(ADV_INFO) $(INTRO_INFO)			     \
	    $(ED_INFO) $(ED_PDF) $(INTRO_PDF) $(ADV_PDF)
	$(MAKE) -C bwk clean

distclean-local: clean-local
	rm -f Makefile stamp-vti version.texi
	$(MAKE) -C bwk distclean
