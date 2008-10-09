%define realname	zabbix
%define extraver	31

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
exit 0

%post
/sbin/chkconfig --add zabbix-sms

# make queues
for n in fail success tmp in q{1..9}; do
    [ ! -d %{_localstatedir}/spool/zabbix-sms/$n ] && mkdir %{_localstatedir}/spool/zabbix-sms/$n
done

exit 0

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

install -d %{buildroot}%{_libdir}/zabbix-sms
install -m 755 misc/modem/alamin/bin/imp/gsgimp-file %{buildroot}%{_libdir}/zabbix-sms

%files
%defattr(-,root,root)
%dir %attr(0755,root,root) %{sms_confdir}
%attr(0644,root,root) %config(noreplace) %{sms_confdir}/*.conf

%attr(0755,root,root)  %{_bindir}/gsgc
%attr(0755,root,root)  %{_sbindir}/*
%{_sysconfdir}/init.d/zabbix-sms

%dir %attr(0711,root,root) %{_localstatedir}/spool/zabbix-sms
%dir %attr(0755,root,root) %{_localstatedir}/log/zabbix-sms

%dir %attr(0755,root,root) %{_libdir}/zabbix-sms
%attr(0755,root,root) %{_libdir}/zabbix-sms/gsgimp-file