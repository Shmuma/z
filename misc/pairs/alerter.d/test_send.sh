#!/bin/sh

NAME=/tmp/$$-golem.data

cat > $NAME <<EOF
Action: ADD
AlertID: 22415
Monitor: monitor-eto.yandex.net
User: lapan
Clock: 1225354634
Value: 1
Name: Modem test trigger
Description: On: zabbix1, At: 2008.10.30 11:17:14, Modem test trigger: ON
Severity: critical
Host: zabbix1.yandex.net

Action: DELETE
AlertID: 22418
Monitor: monitor-eto.yandex.net
User: lapan
Clock: 1225354665
Value: 0
Name: Modem test trigger
Description: On: zabbix1, At: 2008.10.30 11:17:45, Modem test trigger: OFF
Severity: critical
Host: zabbix1.yandex.net
EOF

wget -d -v --post-file $NAME http://localhost/post
