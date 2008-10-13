#!/bin/sh

# script enqueue sms message to alamin message queue
gsgc --configfile /etc/zabbix-sms/gsgc.conf --send "$1" "$3"