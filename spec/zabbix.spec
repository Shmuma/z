%define realname	zabbix
%define extraver	47

Name: zabbix-mysql
Version: 1.4.4
Release: yandex_%{extraver}
Group: System Environment/Daemons
License: GPL
Source: %{realname}-%{version}_yandex%{extraver}.tar.gz
BuildRoot: %{_tmppath}/%{name}-root
BuildPrereq: mysql, mysql-devel, net-snmp-devel, setproctitle-devel, iksemel-devel, pkgconfig, libmemcached
Requires: mysql, net-snmp, setproctitle, iksemel, libmemcached, memcached
Summary: A network monitor.

%define version_full		%{version}_yandex%{extraver}
%define zabbix_bindir 	        %{_sbindir}
%define zabbix_confdir 		%{_sysconfdir}/%{realname}
%define zabbix_agent_run	%{_localstatedir}/run/zabbix-agent/
%define zabbix_agent_log 	%{_localstatedir}/log/zabbix-agent/
%define zabbix_srv_run 		%{_localstatedir}/run/zabbix-server/
%define zabbix_srv_log 		%{_localstatedir}/log/zabbix-server/
%define zabbix_spool 		%{_localstatedir}/spool/zabbix

%description
zabbix is a network monitor.

%package -n zabbix-agent
Summary: Zabbix agent
Group: System Environment/Daemons

%description -n zabbix-agent
the zabbix network monitor agent.

%prep
%setup -q -n %{realname}-%{version}_yandex%{extraver}

%build
%configure --enable-version=%{version_full} --enable-server --enable-agent --enable-memcache --with-mysql --with-jabber --with-net-snmp
make

# adjust in several files /home/zabbix
for zabbixfile in misc/conf/{zabbix_agent.conf,zabbix_agentd.conf} misc/init.d/redhat/zabbix_agentd; do
    sed -i -e "s#BASEDIR=.*#BASEDIR=%{zabbix_bindir}#g" \
        -e "s#PidFile=/var/tmp#PidFile=%{zabbix_agent_run}#g" \
        -e "s#LogFile=/tmp#LogFile=%{zabbix_agent_log}#g" \
        -e "s#ActiveChecksBufFile=/var/tmp#ActiveChecksBufFile=%{zabbix_spool}#g" \
        -e "s#AlertScriptsPath=/home/zabbix#AlertScriptsPath=%{zabbix_confdir}#g" \
        -e "s#/home/zabbix/lock#%{_localstatedir}/lock#g" $zabbixfile
done

for zabbixfile in misc/conf/zabbix_server.conf misc/init.d/redhat/zabbix_server; do
    sed -i -e "s#BASEDIR=.*#BASEDIR=%{zabbix_bindir}#g" \
        -e "s#PidFile=/var/tmp#PidFile=%{zabbix_srv_run}#g" \
        -e "s#LogFile=/tmp#LogFile=%{zabbix_srv_log}#g" \
        -e "s#/home/zabbix/lock#%{_localstatedir}/lock#g" $zabbixfile
done


%pre
if [ -z "`grep zabbix etc/group`" ]; then
    /usr/sbin/groupadd zabbix >/dev/null 2>&1
fi
if [ -z "`grep zabbix etc/passwd`" ]; then
    /usr/sbin/useradd -g zabbix zabbix >/dev/null 2>&1
fi

exit 0

%pre -n zabbix-agent

if [ -z "`grep monitor etc/group`" ]; then
    /usr/sbin/groupadd monitor >/dev/null 2>&1
fi
if [ -z "`grep monitor etc/passwd`" ]; then
    /usr/sbin/useradd -g monitor monitor >/dev/null 2>&1
fi

[ -x /etc/init.d/zabbix_agentd ] && /etc/init.d/zabbix_agentd stop

# change ownership of cache
[ -d %{zabbix_spool} ] && chown -R monitor:monitor %{zabbix_spool}

# move old config files
#[ -f %{zabbix_confdir}/zabbix_agent.conf ] && mv -f %{zabbix_confdir}/zabbix_agent.conf %{zabbix_confdir}/zabbix_agent.conf.old
#[ -f %{zabbix_confdir}/zabbix_agentd.conf ] && mv -f %{zabbix_confdir}/zabbix_agentd.conf %{zabbix_confdir}/zabbix_agentd.conf.old
#[ -f %{zabbix_confdir}/zabbix_trapper.conf ] && mv -f %{zabbix_confdir}/zabbix_trapper.conf %{zabbix_confdir}/zabbix_trapper.conf.old
#[ -f %{zabbix_confdir}/server.conf ] && mv -f %{zabbix_confdir}/server.conf %{zabbix_confdir}/server.conf.old

exit 0


%post
/sbin/chkconfig --add zabbix_server
[ -d %zabbix_srv_run ] || ( mkdir %zabbix_srv_run && chown zabbix:zabbix %zabbix_srv_run )
[ -d %zabbix_srv_log ] || ( mkdir %zabbix_srv_log && chown zabbix:zabbix %zabbix_srv_log )

