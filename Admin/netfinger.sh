#!/bin/sh
#
#	@(#)netfinger.sh	1.2 92/06/28
#
#	Return network user information via finger server at destination.

usage='"Usage: `basename $0` [-D finger_server] [user]@host ..."'

dest=basser.cs.su.oz.au		# Default
users=

for i
do
	case "$i" in
	-D)	dest=$2; shift;			shift	;;
	-D*)	dest=`expr $i : '-D\(.*\)'`;	shift	;;
	-*)	eval echo "$usage";		exit 1	;;
	*)					break	;;
	esac
done

case $# in
0)	eval echo "$usage";	exit 1	;;
esac

for i
do
	case "$i" in
	*@?*)	users="$users $i"		;;
	*)	eval echo "$usage";	exit 1	;;
	esac
done

SPOOLDIR=/usr/spool/MHSnet
eval `egrep 'SPOOLDIR' /usr/lib/MHSnetparams 2>/dev/null`
cd $SPOOLDIR

case "$dest" in
"")	dest=`_bin/netaddr`	;;
esac

privs=`_bin/netprivs`

logname=`expr "$privs" : '\(.*\):.*'`

echo $logname $users | _bin/netmsg -Afinger -D$dest
