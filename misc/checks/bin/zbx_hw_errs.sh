#!/bin/bash
#
# Zabbix script which checks dmesg for errors
#
# Based on: hw_errs.sh,v 1.43 2008-06-10 17:52:49 andozer Exp
#
me=zbx_hw_errs     # strip path
TMP=/tmp/zabbix/
CONF=/etc/zabbix/checks/$me.conf
PREV=$TMP/$me.prev

#set -x

# How long to keep dmesg errors
WATCH_LAST=3600 # seconds, e.g. 3 hours

PATH=/bin:/sbin:/usr/bin:/usr/sbin

TSTAMP=`date '+%s'`

ALARM_PAT='error|warning|fail|\(da[0-9]+:[a-z0-9]+:[0-9]+:[0-9]+:[0-9]+\)|mfi[0-9]|ip_conntrack:\ table\ full|messages\ suppressed'
IGNORE_PAT='thr_sleep|arplookup|page\ allocation\ failure|nfs\ send\ error\ 32\ |PCI\ error\ interrupt|(at [0-9a-f]+)? rip[: ][0-9a-f]+ rsp[: ][0-9a-f]+ error[: ]|smb|pid\ [0-9]+|swap_pager_getswapspace|uhub[0-9]|usbd|ukbd|optimal|rebuild|acpi_throttle[0-9]:\ failed\ to\ attach\ P_CNT|ipfw:\ pullup\ failed|MOD_LOAD'
CRIT_PAT='REJECT|I/O|medium|defect|mechanical|retrying|broken|degraded|offline|failed|unconfigured_bad|domain_links.*segfault'

[ -s $CONF ] && . $CONF

[ -d $TMP ] || mkdir -p $TMP
[ -s $TMP/$me.dmesg.prev ] || touch $TMP/$me.dmesg.prev
[ -s $TMP/$me.msg.prev ] || touch $TMP/$me.msg.prev

dmesg | tail -n +2 | grep -i -E "$ALARM_PAT" | grep -v -i -E "$IGNORE_PAT" > $TMP/$me.dmesg.cur

WARN_LINES_BEFORE=`wc -l < $TMP/$me.msg.prev`
ERR_LINES_BEFORE=`grep -i -E "$CRIT_PAT" $TMP/$me.msg.prev | wc -l`

diff $TMP/$me.dmesg.prev $TMP/$me.dmesg.cur | sed -e '/> / {
        s///
        p
}
d
' | awk -v t=$TSTAMP '{print t, $0}' >> $TMP/$me.msg.prev

WARN_LINES_AFTER=`wc -l < $TMP/$me.msg.prev`
ERR_LINES_AFTER=`grep -i -E "$CRIT_PAT" < $TMP/$me.msg.prev | wc -l`
WARN_LINES_DELTA=$(($WARN_LINES_AFTER - $WARN_LINES_BEFORE))
ERR_LINES_DELTA=$(($ERR_LINES_AFTER - $ERR_LINES_BEFORE))

# throw away old data
awk -v p=$(($TSTAMP-$WATCH_LAST)) '$1>=p {print}' $TMP/$me.msg.prev > $TMP/$me.msg.cur
mv $TMP/$me.dmesg.cur $TMP/$me.dmesg.prev
mv $TMP/$me.msg.cur $TMP/$me.msg.prev

c_err=`grep -i -E "$CRIT_PAT" < $TMP/$me.msg.prev | tail -n 1 | sed -e 's/^[0-9]* //'`
err=`tail -n 1 $TMP/$me.msg.prev | sed -e 's/^[0-9]* //'`

# primary data: amount of new error lines and last line in stderr
echo $ERR_LINES_DELTA
echo "$c_err" 1>&2

# send amount of warning data using zabbix_sender
zabbix_sender -c /etc/zabbix/zabbix_agentd.conf -k unix.kernel.warning.count -o $WARN_LINES_DELTA > /dev/null
zabbix_sender -c /etc/zabbix/zabbix_agentd.conf -k unix.kernel.warning.messages -o "$err" > /dev/null
