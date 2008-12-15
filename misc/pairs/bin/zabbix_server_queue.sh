#!/bin/sh

#set -x

[ $# -ne 2 ] && echo "Usage: $0 max_age|max_gap|max_idx|min_idx data|history" && exit 1

case $2 in
    data)
        hist=0
        ;;
    history)
        hist=1
        ;;
    *)
        echo $0: second argument error
        exit 1
esac

ts=`date +%s`
q_path=/dev/shm/zabbix_queue

[ ! -d $q_path ] && echo 0 && exit 0


function get_q_size ()
{
    name=$1
    proc=$2
    size=0

    for n in $name.$proc.*; do
        size=$(($size+`stat -c %s $n`))
    done

    echo $size
}


exec 2> /dev/null


case $1 in
    max_age)
        val=$(stat -c %Y $q_path/queue_$hist.*.* | while read n; do echo $(($ts-$n)); done | sort -nr | head -n 1)
        ;;
    max_gap)
        val=$(ls -1 $q_path/queue_${hist}_pos.* | sed 's/\./ /g' | 
            while read n proc; do 
                nn=`echo $n | sed 's/_pos//'`;
                s=`get_q_size $nn $proc`
                cat $n.$proc | (read idx ofs;
                    echo $(($s-$ofs)) )
            done | sort -nr | head -n 1)
        ;;
    max_idx)
        val=$(ls -1 $q_path/queue_$hist.*.* | sed 's/.*\.//g' | sort -nr | head -n 1)
        ;;
    min_idx)
        val=$(ls -1 $q_path/queue_$hist.*.* | sed 's/.*\.//g' | sort -n | head -n 1)
        ;;
    size)
        val=$(get_q_size $q_path/queue_$hist \*)
        ;;
    *)
        echo $0: first argument error
        exit 1
esac

[ -z $val ] && echo 0 || echo $val
