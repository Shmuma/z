Name: setproctitle
Version: 0.3.1
Release: 1

Summary: A setproctitle implementation
License: LGPL/BSD-style
Group: System/Libraries
Packager: Dmitry V. Levin <ldv@altlinux.org>
BuildRoot: %{_tmppath}/%{name}-root
Source: %name-%version.tar.gz

%description
This library provides setproctitle function for setting the invoking
process's title.

%package devel
Summary: Development environment for setproctitle
Group: Development/C
Requires: %name = %version-%release

%description devel
This package contains development files required to build
setproctitle-based software.

%prep
%setup -q

%build
make

%install
make install DESTDIR=%buildroot libdir=%_libdir

%post
ldconfig
%postun
ldconfig

%files
%_libdir/*.so.*
%doc LICENSE

%files devel
%_libdir/*.so
%_includedir/*
%_mandir/*

%changelog
* Wed Mar 21 2007 Dmitry V. Levin <ldv@altlinux.org> 0.3.1-alt1
- setproctitle.3:
  + Noted that setproctitle() clobbers argv[].
  + Updated authorship and license information.
- Packaged LICENSE file, updated License tag.

* Sun Sep 03 2006 Dmitry V. Levin <ldv@altlinux.org> 0.3-alt1
- setproctitle: Clear the _whole_ title buffer every time.
- Linked the library with -Wl,-z,defs.

* Sat Apr 29 2006 Dmitry V. Levin <ldv@altlinux.org> 0.2-alt1
- Build with _GNU_SOURCE defined, replaced deprecated
  __progname and __progname_full symbols with
  program_invocation_short_name and program_invocation_name.
- Enabled almost all diagnostics supported by gcc
  and fixed all issues found by gcc-3.4.5-alt2.
- Changed linking to use gcc instead of ld.
- Linked the library with -Wl,-z,now.
- Updated FSF postal address.

* Tue May 03 2005 Dmitry V. Levin <ldv@altlinux.org> 0.1-alt2
- Pass libdir to "make install" for better mutlilib support (#6764).

* Sun Jun 06 2004 Dmitry V. Levin <ldv@altlinux.org> 0.1-alt1
- Initial revision.