%post -n zabbix-agent
/sbin/chkconfig --add zabbix_agentd
[ -d %zabbix_agent_run ] || ( mkdir %zabbix_agent_run && chown monitor:monitor %zabbix_agent_run )
[ -d %zabbix_agent_log ] || ( mkdir %zabbix_agent_log && chown monitor:monitor %zabbix_agent_log )
[ -d %zabbix_spool ] || ( mkdir %zabbix_spool && chown monitor:monitor %zabbix_spool )
chown -R monitor:monitor %{zabbix_confdir}/conf.d/

if [ -z "`grep zabbix_agent etc/services`" ]; then
  cat >>/etc/services <<EOF
zabbix_agent	10050/tcp
EOF
fi

if [ -z "`grep zabbix_trap etc/services`" ]; then
  cat >>/etc/services <<EOF
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

# clean ipcs to ensure new instance can start correctly (semaphores)
#ipcs -s | grep zabbix | sed 's/  */ /g' | cut -d ' ' -f 2 | while read key; do ipcrm -s $key; done
# shared memory segments
#ipcs -m | grep zabbix | sed 's/  */ /g' | cut -d ' ' -f 2 | while read key; do ipcrm -m $key; done

#sleep 1

# start agent after installation
#/sbin/service zabbix_agentd start >/dev/null 2>&1 || :
/etc/init.d/zabbix_agentd start
exit 0


%preun
if [ "$1" = 0 ]
then
  /sbin/service zabbix_server stop >/dev/null 2>&1 || :
  /sbin/chkconfig --del zabbix_server
fi

exit 0

%preun -n zabbix-agent

# move old config files
#mv -f %{zabbix_confdir}/zabbix_agent.conf %{zabbix_confdir}/zabbix_agent.conf.old
#mv -f %{zabbix_confdir}/zabbix_agentd.conf %{zabbix_confdir}/zabbix_agentd.conf.old
#mv -f %{zabbix_confdir}/zabbix_trapper.conf %{zabbix_confdir}/zabbix_trapper.conf.old
#mv -f %{zabbix_confdir}/server.conf %{zabbix_confdir}/server.conf.old

if [ "$1" = 0 ]
then
  # stop agent
  /etc/init.d/zabbix_agentd stop
  /sbin/chkconfig --del zabbix_agentd
  [ -d %zabbix_spool ] && rm -rf %zabbix_spool
fi

exit 0


%clean
rm -fr %buildroot

%install
rm -fr %buildroot
%makeinstall

# create directory structure
install -d %{buildroot}%{zabbix_confdir}
install -d %{buildroot}%{zabbix_confdir}/conf.d
install -d %{buildroot}%{zabbix_confdir}/bin
install -d %{buildroot}%{_sysconfdir}/init.d

# copy conf files
install -m 755 misc/conf/*.conf %{buildroot}%{zabbix_confdir}
install -m 711 misc/zabbix-rebase-server %{buildroot}%{zabbix_bindir}/zabbix-rebase-server

# copy config files
install -m 755 misc/pairs/conf.d/*.conf %{buildroot}%{zabbix_confdir}/conf.d
install -m 755 misc/checks/conf.d/*.conf %{buildroot}%{zabbix_confdir}/conf.d

# copy scripts files
install -m 755 misc/pairs/bin/*.sh %{buildroot}%{zabbix_confdir}/bin/
install -m 755 misc/checks/bin/*.{sh,awk,pl} %{buildroot}%{zabbix_confdir}/bin/

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
%attr(0755,root,root) %{zabbix_bindir}/hfsimport
%attr(0755,root,root) %{zabbix_bindir}/hfsevents
%attr(0755,root,root) %{zabbix_bindir}/hfsfilter
%attr(0755,root,root) %{zabbix_bindir}/hfs_trends_upd
%attr(0755,root,root) %{zabbix_bindir}/memcachetest
%{_sysconfdir}/init.d/zabbix_server

%files -n zabbix-agent
%defattr(-,root,root)
%dir %attr(0755,root,root) %{zabbix_confdir}
%dir %attr(0755,root,root) %{zabbix_confdir}/conf.d
%attr(0644,root,root) %{zabbix_confdir}/zabbix_agent.conf
%attr(0644,root,root) %{zabbix_confdir}/zabbix_agentd.conf
%attr(0644,root,root) %{zabbix_confdir}/zabbix_trapper.conf
%attr(0644,root,root) %{zabbix_confdir}/server.conf

%attr(0644,root,root) %{zabbix_confdir}/conf.d/*.conf
%attr(0755,root,root) %{zabbix_confdir}/bin/*.sh
%attr(0755,root,root) %{zabbix_confdir}/bin/*.awk
%attr(0755,root,root) %{zabbix_confdir}/bin/*.pl

%{_sysconfdir}/init.d/zabbix_agentd
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
