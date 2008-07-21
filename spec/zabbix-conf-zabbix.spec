%define realname	zabbix

Name: zabbix-conf-zabbix
Version: 0.1
Release: 1
Group: System/Configuration/Other
License: GPL
Source: %{realname}-1.4.4_yandex9.tar.gz
BuildArch: noarch
BuildRoot: %{_tmppath}/%{name}-root
Requires: zabbix-agent, sudo
Summary: Zabbix extended checks for monitoring servers (pair and top).

%define zabbix_confdir 		%{_sysconfdir}/%{realname}


%description
Zabbix extended checks for monitoring servers (pair and top).

%prep
%setup -q -n %{realname}-1.4.4_yandex9


%build

%pre
# perform required sudo customizations
umask 0337
cat /etc/sudoers | sed  's/^Defaults  *requiretty/#Defaults requiretty/' > /tmp/sudoers.$$
echo 'zabbix ALL=(ALL)        NOPASSWD: /usr/lpp/mmfs/bin/mmpmon' >> /tmp/sudoers.$$
mv /tmp/sudoers.$$ /etc/sudoers
umask 0022

%post
# restart zabbix agent
service zabbix_agentd restart

%clean
rm -rf %buildroot

%install
install -d %{buildroot}%{zabbix_confdir}/conf.d
install -m 755 misc/checks/conf-zabbix/*.conf %{buildroot}%{zabbix_confdir}/conf.d

%files
%defattr(-,root,root)
%dir %attr(0755,root,root) %{zabbix_confdir}
%attr(0644,root,root) %config(noreplace) %{zabbix_confdir}/conf.d/*.conf

