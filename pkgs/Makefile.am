EXTRA_DIST =									\
	Debian									\
	Fedora									\
	Makefile.am								\
	scripts/build-pkg.in							\
	scripts/deploy-pkg.in


%-deb: Debian/Containerfile
	@stem=$(@:-deb=);							\
	arch=$${stem#*-};							\
	dist=Debian;								\
	echo "Building Deb for $${arch} under $${dist}" >&2;			\
	if ! imageid=$$(scripts/build-pkg "$$dist" "linux/$${arch}"); then	\
	    echo "$@: Build failed" >&2;					\
	    exit 1;								\
	fi;									\
	scripts/deploy-pkg "$$imageid" "$$dist" "linux/$${arch}"


%-amd64-deb: Debian/Containerfile
	@stem=$(@:-deb=);							\
	arch=$${stem#*-};							\
	dist=$(@:-amd64-deb=);							\
	if test ."$dist" = ."$stem"; then					\
	    dist=Debian;							\
	fi;									\
	echo "Building Deb for $${arch} under $${dist}" >&2;			\
	if ! imageid=$$(scripts/build-pkg "$$dist" "linux/$${arch}"); then	\
	    echo "$@: Build failed" >&2;					\
	    exit 1;								\
	fi;									\
	scripts/deploy-pkg "$$imageid" "$$dist" "linux/$${arch}"

%-arm-deb: Debian/Containerfile
	@stem=$(@:-deb=);							\
	arch=$${stem#*-};							\
	dist=$(@:-arm-deb=);							\
	if test ."$dist" = ."$stem"; then					\
	    dist=Debian;							\
	fi;									\
	echo "Building Deb for $${arch} under $${dist}" >&2;			\
	if ! imageid=$$(scripts/build-pkg "$$dist" "linux/$${arch}"); then	\
	    echo "$@: Build failed" >&2;					\
	    exit 1;								\
	fi;									\
	scripts/deploy-pkg "$$imageid" "$$dist" "linux/$${arch}"

%-arm64-deb: Debian/Containerfile
	@stem=$(@:-deb=);							\
	arch=$${stem#*-};							\
	dist=$(@:-arm64-deb=);							\
	if test ."$dist" = ."$stem"; then					\
	    dist=Debian;							\
	fi;									\
	echo "Building Deb for $${arch} under $${dist}" >&2;			\
	if ! imageid=$$(scripts/build-pkg "$$dist" "linux/$${arch}"); then	\
	    echo "$@: Build failed" >&2;					\
	    exit 1;								\
	fi;									\
	scripts/deploy-pkg "$$imageid" "$$dist" "linux/$${arch}"

%-rpm: Fedora/Containerfile
	@stem=$(@:-rpm=);							\
	arch=$${stem#*-};							\
	dist=Fedora;								\
	echo "Building RPMs for $${arch} under $${dist}" >&2;			\
	if ! imageid=$$(scripts/build-pkg "$$dist" "linux/$${arch}"); then	\
	    echo "$@: Build failed" >&2;					\
	    exit 1;								\
	fi;									\
	scripts/deploy-pkg "$$imageid" "$$dist" "linux/$${arch}"

%-amd64-rpm: Fedora/Containerfile
	@stem=$(@:-rpm=);							\
	arch=$${stem#*-};							\
	dist=$(@:-amd64-deb=);							\
	if test ."$dist" = ."$stem"; then					\
	    dist=Fedora;							\
	fi;									\
	echo "Building RPMs for $${arch} under $${dist}" >&2;			\
	if ! imageid=$$(scripts/build-pkg "$$dist" "linux/$${arch}"); then	\
	    echo "$@: Build failed" >&2;					\
	    exit 1;								\
	fi;									\
	scripts/deploy-pkg "$$imageid" "$$dist" "linux/$${arch}"

%-arm64-rpm: Fedora/Containerfile						\
	@stem=$(@:-rpm=);							\
	arch=$${stem#*-};							\
	dist=$(@:-arm64-deb=);							\
	if test ."$dist" = ."$stem"; then					\
	    dist=Fedora;							\
	fi;									\
	echo "Building RPMs for $${arch} under $${dist}" >&2;			\
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
