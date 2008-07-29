%define realname	zabbix

Name: zabbix-oracle
Version: 1.4.4
Release: yandex_11
Group: System Environment/Daemons
License: GPL
Source: %{realname}-%{version}_yandex11.tar.gz
BuildRoot: %{_tmppath}/%{name}-root
BuildPrereq: libsqlora8-devel, net-snmp-devel, setproctitle-devel, iksemel-devel, pkgconfig
Requires: libsqlora8, net-snmp, setproctitle, iksemel
Summary: A network monitor.

%define zabbix_bindir 	        %{_sbindir}
%define zabbix_confdir 		%{_sysconfdir}/%{realname}
%define zabbix_run 		%{_localstatedir}/run/zabbix
%define zabbix_log 		%{_localstatedir}/log/zabbix
%define zabbix_spool 		%{_localstatedir}/spool/zabbix
%define zabbix_www		/var/www/html/zabbix

%description
zabbix is a network monitor.

%package -n zabbix-phpfrontend
Summary: Zabbix web frontend (php).
Group: System Environment/Daemons
Requires: php php-common php-oci8 php-gd php-bcmath php-cli

%description -n zabbix-phpfrontend
A php frontent to zabbix.

%prep
%setup -q -n %{realname}-%{version}_yandex11

%build
%configure --enable-server --with-oracle --with-jabber --with-net-snmp
make

# adjust in several files /home/zabbix
HOSTNAME=$(hostname -f)

for zabbixfile in misc/conf/* misc/init.d/redhat/{zabbix_agentd,zabbix_server}; do
    sed -i -e "s#BASEDIR=.*#BASEDIR=%{zabbix_bindir}#g" \
        -e "s#PidFile=/var/tmp#PidFile=%{zabbix_run}#g" \
        -e "s#LogFile=/tmp#LogFile=%{zabbix_log}#g" \
        -e "s#ActiveChecksBufFile=/var/tmp#ActiveChecksBufFile=%{zabbix_spool}#g" \
        -e "s#AlertScriptsPath=/home/zabbix#AlertScriptsPath=%{zabbix_confdir}#g" \
        -e "s#Hostname=.*#Hostname=$HOSTNAME#g" \
        -e "s#/home/zabbix/lock#%{_localstatedir}/lock#g" $zabbixfile
done


%pre
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

%preun
if [ "$1" = 0 ]
then
  /sbin/service zabbix_server stop >/dev/null 2>&1 || :
  /sbin/chkconfig --del zabbix_server
fi

%clean
rm -fr %buildroot

%install
rm -fr %buildroot
%makeinstall

# create directory structure
install -d %{buildroot}%{zabbix_confdir}
install -d %{buildroot}%{_sysconfdir}/init.d
install -d %{buildroot}%{zabbix_www}

# copy conf files
install -m 755 misc/conf/zabbix_server.conf %{buildroot}%{zabbix_confdir}

# redhat install scripts
install -m 755 misc/init.d/redhat/zabbix_server %{buildroot}%{_sysconfdir}/init.d/

# frontend
cp -r frontends/php/* %{buildroot}%{zabbix_www}/

%files
%defattr(-,root,root)
%doc AUTHORS COPYING NEWS README INSTALL create upgrades
%dir %attr(0755,root,root) %{zabbix_confdir}
%attr(0644,root,root) %config(noreplace) %{zabbix_confdir}/zabbix_server.conf
%attr(0755,root,root) %{zabbix_bindir}/zabbix_server
%attr(0755,root,root) %{zabbix_bindir}/hfsdump
%attr(0755,root,root) %{zabbix_bindir}/hfs_trends_upd
%config(noreplace) %{_sysconfdir}/init.d/zabbix_server

%files -n zabbix-phpfrontend
%defattr(0644,root,root,0755)
%{zabbix_www}

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
