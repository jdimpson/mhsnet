#!/bin/sh
#
#	@(#)netpeeper.sh	1.3 94/01/05
#
#	Return internet host information via DNS server at destination.
#	The handler defaults to looking up "mx" records, but other
#	record types can be returned by using the -Q option.

usage='"Usage: `basename $0` [-D dig_server] [-Q query_type] [@DNS_server] host"'

dest=
qtype=";"
dnsserver=";"

while [ $# -gt 0 ]
do
	case $1 in
	@*)	dnsserver=";$1";			shift	;;
	-D)	dest="$2"; shift;			shift	;;
	-D*)	dest=`expr $1 : '-D\(.*\)'`;		shift	;;
	-Q)	qtype=";$2"; shift;			shift	;;
	-Q*)	qtype=";`expr $1 : '-Q\(.*\)'`";	shift	;;
	-*)	eval echo "$usage";			exit 1	;;
	*)						break	;;
	esac
done

case $# in
	1)							;;
	*)	eval echo "$usage";			exit 1	;;
esac

SPOOLDIR=/usr/spool/MHSnet
eval `egrep 'SPOOLDIR' /usr/lib/MHSnetparams 2>/dev/null`
cd $SPOOLDIR

case "$dest" in
	"")	dest=`_bin/netaddr`			;;
esac

privs=`_bin/netprivs`

logname=`expr "$privs" : '\(.*\):.*'`

_bin/netmsg -Apeeper -D$dest -E"$logname;$1$qtype$dnsserver" </dev/null
