#! /bin/sh

HELP="usage: $0 [-n name] [-h host] [-p ping] [-t timeout]"
nameclt="/usr/bin/nameclt"
convertior="/usr/bin/convertior"
pingior="/usr/local/bin/pingior"
host="127.0.0.1"
timeout=10
ping=""
string=1

if [ ! -x $nameclt ] || [ ! -x $pingior ] || [ ! -x $convertior ]
then
	echo 2
	echo "Can't found nameclt, convertior or pingior" >&2
	exit 2
fi      

if [ $# = 0 ]
then
	echo 2
	echo $HELP >&2
	exit 2
fi;	


while getopts "vpt:n:h:" flag
do
	if [ x"$flag" = "x?" ]
	then
		echo $HELP 
		exit 2
	fi

	if [ "x${flag}" = "xh" ]
	then
		host=$OPTARG
	fi
	if [ "x${flag}" = "xn" ]
	then
		name=$OPTARG
	fi

	if [ "x${flag}" = "xp" ]
	then
		string=0
		ping="-p"
	fi;

	if [ "x${flag}" = "xt" ]
	then
		timeout=$OPTARG
	fi	

done

ior=`$nameclt resolve $name 2>/dev/null`
if [ $? != 0 ]
then
	echo 2
	echo "Can't resolve $name" >&2
	exit 2
fi

converted_ior=`$convertior $ior $host 2>/dev/null`

if [ $? != 0 ]
then
	echo 2
	echo "Can't convert ior($ior) into name($name)" >&2
	exit 2
fi

result=`$pingior -t $timeout -i $converted_ior $ping -v`
exit_code=$?

if [ x"$string" = "x0" ]
then
	echo $exit_code
	exit 0
fi	
if [ x"$string" = "x1" ]
then
	echo $result | cut -f1 -d";"
	echo $result | cut -f2 -d";" >&2
	exit 0
fi
