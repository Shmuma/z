#!/bin/sh

TMP=/tmp/$$-ms.xml

die ()
{
    echo $1
    echo $2 >&2
    rm -f $TMP
    exit 0
}

wget --timeout=30 -q -O - "http://viewer:Pa\$\$w0rd!@$1/RCServer/systeminfo.xml" | tail -c +4 > $TMP

[ ! -f $TMP ] && die 2 "Error obtaining XML from service"
[ ! -s $TMP ] && die 2 "XML is empty"

# validate
xmlstarlet val $TMP 2>&1 > /dev/null; err=$?

if test $err -eq 0; then
    die 0 ""
else
    die 2 "XML is not valid"
fi