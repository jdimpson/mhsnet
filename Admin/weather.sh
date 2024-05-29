#!/bin/sh
#
#	@(#)netweather.sh	1.2 91/10/21
#
#	Transmit weather image to destination

usage="Usage: $0 -D destination imagename"

dest=
pri=

while [ $# -gt 0 ]
do
	case "$1" in
	-D)	dest="$2"; shift;			shift	;;
	-D*)	dest=`expr "$1" : '-D\(.*\)'`;		shift	;;
	-P)	pri="-P$2"; shift;			shift	;;
	-P*)	pri="$1";				shift	;;
	-*)	echo "$usage";				exit 1	;;
	*)						break	;;
	esac
done

case $# in
	1)							;;
	*)	echo "$usage";				exit 1	;;
esac

case "$dest" in
	"")	echo "$usage";				exit 1	;;
esac

SPOOLDIR=/var/spool/MHSnet
eval `egrep 'SPOOLDIR' /usr/lib/MHSnetparams 2>/dev/null`

privs=`$SPOOLDIR/_bin/netprivs`
logname=`expr "$privs" : '\(.*\):.*'`
fname=`basename $1`

$SPOOLDIR/_bin/netmsg -dirx -Aweather -D$dest -E"fname=$fname;logname=$logname" $pri "$@"
