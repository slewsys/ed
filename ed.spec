%define _fortify_level 3

Name:           ed
Version:        2.1.0
Release:        1%{?dist}
Summary:        The standard Unix text editor
License:        BSD-2-Clause OR GPL-2.0-or-later OR MIT
URL:            https://github.com/slewsys/%{name}
Source:         https://github.com/slewsys/%{name}/releases/download/v%{version}/%{name}-%{version}.tar.zst
BuildRequires:  automake
BuildRequires:  autoconf
BuildRequires:  gcc
BuildRequires:  gettext-devel
BuildRequires:  glibc-gconv-extra
BuildRequires:  groff
BuildRequires:  libtool
BuildRequires:  make
BuildRequires:  openssl-devel
BuildRequires:  texinfo
BuildRequires:  texinfo-tex

%description
Ed is a 100% POSIX compliant and 8-bit clean implementation of the
Unix line editor with modern extensions.

%prep
%autosetup
%conf
%configure \
  --enable-all-extensions
%build
%make_build

%check
echo ====================TESTING=========================
make check
echo ====================TESTING END=====================

%install
%make_install

# Adjust contrib sources to RPM specs.
find %{_builddir} -name 'call-graph.in' -print | xargs sed -i -e '1{s|#!.*|#!/bin/bash|;q}'
find %{_builddir} -name 'generate-random-graph.in' -print | xargs sed -i -e '1{s|#!.*|#!/bin/bash|;q}'
find %{_builddir} -name 'cats.ed' -print | xargs sed -i -e '1{s|#!.*|#!/bin/ed -f|;q}'
find %{_builddir} -name 'import-gnulib.sh' -delete
find %{_builddir} -name '.gitignore' -delete
rm -f ${RPM_BUILD_ROOT}/%{_infodir}/dir


%find_lang %{name}-gnulib

%files -f %{name}-gnulib.lang
%{_bindir}/ed
%{_mandir}/man1/ed.1*
%{_infodir}/ed.info*
%{_infodir}/ed-intro.info*
%{_infodir}/ed-adv.info*

%doc BUGS AUTHORS ChangeLog NEWS README.md THANKS TODO
%doc contrib

%license COPYING

%changelog
* Sat Feb 01 2025 Andrew L. Moore <slewsys@gmail.com> - 2.1.1-1
- Update version to 2.1.1.

* Sat Feb 01 2025 Jonathan Wakely <jwakely@redhat.com> - 2.1.0-33
- Update README.md

* Sat Feb 01 2025 Andrew L. Moore <slewsys@gmail.com> - 2.1.0-32
- Update README.

* Sat Feb 01 2025 Andrew L. Moore <slewsys@gmail.com> - 2.1.0-31
- Update clean/distclean targets per GNU Coding Standards.

* Sat Feb 01 2025 Andrew L. Moore <slewsys@gmail.com> - 2.1.0-30
- Add contrib Makefile.

* Sat Feb 01 2025 Andrew L. Moore <slewsys@gmail.com> - 2.1.0-29
- Update ed.spec.

* Sat Feb 01 2025 Andrew L. Moore <slewsys@gmail.com> - 2.1.0-28
- Add Makefile targets for RPM and Deb packages.

* Fri Jan 31 2025 Andrew L. Moore <slewsys@gmail.com> - 2.1.0-27
- Use system getopt_long if available.

* Fri Jan 31 2025 Andrew L. Moore <slewsys@gmail.com> - 2.1.0-26
- Switch to TOML syntax per REUSE Specification v3.3.

* Fri Jan 31 2025 Andrew L. Moore <slewsys@gmail.com> - 2.1.0-25
- Update Gnulib and lib/patch-Makefile.am.diff.

* Sun Jan 26 2025 Andrew L. Moore <slewsys@gmail.com> - 2.1.0-24
- Don't save deleted lines to (default) register.

* Sun Jan 26 2025 Andrew L. Moore <slewsys@gmail.com> - 2.1.0-23
- Enable compression of test suite data.

* Sun Jan 26 2025 Andrew L. Moore <slewsys@gmail.com> - 2.1.0-22
- Adjust testsuite for OmniOS.

* Sun Jan 26 2025 Andrew L. Moore <slewsys@gmail.com> - 2.1.0-21
- Rename environment variable ZSTD_CMD => ZSTD.

* Sun Jan 26 2025 Andrew L. Moore <slewsys@gmail.com> - 2.1.0-20
- Update shell-call-graph parsing regex.

* Sun Jan 26 2025 Andrew L. Moore <slewsys@gmail.com> - 2.1.0-19
- Fix some comments.

* Sun Jan 26 2025 Andrew L. Moore <slewsys@gmail.com> - 2.1.0-18
- Revert experimental global-command structure.

* Sun Jan 26 2025 Andrew L. Moore <slewsys@gmail.com> - 2.1.0-17
- CI: Update macOS actions.

* Sun Jan 26 2025 Andrew L. Moore <slewsys@gmail.com> - 2.1.0-16
- Fix Sun/BSD file-pointer assignment.

* Sun Jan 26 2025 Andrew L. Moore <slewsys@gmail.com> - 2.1.0-15
- Add NOP to avoid c2x-extension warning.

* Sun Jan 26 2025 Andrew L. Moore <slewsys@gmail.com> - 2.1.0-14
- Initial support for OpenBSD.

* Sun Jan 26 2025 Andrew L. Moore <slewsys@gmail.com> - 2.1.0-13
- Handle SIGPIPE and add test.

* Sun Jan 26 2025 Andrew L. Moore <slewsys@gmail.com> - 2.1.0-12
- Use 'command -v' instead of 'which'.

* Mon Dec 23 2024 Andrew L. Moore <slewsys@gmail.com> - 2.1.0-11
- Update test requirements to include zstd.

* Sat Dec 21 2024 Andrew L. Moore <slewsys@gmail.com> - 2.1.0-10
- Allow long searches to be interrupted.

* Tue Jul 02 2024 Andrew L. Moore <slewsys@gmail.com> - 2.1.0-9
- Fix compile error.

* Tue Jul 02 2024 Andrew L. Moore <slewsys@gmail.com> - 2.1.0-8
- call-graph: Loosen function syntax requirement.

* Thu Jun 27 2024 Andrew L. Moore <slewsys@gmail.com> - 2.1.0-7
- Add Crowdin translations to PO file fr.po.

* Wed Jun 26 2024 Andrew L. Moore <slewsys@gmail.com> - 2.1.0-6
- Fix a typo.

* Tue Jun 25 2024 Andrew L. Moore <slewsys@gmail.com> - 2.1.0-5
- Fix corrupted Spanish translation.

* Wed May 29 2024 Andrew L. Moore <slewsys@gmail.com> - 2.1.0-4
- If vfork not available, shell command reports error.

* Mon May 27 2024 Andrew L. Moore <slewsys@gmail.com> - 2.1.0-3
- Don't ignore SIGCHLD.

* Sun May 26 2024 Andrew L. Moore <slewsys@gmail.com> - 2.1.0-2
- Fix uninitialized variables and remove unused ones.

* Sun May 26 2024 Andrew L. Moore <slewsys@gmail.com> - 2.1.0-1
- Add Fedora RPM spec.
