%define realname	zabbix
%define extraver	30

Name: zabbix-sms
Version: 0.1
Release: 1
Group: System Environment/Daemons
License: GPL
Source: %{realname}-1.4.4_yandex%{extraver}.tar.gz

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


%files
%defattr(-,root,root)
