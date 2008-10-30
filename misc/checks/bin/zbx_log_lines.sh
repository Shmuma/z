#!/bin/sh
# Uses mtail to count total amount of log lines. Useful to calculate RPS of different progs.

LINES=$(/etc/zabbix/bin/mtail.sh $1 log_lines | wc -l)
STATE_PATH=/tmp/zabbix/log_lines

[ ! -d $STATE_PATH ] && mkdir -p $STATE_PATH

ID=`echo $1 | md5sum | cut -d ' ' -f 1`

COUNT=0
[ -f $STATE_PATH/$ID ] && COUNT=$(cat $STATE_PATH/$ID)

echo $COUNT + $LINES | bc | tee $STATE_PATH/$ID
