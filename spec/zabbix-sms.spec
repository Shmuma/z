%define realname	zabbix
%define extraver	30

Name: zabbix-sms
Version: 0.1
Release: 1
Group: System Environment/Daemons
License: GPL
Source: %{realname}-1.4.4_yandex%{extraver}.tar.gz
BuildArch: noarch
BuildRoot: %{_tmppath}/%{name}-root
Requires: perl,coreutils,ppp
Summary: Alamin SMS gateway adopted for zabbix.

%define sms_confdir 		%{_sysconfdir}/zabbix-sms


%description
Alamin SMS gateway prepackaged for zabbix server.

%prep
%setup -q -n %{realname}-1.4.4_yandex%{extraver}

%build

%pre

%post
/sbin/chkconfig --add zabbix-sms

# make queues
for n in fail success tmp q{`seq 1 9`}; do
    mkdir %{_localstatedir}/spool/zabbix-sms/$n
done

%clean
rm -rf %buildroot

%install
install -d %{buildroot}%{sms_confdir}
install -m 644 misc/modem/alamin/config/{gsgc,gsgd,user}.conf %{buildroot}%{sms_confdir}

install -d %{buildroot}%{_bindir}
install -m 755 misc/modem/alamin/bin/gsgc %{buildroot}%{_bindir}
install -d %{buildroot}%{_sbindir}
install -m 755 misc/modem/alamin/bin/{gsgcmd,gsgmdd} %{buildroot}%{_sbindir}

install -d %{buildroot}%{_sysconfdir}/init.d
install -m 755 misc/modem/alamin/start/zabbix-sms %{buildroot}%{_sysconfdir}/init.d

install -d %{buildroot}%{_localstatedir}/spool/zabbix-sms
install -d %{buildroot}%{_localstatedir}/log/zabbix-sms


%files
%defattr(-,root,root)
%dir %attr(0755,root,root) %{sms_confdir}
%attr(0644,root,root) %config(noreplace) %{sms_confdir}/*.conf

%attr(0755,root,root)  %{_bindir}/gsgc
%attr(0755,root,root)  %{_sbindir}/*
%{_sysconfdir}/init.d/zabbix-sms

%dir %attr(0700,root,root) %{_localstatedir}/spool/zabbix-sms
%dir %attr(0755,root,root) %{_localstatedir}/log/zabbix-sms
