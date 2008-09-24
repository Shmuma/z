%define modname pam
%define dirname %{modname}
%define soname %{modname}.so
%define inifile A55_%{modname}.ini

Summary:	PAM integration for PHP
Name:		php-%{modname}
Version:	1.0.2
Release:	4
Group:		Development/PHP
License:	PHP License
URL:		http://pecl.php.net/package/pam
Source0:	http://pecl.php.net/get/%{modname}-%{version}.tgz
BuildRequires:	php-devel >= 0:5.1.0
BuildRequires:	pam-devel
BuildRoot:	%{_tmppath}/%{name}-%{version}-%{release}-buildroot

%description
This extension provides PAM (Pluggable Authentication Modules) integration. PAM
is a system of libraries that handle the authentication tasks of applications
and services. The library provides a stable API for applications to defer to
for authentication tasks.

%prep

%setup -q -n %{modname}-%{version}
[ "../package*.xml" != "/" ] && mv ../package*.xml .

# lib64 fix
perl -pi -e "s|/lib\b|/%{_lib}|g" config.m4

%build

phpize
./configure --with-libdir=%{_lib} \
    --with-%{modname}=shared,/%{_lib}

make
mv modules/*.so .

#%{_usrsrc}/php-devel/buildext %{modname} "pam.c" \
#    "-L/%{_lib}/security -L/%{_lib} -I%{_includedir}/security -lpam -ldl" \
#    "-DCOMPILE_DL_PAM -DHAVE_PAM -DHAVE_SECURITY_PAM_APPL_H"

%install
[ "%{buildroot}" != "/" ] && rm -rf %{buildroot} 

install -d %{buildroot}%{_sysconfdir}/php.d
install -d %{buildroot}%{_sysconfdir}/pam.d
install -d %{buildroot}%{_libdir}/php/modules

install -m755 %{soname} %{buildroot}%{_libdir}/php/modules/

cat > %{buildroot}%{_sysconfdir}/php.d/%{inifile} << EOF
extension = %{soname}

[pam]
pam.servicename = "%{name}";
EOF

cat > %{buildroot}%{_sysconfdir}/pam.d/%{name} <<EOF
auth	sufficient	pam_pwdb.so shadow nodelay
account	sufficient	pam_pwdb.so
EOF

%post
if [ -f /var/lock/subsys/httpd ]; then
    %{_initrddir}/httpd restart >/dev/null || :
fi

%postun
if [ "$1" = "0" ]; then
    if [ -f /var/lock/subsys/httpd ]; then
	%{_initrddir}/httpd restart >/dev/null || :
    fi
fi

%clean
[ "%{buildroot}" != "/" ] && rm -rf %{buildroot}

%files 
%defattr(-,root,root)
%doc README CREDITS package*.xml 
%config(noreplace) %attr(0644,root,root) %{_sysconfdir}/pam.d/%{name}
%config(noreplace) %attr(0644,root,root) %{_sysconfdir}/php.d/%{inifile}
%attr(0755,root,root) %{_libdir}/php/modules/%{soname}


%changelog
* Fri Jul 18 2008 Oden Eriksson <oeriksson@mandriva.com> 1.0.2-4mdv2009.0
+ Revision: 238417
- rebuild

* Fri May 02 2008 Oden Eriksson <oeriksson@mandriva.com> 1.0.2-3mdv2009.0
+ Revision: 200255
- rebuilt for php-5.2.6

* Mon Feb 04 2008 Oden Eriksson <oeriksson@mandriva.com> 1.0.2-2mdv2008.1
+ Revision: 162107
- rebuild

  + Olivier Blin <oblin@mandriva.com>
    - restore BuildRoot

  + Thierry Vignaud <tvignaud@mandriva.com>
    - kill re-definition of %%buildroot on Pixel's request

* Wed Nov 28 2007 Oden Eriksson <oeriksson@mandriva.com> 1.0.2-1mdv2008.1
+ Revision: 113772
- 1.0.2

* Sun Nov 11 2007 Oden Eriksson <oeriksson@mandriva.com> 1.0.1-2mdv2008.1
+ Revision: 107703
- restart apache if needed

* Thu Sep 27 2007 Oden Eriksson <oeriksson@mandriva.com> 1.0.1-1mdv2008.1
+ Revision: 93259
- 1.0.1

* Sat Sep 01 2007 Oden Eriksson <oeriksson@mandriva.com> 1.0.0-6mdv2008.0
+ Revision: 77565
- rebuilt against php-5.2.4

* Thu Jun 14 2007 Oden Eriksson <oeriksson@mandriva.com> 1.0.0-5mdv2008.0
+ Revision: 39513
- use distro conditional -fstack-protector

* Fri Jun 01 2007 Oden Eriksson <oeriksson@mandriva.com> 1.0.0-4mdv2008.0
+ Revision: 33867
- rebuilt against new upstream version (5.2.3)

* Thu May 03 2007 Oden Eriksson <oeriksson@mandriva.com> 1.0.0-3mdv2008.0
+ Revision: 21347
- rebuilt against new upstream version (5.2.2)


* Thu Feb 08 2007 Oden Eriksson <oeriksson@mandriva.com> 1.0.0-2mdv2007.0
+ Revision: 117604
- rebuilt against new upstream version (5.2.1)

* Mon Nov 13 2006 Oden Eriksson <oeriksson@mandriva.com> 1.0.0-1mdv2007.0
+ Revision: 83617
- Import php-pam

* Mon Nov 13 2006 Oden Eriksson <oeriksson@mandriva.com> 1.0.0-1mdv2007.1
- initial Mandriva package

