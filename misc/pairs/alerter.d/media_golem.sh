#!/bin/sh

MHOST=$(host -t PTR `cat /etc/zabbix/zabbix_server.conf | grep ListenIP | cut -d = -f 2` | cut -d ' ' -f 5 | sed 's/\.$//')
OUT=/tmp/golem-reqs.txt

exec 2>> /tmp/golem-errs.txt

#ZABBIX_ALERT_ID=22373

SQL="select u.alias as User, a.clock as Clock, t.value as Value, t.description as Name, a.message as Description, if (t.priority > 2,'critical','warning') as Severity, h.host as Host from alerts a, triggers t, users u, functions f,items i, hosts h where a.alertid=$ZABBIX_ALERT_ID and a.triggerid=t.triggerid and a.userid=u.userid and f.triggerid=t.triggerid and f.itemid=i.itemid and h.hostid=i.hostid limit 1\G";

echo "" >> $OUT
echo "AlertID: $ZABBIX_ALERT_ID" >> $OUT
echo "Monitor: $MHOST" >> $OUT
mysql zabbix -B --user=root -e "$SQL" | sed 's/^ *//g' | tail -n +2 >> $OUT
