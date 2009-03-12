# $Id: tpl.spec 3101 2005-04-04 20:13:17Z dag $
# Authority: dag
# Upstream: Markus F.X.J. Oberhumer <markus$oberhumer,com>

%define real_name tpl

Summary: Serialization in C
Name: libtpl
Version: 1.3
Release: 1.rf
License: GPL
Group: System Environment/Libraries
URL: http://tpl.sourceforge.net/

Packager: OpServices <info@opservices.com.br>
Vendor: OpServices, http://www.opservices.com.br

Source: http://www.oberhumer.com/opensource/tpl/download/tpl-%{version}.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root

BuildRequires: zlib-devel, autoconf, gcc-c++
Requires: zlib >= 1.0.0

%description
Tpl is a library for serializing C data. The data is stored in its 
natural binary form. The API is small and tries to stay "out of the way".
Compared to using XML, tpl is faster and easier to use in C programs.
Tpl can serialize many C data types, including structures.
	
%package devel
Summary: Header files, libraries and development documentation for %{name}
Group: Development/Libraries
Requires: %{name} = %{version}-%{release}

%description devel
This package contains the header files, static libraries and development
documentation for %{name}. If you like to develop programs using %{name},
you will need to install %{name}-devel.

%prep
%setup -n %{real_name}-%{version}

%build
%configure \
	--prefix=/usr \
	--mandir=/usr/share/man \
	--infodir=/usr/share/info \
	--localstatedir=/var/run \
	--sysconfdir=/etc \
	--enable-shared \
	--enable-static
%{__make} %{?_smp_mflags}

%install
%{__rm} -rf %{buildroot}
%{__make} install DESTDIR="%{buildroot}"

%post -p /sbin/ldconfig
%postun -p /sbin/ldconfig

%clean
%{__rm} -rf %{buildroot}

%files
%defattr(-, root, root, 0755)
%{_libdir}/libtpl.so.*
%doc doc/

%files devel
%defattr(-, root, root, 0755)
%doc doc/
%{_includedir}
%{_libdir}/libtpl.a
%exclude %{_libdir}/libtpl.la
%{_libdir}/libtpl.so

%changelog