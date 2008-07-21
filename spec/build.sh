#!/bin/sh

set -x
for host in ztop1 zab1 dfs2; do
    ssh $host 'rm -rf rpm; mkdir rpm'
    scp $1 $host:rpm/
    ssh $host <<'EOF'
rm -rf /usr/src/redhat/{RPMS/*/*.rpm,SOURCES/*,BUILD/*}
cd rpm
FNAME=`ls -1`
echo $FNAME
cp $FNAME /usr/src/redhat/SOURCES/
tar xf $FNAME
EOF
done

ssh ztop1<<'EOF'
cd rpm
rpmbuild -ba */spec/zabbix-oracle.spec > log.txt 2>&1
rpmbuild -ba */spec/zabbix-conf-zabbix.spec > log-conf-zabbix.txt 2>&1
EOF

for host in zab1 dfs2; do
ssh $host<<'EOF'
cd rpm
rpmbuild -ba */spec/zabbix.spec > log.txt 2>&1
EOF
done

[ -d res ] && rm -rf res
mkdir res
for host in ztop1 zab1 dfs2; do
    ssh $host<<'EOF'
cd rpm
mkdir res
cp /usr/src/redhat/RPMS/*/*.rpm res/
ls -1 res/
EOF
    scp -r $host:rpm/res .
done
