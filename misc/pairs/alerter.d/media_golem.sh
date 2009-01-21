#!/bin/sh

#MHOST=$(host -t PTR `cat /etc/zabbix/zabbix_server.conf | grep ListenIP | cut -d = -f 2` | cut -d ' ' -f 5 | sed 's/\.$//')
OUT=/tmp/golem-reqs-$$.txt

exec 2> /tmp/golem-errs.txt

#set -x
#ZABBIX_ALERT_ID=83077

SQL="select u.alias as User, a.clock as Clock, t.value as Value, t.description as Name, a.message as Description, if (t.priority > 2,'critical','warning') as Severity, h.host as Host from alerts a, triggers t, users u, functions f,items i, hosts h where a.alertid=$ZABBIX_ALERT_ID and a.triggerid=t.triggerid and a.userid=u.userid and f.triggerid=t.triggerid and f.itemid=i.itemid and h.hostid=i.hostid limit 1\G";

#echo "" >> $OUT
#echo "AlertID: $ZABBIX_ALERT_ID" >> $OUT
#echo "Monitor: $MHOST" >> $OUT
mysql zabbix -B --user=root -e "$SQL" | sed 's/^ *//g' | tail -n +2 > $OUT

host=$(cat $OUT | grep Host | cut -f 2 -d ' ')
event=$(cat $OUT | grep Name | cut -f 2- -d ' ')
descr=$(cat $OUT | grep Description | cut -f 2- -d ' ' | sed 's/&/%26;/g')
value=$(cat $OUT | grep Value | cut -f 2 -d ' ')

#echo $host $event $value $descr

extra=""

if [ $value = 0 ]; then
	extra="&status=ok"
fi

#wget --no-check-certificate -q -O /dev/null "https://golem.yandex.net/api/event.sbml?object=$host&eventtype=$event&info=$descr$extra"
wget --no-check-certificate -q -O /dev/null "https://golem.yandex.net/api/event.sbml?object=$host&eventtype=$event&info=$descr$extra"


rm -f $OUT
