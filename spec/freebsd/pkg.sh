#!/bin/sh

cp -f ../../misc/zabbix-rebase-server sbin/
cp -f ../../misc/checks/bin/* etc/zabbix/bin/
cp -f ../../misc/checks/conf.d/* etc/zabbix/conf.d/

VER=1.4.4_18

# replace version in @name tag
cat plist | sed "s/^@name.*$/@name zabbix_agent_yandex-$VER/" > plist.tmp && mv plist.tmp plist
pkg_create -v -i inst -I pinst -f plist -s `pwd` -S `pwd` -p / -d descr -c comm zabbix_agent_yandex-$VER.tgz
