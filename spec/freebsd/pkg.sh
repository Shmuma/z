#!/bin/sh

cp -f ../../misc/zabbix-rebase-server usr/local/sbin/
cp -f ../../misc/checks/bin/* usr/local/etc/zabbix/bin/
cp -f ../../misc/checks/conf.d/* usr/local/etc/zabbix/conf.d/

VER=1.4.4_60

# replace version in @name tag
cat > plist <<EOF
@name zabbix_agent_yandex-$VER
@cwd /
EOF

cat plist | sed "s/^@name.*$/@name zabbix_agent_yandex-$VER/" > plist.tmp && mv plist.tmp plist
find etc usr -type f >> plist

pkg_create -v -i inst -I pinst -f plist -s `pwd` -S `pwd` -p / -d descr -c comm zabbix_agent_yandex-$VER.tgz
