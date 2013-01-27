# Makefile.am: Doc subdirectory automake template for the ed line editor.

# Copyright © 1993-2013 Andrew L. Moore, SlewSys Research

# Last modified: 2011-04-14 <alm@slewsys.org>

# Process this file with automake to create Makefile.in

TEXI       = ed.texi
INTRO_TEXI = ed-intro.texi
ADV_TEXI   = ed-adv.texi

INTRO_PS  = $(INTRO_TEXI:.texi=.ps)
INTRO_PDF = $(INTRO_TEXI:.texi=.pdf)
ADV_PS    = $(ADV_TEXI:.texi=.ps)
ADV_PDF   = $(ADV_TEXI:.texi=.pdf)


# Munged versions ed-intro. and ed-adv.texi for inclusion in ed.info.
INTRO_INCL_TEXI = $(INTRO_TEXI:.texi=-incl.texi)
INTRO_INFO_TEXI = $(INTRO_TEXI:.texi=-info.texi)
ADV_INCL_TEXI   = $(ADV_TEXI:.texi=-incl.texi)
ADV_INFO_TEXI   = $(ADV_TEXI:.texi=-info.texi)

INFO       = ed.info
INTRO_INFO = $(INTRO_TEXI:.texi=.info)
ADV_INFO   = $(ADV_TEXI:.texi=.info)

# Automake doesn't like absolute paths in TEXINOFS decls.
info_TEXINFOS = 							      \
	$(TEXI)							      	      \
	$(INTRO_TEXI)						      	      \
	$(ADV_TEXI)

ed_TEXINFOS =								      \
	$(INTRO_INCL_TEXI)					      	      \
	$(ADV_INCL_TEXI)	      				      	      \
	gpl.texi							      \
	fdl.texi

man_MANS = ed.1
EXTRA_DIST = $(man_MANS) SUSv3

SUBDIRS = bwk

# For compatibility with GNU and BSD makes, don't use implied source
# variables (e.g., $<, $^, $?, ${.ALLSRC}, etc.), if not defining
# implicit rules.

.PHONY: all-local

all-local: $(INFO)

commented:
	for f in  $(TEXI)				              	      \
	          $(INTRO_TEXI)    		      		      	      \
	          $(ADV_TEXI)			              		      \
	          version.texi                  		      	      \
	          gpl.texi				              	      \
	          fdl.texi;					      	      \
	do test -f "$$f" || cp "$(srcdir)/$$f" .; done
	$(MAKEINFO) $(INFO)

#$(INFO): $(INTRO_INCL_TEXI) $(ADV_INCL_TEXI)
$(INTRO_INFO): $(INTRO_INFO_TEXI)
$(ADV_INFO): $(ADV_INFO_TEXI)
$(PS): $(TEXI) $(INTRO_PS) $(ADV_PS)
$(PDF): $(TEXI) $(INTRO_PDF) $(ADV_PDF)

# Substitute `Top' node pointers for inclusion in ed.info
$(INTRO_INCL_TEXI): $(INTRO_INFO_TEXI)
	sed -e '1,/^@end ifclear/d'                                           \
	    -e 's/, Top/, A Tutorial Introduction to the UNIX Text Editor/g'  \
	    -e '/^@bye/d'  $(INTRO_INFO_TEXI) >$@.tmp
	mv $@.tmp $@

# Strip hardcopy formatting prior to running makeinfo.
$(INTRO_INFO_TEXI): $(INTRO_TEXI)
	sed -e 's/``@kbd{\([^'\'']*\)}'\'\''/``\1'\'\''/g'                    \
	    -e 's/@code{\([^}]*\)}/\1/g'                                      \
	    -e 's/@math{_\\sqcup}/ /g'                                        \
	    -e 's/@strong{@code{\([^}]*\)}}/\1/g'                             \
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
	sed -e 's/``@kbd{\([^'\'']*\)}'\'\''/``\1'\'\''/g'                    \
	    -e 's/@code{\([^}]*\)}/\1/g'                                      \
	    -e 's/@math{_\\sqcup}/ /g'                                        \
	    -e 's/@strong{@code{\([^}]*\)}}/\1/g'                             \
	    -e 's/@strong{\([^}]*\)}/\1/g'                                    \
	    $(srcdir)/$(ADV_TEXI) >$@.tmp
	mv $@.tmp $@

$(INTRO_PS):
$(INTRO_PDF):
$(ADV_PS):
$(ADV_PDF):
	if cd "$$builddir"/bwk && $(MAKE) $(AM_MAKEFLAGS); then		      \
	    cd "$$builddir" &&						      \
	    ln -sf bwk/$(INTRO_PS)					      \
	           bwk/$(INTRO_PDF)					      \
	           bwk/$(ADV_PS)					      \
	           bwk/$(ADV_PDF)					      \
		   .;							      \
	else if test ."@TEXI2PDF@" != .''; then				      \
	    $(TEXI2PDF) --build=tidy ed-intro.texi;			      \
	    $(TEXI2PDF) --build=tidy ed-adv.texi;			      \
	    $(TEXI2DVI) --build=tidy ed-intro.texi;			      \
	    $(TEXI2DVI) --build=tidy ed-adv.texi;			      \
	fi

clean-local:
	rm -f $(INTRO_INCL_TEXI) $(ADV_INCL_TEXI)             		      \
	      $(INTRO_INFO_TEXI) $(ADV_INFO_TEXI)

# Local variables:
# mode: makefile-gmake
# eval: (add-hook 'write-file-functions 'time-stamp)
# time-stamp-start: "Last modified: "
# time-stamp-format: "%:y-%02m-%02d <%u@%h>"
# time-stamp-end: "$"
# End:
