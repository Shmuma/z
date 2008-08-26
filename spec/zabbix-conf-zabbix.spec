%define realname	zabbix

Name: zabbix-conf-zabbix
Version: 0.2
Release: 1
Group: System/Configuration/Other
License: GPL
Source: %{realname}-1.4.4_yandex20.tar.gz
BuildArch: noarch
BuildRoot: %{_tmppath}/%{name}-root
Requires: zabbix-agent, sudo
Summary: Zabbix extended checks for monitoring servers (pair and top).

%define zabbix_confdir 		%{_sysconfdir}/%{realname}


%description
Zabbix extended checks for monitoring servers (pair and top).

%prep
%setup -q -n %{realname}-1.4.4_yandex20


%build

%pre
# perform required sudo customizations
umask 0337
if cat /etc/sudoers | grep monitor | grep -q mmpmon; then
	echo 'Monitor already have sudo for gpfs monitoring'
else
	cat /etc/sudoers | sed  's/^Defaults  *requiretty/#Defaults requiretty/' > /tmp/sudoers.$$
	echo 'monitor ALL=(ALL)        NOPASSWD: /usr/lpp/mmfs/bin/mmpmon' >> /tmp/sudoers.$$
	mv /tmp/sudoers.$$ /etc/sudoers
fi
umask 0022

%post
# restart zabbix agent
service zabbix_agentd restart

%clean
rm -rf %buildroot

%install
install -d %{buildroot}%{zabbix_confdir}/conf.d
install -m 755 misc/checks/conf-zabbix/*.conf %{buildroot}%{zabbix_confdir}/conf.d
install -d %{buildroot}%{zabbix_confdir}/bin
install -m 755 misc/checks/bin/* %{buildroot}%{zabbix_confdir}/bin

%files
%defattr(-,root,root)
%dir %attr(0755,root,root) %{zabbix_confdir}
%attr(0644,root,root) %config(noreplace) %{zabbix_confdir}/conf.d/*.conf
%attr(0755,root,root) %config(noreplace) %{zabbix_confdir}/bin/

