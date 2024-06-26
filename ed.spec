%define _fortify_level 3

Name:           ed
Version:        2.1.0
Release:        %autorelease
Summary:        The standard Unix text editor
License:        BSD-2-Clause OR GPL-2.0-or-later OR MIT
URL:            https://github.com/slewsys/%{name}
Source:         https://github.com/slewsys/%{name}/releases/download/v%{version}/%{name}-%{version}.tar.zst
BuildRequires:  m4
BuildRequires:  automake
BuildRequires:  autoconf
BuildRequires:  make
BuildRequires:  gcc
BuildRequires:  gettext
BuildRequires:  gettext-devel
BuildRequires:  openssl-devel
BuildRequires:  libtool
BuildRequires:  ghostscript
BuildRequires:  groff
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
%doc doc/ed.pdf doc/bwk/ed-intro.pdf doc/bwk/ed-adv.pdf
%doc contrib

%license COPYING

%changelog
%autochangelog
