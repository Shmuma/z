#!/bin/sh

# obtain history data of items we need to export. Arguments:
# 1. hostid to be exported
# 2. template's hostid to filter
# 3. directory to save results

# it creates bunch of files with names derived from key (all commas and [] are replaced with _)
dir=$3

mysql -B -e "select i.itemid,i.key_,i.value_type,i.delay from items i, items ii where i.hostid=$1 and ii.itemid = i.templateid and ii.hostid = $2 order by i.itemid"  zabbix | sed '1d' | \
while read itemid key type delay; do
    # prepare filename
    k=$(echo "$key" | tr '/,[]() .' '________')
    out="$dir/$k.dat"
    rm -f $out
    case $type in
        0)  # float
            echo "Dumping float item $itemid ($key) to $out"
            mysql -B -e "select clock,value from history where itemid=$itemid order by clock" zabbix | sed '1d' | \
            while read clock val; do
                echo -e "time=$clock\tdelay=$delay\ttype=1\tvalue=$val" >> $out
            done
            ;;

        3)  # uint64
            echo "Dumping int item $itemid ($key) to $out"
            mysql -B -e "select clock,value from history_uint where itemid=$itemid order by clock" zabbix | sed '1d' | \
            while read clock val; do
                echo -e "time=$clock\tdelay=$delay\ttype=0\tvalue=$val" >> $out
            done
            ;;
    esac
done
