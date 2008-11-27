#!/bin/sh

ITEMS_FILE=/tmp/items-$$.txt
DIRS_FILE=/tmp/dirs-$$.txt
HFS_PATH=/gpfs/`/etc/zabbix/bin/zbx_site.sh`/items

mysql zabbix -BN -e 'select itemid from items order by itemid' > $ITEMS_FILE
ls -1d $HFS_PATH/* | while read d; do ls -1d $d/*; done | sed 's/\/.*\///g' | sort -n > $DIRS_FILE

if [ ! -s $ITEMS_FILE -o ! -s $DIRS_FILE ]; then
    exit 0;
fi

COUNT=$(comm -23 $DIRS_FILE $ITEMS_FILE | wc -l)
TOTAL=$(cat $ITEMS_FILE | wc -l)

echo -n "Are you sure to clean $COUNT items' dirs from $HFS_PATH? Total items $TOTAL. [y/n]: "
read ans
if [ $ans != "y" ]; then
    echo Not confirmed!
    exit 1
fi

COUNT=0
TOTAL=0

comm -23 $DIRS_FILE $ITEMS_FILE | while read item; do
    DIR=`echo $item | sed 's/...$//'`
    P=$HFS_PATH/$DIR/$item
    COUNT=$(($COUNT+1))
    SIZE=`du -sb $P | cut -f 1`
    TOTAL=$(echo $TOTAL + $SIZE | bc)
    echo -en "Item $COUNT, freed $TOTAL bytes\r"
done

rm -f $ITEMS_FILE $DIRS_FILE