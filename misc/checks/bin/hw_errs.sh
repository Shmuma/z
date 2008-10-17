#!/bin/bash
#
# Tag kernel reported errors (from dmesg)
#
# $Id: hw_errs.sh,v 1.43 2008-06-10 17:52:49 andozer Exp $
#
me=${0##*/}     # strip path
me=${me%.*}     # strip extension
BASE=/tmp/
CONF=$BASE/etc/$me.conf
TMP=$BASE/tmp
PREV=$TMP/$me.prev
WATCH_LAST=3600 # seconds, e.g. 3 hours
PATH=/bin:/sbin:/usr/bin:/usr/sbin
TSTAMP=`date '+%s'`
ALARM_PAT='error|warning|fail|\(da[0-9]+:[a-z0-9]+:[0-9]+:[0-9]+:[0-9]+\)|mfi[0-9]|ip_conntrack:\ table\ full|messages\ suppressed'
IGNORE_PAT='thr_sleep|arplookup|page\ allocation\ failure|nfs\ send\ error\ 32\ |PCI\ error\ interrupt|(at [0-9a-f]+)? rip[: ][0-9a-f]+ rsp[: ][0-9a-f]+ error[: ]|smb|pid\ [0-9]+|swap_pager_getswapspace|uhub[0-9]|usbd|ukbd|optimal|rebuild|acpi_throttle[0-9]:\ failed\ to\ attach\ P_CNT|ipfw:\ pullup\ failed|MOD_LOAD'
CRIT_PAT='I/O|medium|defect|mechanical|retrying|broken|degraded|offline|failed|unconfigured_bad|domain_links.*segfault'

[ -s $CONF ] && . $CONF

[ -d $TMP ] || mkdir -p $TMP
[ -s $TMP/$me.dmesg.prev ] || touch $TMP/$me.dmesg.prev
[ -s $TMP/$me.msg.prev ] || touch $TMP/$me.msg.prev

die () {
        echo "PASSIVE-CHECK:$me;$1;$2"
        exit 0
}


dmesg | tail -n +2 | grep -i -E "$ALARM_PAT" | grep -v -i -E "$IGNORE_PAT" > $TMP/$me.dmesg.cur

diff $TMP/$me.dmesg.prev $TMP/$me.dmesg.cur | sed -e '/> / {
        s///
        p
}
d
' | awk -v t=$TSTAMP '{print t, $0}' >> $TMP/$me.msg.prev

awk -v p=$(($TSTAMP-$WATCH_LAST)) '$1>=p {print}' $TMP/$me.msg.prev > $TMP/$me.msg.cur
mv $TMP/$me.dmesg.cur $TMP/$me.dmesg.prev
mv $TMP/$me.msg.cur $TMP/$me.msg.prev
[ -s $TMP/$me.msg.prev ] || die 0 Ok

c_err=`grep -i -E "$CRIT_PAT" $TMP/$me.msg.prev | tail -n 1 | sed -e 's/^[0-9]* //'`
[ -n "$c_err" ] && die 2 "$c_err"
err=`tail -n 1 $TMP/$me.msg.prev | sed -e 's/^[0-9]* //'`
die 1 "$err"

