#!/bin/sh
#
#	@(#)wwwcache.sh	1.1 95/07/28
#
#	Push document into WWW cache at destination.

usage='name=`basename $0`

echo "Usage:	$name [-D server] <URL> <type> <document>
You will need to be a network SU and have OTHERHANDLERS permission."

exit 1'

#	Change "dest" to show your preferred cache's address.

dest=staff.cs.su.oz.au
flags=

skip=
for i
do
	case "$skip$i" in
	SKIP*)	skip=;					shift;;
	-D)	dest="$2"; skip=SKIP;			shift;;
	-D*)	dest="`expr $i : '-D\(.*\)'`";		shift;;
	*)						break;;
	esac
done

case $# in
3)	;;
*)	eval "$usage"
	;;
esac

SPOOLDIR=/var/spool/MHSnet
eval `egrep 'SPOOLDIR' /usr/lib/MHSnetparams 2>/dev/null`

case "$dest" in
'')	dest=`$SPOOLDIR/_bin/netaddr`		;;
esac

$SPOOLDIR/_bin/netmsg -Awwwpusher -D$dest -E"URL=$1;Content-type=$2" $3
