#!/bin/sh

# script enqueue sms message to alamin message queue
# obtaining message priority
PRIO=5

set -x 
exec 2> /tmp/sms-errs.txt

if [ ! -z $ZABBIX_ALERT_ID ]; then
    PRIO=`mysql zabbix --connect_timeout=1 -u root -e "select t.priority from triggers t, alerts a where t.triggerid=a.triggerid and a.alertid=$ZABBIX_ALERT_ID" -B -N`

    echo $PRIO >> /tmp/prio.txt
    if [ a$PRIO = a ]; then
	PRIO=5
    fi

    if [ ! \( $PRIO -ge 0 -a $PRIO -le 5 \) ]; then
        PRIO=5
    fi
fi

PRIO=$((6-$PRIO))

gsgc --configfile /etc/zabbix-sms/gsgc.conf --priority $PRIO --send "$1" "$3"
RET=$?

# if send was not successfull, trying other machine in pair
if [ $RET -ne 0 ]; then
    if [ -f /etc/zabbix-sms/backup-hosts.conf ]; then
        for h in `cat /etc/zabbix-sms/backup-hosts.conf`; do 
            gsgc --configfile /etc/zabbix-sms/gsgc.conf --host $h --priority $PRIO --send "$1" "$3"
            RET=$?
            if [ $RET -eq 0 ]; then
                exit 0
            fi
        done
    fi
fi

exit $RET
