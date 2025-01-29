EXTRA_DIST =									\
	Debian									\
	Fedora									\
	Makefile.am								\
	scripts/build-pkg.in							\
	scripts/deploy-pkg.in


%-deb: Debian/Containerfile
	@stem=$(@:-deb=);							\
	arch=$${stem#*-};							\
	dist=$${stem%-*};							\
	if test ."$$arch" = ."$$dist"; then					\
	    dist=Debian;							\
	fi;									\
	echo "Building Deb for $${arch} in $${dist} container." >&2;		\
	if ! imageid=$$(scripts/build-pkg "$$dist" "linux/$${arch}"); then	\
	    echo "$@: Build failed" >&2;					\
	    exit 1;								\
	fi;									\
	scripts/deploy-pkg "$$imageid" "$$dist" "linux/$${arch}"


%-rpm: Fedora/Containerfile
	@stem=$(@:-rpm=);							\
	arch=$${stem#*-};							\
	dist=$${stem%-*};							\
	if test ."$$arch" = ."$$dist"; then					\
	    dist=Fedora;							\
	fi;									\
	echo "Building RPMs for $${arch} in $${dist} container." >&2;		\
	if ! imageid=$$(scripts/build-pkg "$$dist" "linux/$${arch}"); then	\
	    echo "$@: Build failed" >&2;					\
	    exit 1;								\
	fi;									\
	scripts/deploy-pkg "$$imageid" "$$dist" "linux/$${arch}"

clean-local:
	find . \( -name 'ed_*' -o -name 'ed-*' -o -name '*.log' \
		  -o -name '.build-lock' \) -delete

distclean-local: clean-local
	rm -f Makefile scripts/build-pkg scripts/deploy-pkg
