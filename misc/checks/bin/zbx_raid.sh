#!/bin/sh
#
# $Id: raid.sh,v 1.4 2008-05-30 09:24:43 andozer Exp $
#
me=${0##*/}     # strip path
me=${me%.*}     # strip extension
CONF=$ZBX_CONFDIR/checks/${me}.conf
CONF2=/home/monitor/etc/raid.conf
CONF3=/home/monitor/etc/bsd_raid.conf
TMP=/tmp/$me
PATH=/bin:/sbin:/usr/bin:/usr/sbin
err_c=0


die () {
        echo "$1"
        echo "$2" 1>&2
        exit 0
}

[ -s $CONF ] && . $CONF
[ -s $CONF2 ] && . $CONF2
[ -s $CONF3 ] && . $CONF3

raid_linux ()
{
    STAT_OID=/proc/mdstat

    [ -r $STAT_OID ] || die 2 "Can not read $STAT_OID"
    # Get arrays
    set -- `cat $STAT_OID | tail -n +2 | grep raid | cut -d " " -f 1`
    for i in "$@"
    do
        i=/dev/$i
        # prepare data for analysis
        set -- `sudo /sbin/mdadm --detail $i | grep 'faulty\|remove\|degraded'`
        i=${i##*\\}

        [ "$*" ] || continue

        shift 4

        ERR="$i:$*"

        set -- `grep recovery $STAT_OID`

        [ "$*" ] && {
                ERR="$ERR $2 $3 $4" 
                set --
                set -- `echo $ERR | grep '%'`
                [ "$*" ] && {
                        [ "$err_c" == "2" ] || err_c=1
                        ERR_MES="$ERR_MES $ERR"
                        continue
                }
        }

        case "$ERR" in
        *faulty*)       err_c=2
                        ;;
        *removed*)      set -- `grep -A1 $i $STAT_OID | tail -1`
                        err_c=2
                        ;;
        *)              err_c=2
                        ;;
        esac
        ERR_MES="$ERR_MES $ERR $*"

    done

    [ "$ERR_MES" ] || ERR_MES=""
    die $err_c "$ERR_MES"
}

raid_bsd ()
{
    err_os=0
    ReleaseLimit=5
    VersionLimit=4
    EXT=0
    EXT2=0

    # Check version of release
    a=`uname -r | sed 's/[A-Za-z\-]//g' | cat | awk -v rel=$ReleaseLimit -v ver=$VersionLimit 'BEGIN { FS = "." }
    {
        ret = 0;
        CurrentRelease = $1;
        CurrentVersion = $2;
        if (CurrentRelease < rel)
        {
                ret = 1;
        }
        if (CurrentRelease == rel)
        {
                if (CurrentVersion < ver)
                {
                        ret = 1;
                }
        }
        print ret;
    }'`

    [ $a -eq 0 ] || die 0 "freebsd current version less $ReleaseLimit.$VersionLimit"

    set -- `/sbin/gmirror status 2>/dev/null | grep -v Components | awk 'BEGIN { cc = 0; }
    {
        if ($0 ~ /mirror/)
        {
                if (cc != 0)
                {
                        printf "\n";
                }
        }
        for (i = 1; i <= NF; i++)
        {
                printf "%s ", $i;
        }
        cc = cc + 1;
    } END { printf "\n"; }' | awk 'BEGIN { ext = 0; }
    {
        if ($0 !~ /COMPLETE/)
        {
                if ($0 ~ /%/)
                {
                        if (ext < 1 ) ext = 1;
                        err_str = err_str $0;
                }
                else
                {
                        if ($0 ~ /DEGRADED/)
                        {
                                ext = 2;
                                err_str = err_str $0;
                        }
                }
        }
    } END { printf "%d %s", ext, err_str; }'`
    EXT=$1
    shift
    err_str="$*"

    set -- `/sbin/gstripe status 2>/dev/null | grep -v Components | awk 'BEGIN { cc = 0; }
    {
        if ($0 ~ /stripe/)
        {
                if (cc != 0)
                {
                        printf "\n";
                }
        }
        for (i = 1; i <= NF; i++)
        {
                printf "%s ", $i;
        }
        cc = cc + 1;
    } END { printf "\n"; }' | awk 'BEGIN { ext = 0; }
    {
        if ($0 !~ /UP/)
        {
                if ($0 ~ /DEGRADED/)
                {
                        ext = 2;
                        err_str = err_str $0;
                }
        }
    } END { printf "%d %s", ext, err_str; }'`
    EXT2=$1
    shift
    err_str2="$*"

    C=$EXT
    MESS="$err_str $err_str2"
    [ $EXT2 -gt $C ] && C=$EXT2
    [ $C -eq 0 ] && MESS=""

    die $C "$MESS"
}


case `uname` in
    FreeBSD) 
        raid_bsd $@
        ;;
    Linux)
        raid_linux $@
        ;;
    *)       die 0 "unsupported OS"
        ;;
esac