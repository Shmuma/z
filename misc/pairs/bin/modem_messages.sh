#!/bin/sh

[ $# -ne 2 ] && echo "Usage: $0 today|total queued|out" && exit 1

LOG=/var/log/zabbix-sms/gsgd-accounting.log

case $2 in
    queued)
        queued=1
        ;;
    out)
        queued=0
        ;;
    *)
        echo Second argument is invalid
        exit 0
        ;;
esac

ts=`date +%s`
day=$(($ts / 86400))
from=$(($day * 86400))
to=$((($day+1) * 86400))


case $1 in
    today)
        cat $LOG | \
        if [ $queued -eq 1 ]; then
            grep -e '^[0-9]*,OUT,OK,queue' | awk -F , -v from=$from -v to=$to '{ if ($1 >= from && $1 <= to) print $0; }'
        else
            grep -e '^[0-9]*,OUT,OK,at' | awk -F , -v from=$from -v to=$to '{ if ($1 >= from && $1 <= to) print $0; }'
        fi | wc -l
        ;;
    total)
        cat $LOG | \
        if [ $queued -eq 1 ]; then
            grep -e '^[0-9]*,OUT,OK,queue'
        else
            grep -e '^[0-9]*,OUT,OK,at'
        fi | wc -l
        ;;
    *)
        echo First argument is invalid
        exit 0
        ;;
esac

