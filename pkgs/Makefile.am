EXTRA_DIST =								\
	Debian								\
	Fedora								\
	Makefile.am							\
	scripts/build-pkg.in						\
	scripts/deploy-pkg.in

.PHONY: clean-local clean-containers distclean-local

%-deb: Debian/Containerfile
	@stem=$(@:-deb=);						\
	arch=$${stem#*-};						\
	dist=$${stem%-*};						\
	if test ."$$arch" = ."$$dist"; then				\
	    dist=Debian;						\
	fi;								\
	echo "Building Deb for $${arch} in $${dist} container." >&2;	\
	if ! imid=$$(scripts/build-pkg "$$dist" "linux/$${arch}"); then	\
	    echo "$@: Build failed" >&2;				\
	    exit 1;							\
	fi;								\
	scripts/deploy-pkg "$$imid" "$$dist" "linux/$${arch}"


%-rpm: Fedora/Containerfile
	@stem=$(@:-rpm=);						\
	arch=$${stem#*-};						\
	dist=$${stem%-*};						\
	if test ."$$arch" = ."$$dist"; then				\
	    dist=Fedora;						\
	fi;								\
	echo "Building RPMs for $${arch} in $${dist} container." >&2;	\
	if ! imid=$$(scripts/build-pkg "$$dist" "linux/$${arch}"); then	\
	    echo "$@: Build failed" >&2;				\
	    exit 1;							\
	fi;								\
	scripts/deploy-pkg "$$imid" "$$dist" "linux/$${arch}"

clean-local:
	find . \( -name 'ed_*' -o -name 'ed-*' -o -name '*.log'		\
		  -o -name '.build-lock' \) -delete
	find . -maxdepth 1 -name '[A-Z]*' -type d |			\
	    while read dir; do						\
		pushd "$$dir" || exit $?;				\
		find . -maxdepth 1 -not -name Containerfile		\
		    -exec rm -rf {} \+;					\
		popd;							\
	    done

clean-containers:
	@CONTAINER_CLI@ container rm --force				\
	    $$(@CONTAINER_CLI@ container list --external --noheading |	\
	      awk '$$NF ~ /working-container/ { print $$1 }') || true;	\
	@CONTAINER_CLI@ image rm					\
	    $$(@CONTAINER_CLI@ image list --noheading |			\
	      awk '$$1 == "<none>" { print $$3 }') || true

distclean-local: clean-local
	rm -f Makefile scripts/build-pkg scripts/deploy-pkg
