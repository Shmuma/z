Name: libsqlora8
Version: 2.3.3
Release: 1
Group: System Environment/Libraries
License: GPL
Source: %{name}-%{version}.tar.gz
BuildRoot: %{_tmppath}/%{name}-root
Summary: C-library to access Oracle databases via the OCI interface.
Requires: oracle-instantclient-basic
AutoReq: no

%description
C-library to access Oracle databases via the OCI interface.

%package devel
Summary: Development environment for libsqlora8
Group: Development/C
Requires: %name = %version-%release

%description devel 
This package contains developmetn files for usage of libsqlora8 in
other programs.

%prep
%setup -q

%build
%ifarch x86_64
%configure --enable-64bit 
%else
echo Unsupported architecture. Fail.
exit 1
%endif

make

%install
%makeinstall pkgconfigdir=%buildroot/%_libdir/pkgconfig/

%files
%_libdir/*.so.*


%files devel
%_bindir/*
%_libdir/*.so
%_libdir/*.a
%_libdir/*.la
%_libdir/pkgconfig/*
%_libdir/libsqlora8/*
%_defaultdocdir/*
%_includedir/*
%_datadir/aclocal/*