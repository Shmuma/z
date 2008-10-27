#!/bin/sh

# Multitail script. Outputs new part of some text file which appeared since last run.
# Usage: mtail.sh filename client

[ $# -ne 2 ] && echo "Usage: mtail.sh filename id_string" && exit 1

STATE_PATH=/tmp/zabbix/mtail/

FILE=$1
ID=`echo $FILE $2 | md5sum | cut -d ' ' -f 1`
OFS=0

[ ! -d $STATE_PATH ] && mkdir -p $STATE_PATH
[ -f $STATE_PATH/$ID ] && OFS=`cat $STATE_PATH/$ID`

NEW_OFS=$((`stat -c %s $FILE`+1))

# reset offset if file becomes smaller (rotated?)
[ $NEW_OFS -lt $OFS ] && OFS=0

tail -c +$OFS $FILE

echo $NEW_OFS > $STATE_PATH/$ID
