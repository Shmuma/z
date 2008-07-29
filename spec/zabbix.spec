%define realname	zabbix

Name: zabbix-mysql
Version: 1.4.4
Release: yandex_12
Group: System Environment/Daemons
License: GPL
Source: %{realname}-%{version}_yandex12.tar.gz
BuildRoot: %{_tmppath}/%{name}-root
BuildPrereq: mysql, mysql-devel, net-snmp-devel, setproctitle-devel, iksemel-devel, pkgconfig
Requires: mysql, net-snmp, setproctitle, iksemel
Summary: A network monitor.

%define zabbix_bindir 	        %{_sbindir}
%define zabbix_confdir 		%{_sysconfdir}/%{realname}
%define zabbix_run 		%{_localstatedir}/run/zabbix
%define zabbix_log 		%{_localstatedir}/log/zabbix
%define zabbix_spool 		%{_localstatedir}/spool/zabbix

%description
zabbix is a network monitor.

%package -n zabbix-agent
Summary: Zabbix agent
Group: System Environment/Daemons

%description -n zabbix-agent
the zabbix network monitor agent.

%prep
%setup -q -n %{realname}-%{version}_yandex12

%build
%configure --enable-server --enable-agent --with-mysql --with-jabber --with-net-snmp
make

# adjust in several files /home/zabbix
for zabbixfile in misc/conf/* misc/init.d/redhat/{zabbix_agentd,zabbix_server}; do
    sed -i -e "s#BASEDIR=.*#BASEDIR=%{zabbix_bindir}#g" \
        -e "s#PidFile=/var/tmp#PidFile=%{zabbix_run}#g" \
        -e "s#LogFile=/tmp#LogFile=%{zabbix_log}#g" \
        -e "s#ActiveChecksBufFile=/var/tmp#ActiveChecksBufFile=%{zabbix_spool}#g" \
        -e "s#AlertScriptsPath=/home/zabbix#AlertScriptsPath=%{zabbix_confdir}#g" \
        -e "s#/home/zabbix/lock#%{_localstatedir}/lock#g" $zabbixfile
done


%pre
if [ -z "`grep zabbix etc/group`" ]; then
    /usr/sbin/groupadd zabbix >/dev/null 2>&1
fi
if [ -z "`grep zabbix etc/passwd`" ]; then
    /usr/sbin/useradd -g zabbix zabbix >/dev/null 2>&1
fi

%pre -n zabbix-agent
if [ -z "`grep zabbix etc/group`" ]; then
    /usr/sbin/groupadd zabbix >/dev/null 2>&1
fi
if [ -z "`grep zabbix etc/passwd`" ]; then
    /usr/sbin/useradd -g zabbix zabbix >/dev/null 2>&1
fi

%post
/sbin/chkconfig --add zabbix_server
[ -d %zabbix_run ] || ( mkdir %zabbix_run && chown zabbix:zabbix %zabbix_run )
[ -d %zabbix_log ] || ( mkdir %zabbix_log && chown zabbix:zabbix %zabbix_log )

%post -n zabbix-agent
/sbin/chkconfig --add zabbix_agentd
[ -d %zabbix_run ] || ( mkdir %zabbix_run && chown zabbix:zabbix %zabbix_run )
[ -d %zabbix_log ] || ( mkdir %zabbix_log && chown zabbix:zabbix %zabbix_log )
[ -d %zabbix_spool ] || ( mkdir %zabbix_spool && chown zabbix:zabbix %zabbix_spool )

if [ -z "`grep zabbix_agent etc/services`" ]; then
  cat >>etc/services <<EOF
zabbix_agent	10050/tcp
EOF
fi

if [ -z "`grep zabbix_trap etc/services`" ]; then
  cat >>etc/services <<EOF
zabbix_trap	10051/tcp
EOF
fi

# replace Hostname of config file with fqdn
HOSTNAME=$(hostname -f)

for zabbixfile in %{zabbix_confdir}/zabbix_agentd.conf; do
    sed -i -e "s#Hostname=.*#Hostname=$HOSTNAME#g" $zabbixfile
done

# perform rebase
%{zabbix_bindir}/zabbix-rebase-server


%preun
if [ "$1" = 0 ]
then
  /sbin/service zabbix_server stop >/dev/null 2>&1 || :
  /sbin/chkconfig --del zabbix_server
fi

%preun -n zabbix-agent
if [ "$1" = 0 ]
then
  /sbin/service zabbix_agentd stop >/dev/null 2>&1 || :
  /sbin/chkconfig --del zabbix_agentd
  [ -d %zabbix_spool ] && rm -rf %zabbix_spool
fi

%clean
rm -fr %buildroot

%install
rm -fr %buildroot
%makeinstall

# create directory structure
install -d %{buildroot}%{zabbix_confdir}
install -d %{buildroot}%{zabbix_confdir}/conf.d
install -d %{buildroot}%{_sysconfdir}/init.d

# copy conf files
install -m 755 misc/conf/*.conf %{buildroot}%{zabbix_confdir}
install -m 711 misc/zabbix-rebase-server %{buildroot}%{zabbix_bindir}/zabbix-rebase-server

# redhat install scripts
install -m 755 misc/init.d/redhat/zabbix_agentd %{buildroot}%{_sysconfdir}/init.d/
install -m 755 misc/init.d/redhat/zabbix_server %{buildroot}%{_sysconfdir}/init.d/

%files
%defattr(-,root,root)
%doc AUTHORS COPYING NEWS README INSTALL create upgrades
%dir %attr(0755,root,root) %{zabbix_confdir}
%attr(0644,root,root) %config(noreplace) %{zabbix_confdir}/zabbix_server.conf
%attr(0755,root,root) %{zabbix_bindir}/zabbix_server
%attr(0755,root,root) %{zabbix_bindir}/hfsdump
%attr(0755,root,root) %{zabbix_bindir}/hfs_trends_upd
%config(noreplace) %{_sysconfdir}/init.d/zabbix_server

%files -n zabbix-agent
%defattr(-,root,root)
%dir %attr(0755,root,root) %{zabbix_confdir}
%dir %attr(0755,root,root) %{zabbix_confdir}/conf.d
%attr(0644,root,root) %config(noreplace) %{zabbix_confdir}/zabbix_agent.conf
%attr(0644,root,root) %config(noreplace) %{zabbix_confdir}/zabbix_agentd.conf
%attr(0644,root,root) %config(noreplace) %{zabbix_confdir}/zabbix_trapper.conf
%attr(0644,root,root) %config(noreplace) %{zabbix_confdir}/server.conf
%config(noreplace) %{_sysconfdir}/init.d/zabbix_agentd
%attr(0755,root,root) %{zabbix_bindir}/zabbix_agent
%attr(0755,root,root) %{zabbix_bindir}/zabbix_agentd
%attr(0755,root,root) %{zabbix_bindir}/zabbix_sender
%attr(0755,root,root) %{zabbix_bindir}/zabbix_get
%attr(0711,root,root) %{zabbix_bindir}/zabbix-rebase-server

%changelog
* Fri Jan 29 2005 Dirk Datzert <dirk@datzert.de>
- update to 1.1aplha6

* Tue Jun 01 2003 Alexei Vladishev <alexei.vladishev@zabbix.com>
- update to 1.0beta10 

* Tue Jun 01 2003 Harald Holzer <hholzer@may.co.at>
- update to 1.0beta9
- move phpfrontend config to /etc/zabbix

* Tue May 23 2003 Harald Holzer <hholzer@may.co.at>
- split the php frontend in a extra package

* Tue May 20 2003 Harald Holzer <hholzer@may.co.at>
- 1.0beta8
- initial packaging
