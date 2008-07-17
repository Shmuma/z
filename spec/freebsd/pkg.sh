#!/bin/sh

cp -f ../../misc/zabbix-rebase-server sbin/
pkg_create -v -i inst -I pinst -f plist -s `pwd` -S `pwd` -p / -d descr -c comm zabbix_agent.tgz
