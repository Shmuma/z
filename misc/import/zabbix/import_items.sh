#!/bin/sh

# obtain history data of items we need to export. Arguments:
# 1. hostid to be exported
# 2. template's hostid to filter
# 3. directory to get data from
# 4. HFS prefix

# it creates bunch of files with names derived from key (all commas and [] are replaced with _)
in_dir=$3
out_dir=$4

mysql -B -e "select i.itemid,i.key_,i.value_type,i.delay from items i, items ii where i.hostid=$1 and ii.itemid = i.templateid and ii.hostid = $2 order by i.itemid"  zabbix | sed '1d' | \
while read itemid key type delay; do
    # prepare filename
    k=$(echo "$key" | tr '/,[]() .' '________')
    in="$in_dir/$k.dat"
    out="$out_dir/items/$(($itemid/1000))/$itemid"

    case $type in
        0|3)
            if [ -f $in ]; then
                echo Dumping $in to $out
                cp -f $out/history.meta $out/hist.meta
                co -f $out/history.data $out/hist.data
                (hfsdump $out/history.meta; cat $in) | sort -n -t = > $out/dump.txt
                hfsimport $out $out/dump.txt
                rm -f $out/dump.txt
            else
                echo Key $key skipped, no data for it
            fi
            ;;
    esac
done
