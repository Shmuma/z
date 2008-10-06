%define realname	zabbix
%define extraver	30

Name: zabbix-sms
Version: 0.1
Release: 1
Group: System Environment/Daemons
License: GPL
Source: %{realname}-1.4.4_yandex%{extraver}.tar.gz

%define sms_confdir 		%{_sysconfdir}/zabbix-sms


%description
Alamin SMS gateway prepackaged for zabbix server.

%prep
%setup -q -n %{realname}-1.4.4_yandex%{extraver}

%build

%pre

%post

%clean
rm -rf %buildroot

%install
install -d %{buildroot}%{sms_confdir}
install -m 644 misc/modem/alamin/config/{gsgc,gsgd}.conf %{buildroot}%{sms_confdir}

%files
%defattr(-,root,root)
%dir %attr(0755,root,root) %{sms_confdir}
%attr(0644,root,root) %config(noreplace) %{sms_confdir}/*.conf