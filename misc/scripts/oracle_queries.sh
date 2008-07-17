#!/bin/sh

RES=$(echo "select value from v\$sysstat where statistic#=(select statistic# from v\$statname where name='user calls');" | sqlplus -S -L zabbix/zabbix | tail -n 2 | head -n 1 | sed 's/ //g')

echo ${RES:-0}
