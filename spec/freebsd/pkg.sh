#!/bin/sh

pkg_create -v -i inst -I pinst -f plist -s `pwd` -S `pwd` -p / -d descr -c comm zabbix_agent.tgz
