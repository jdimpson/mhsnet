#!/bin/sh
#
#	Shell script to kill dial-up daemons exceeding time.
#
#	@(#)call_term.sh	1.3 90/03/31
#

PATH=/bin:/usr/bin:/usr/ucb:/usr/local:/usr/local/bin
export PATH

case $# in
0)	echo "Usage: $0 link ..."; exit 1;;
*)	links=$1; shift;;
esac

for i
do
	links="$links $i"
done

SPOOLDIR=/usr/spool/MHSnet
eval `egrep 'SPOOLDIR' /usr/lib/MHSnetparams 2>/dev/null`

cd $SPOOLDIR

for link in $links
do
	stats=`_bin/netlink -v $link|grep $link`

	case "$stats" in
		''|*inactive*)	continue	;;
	esac

	pgrp=`expr "$stats" : '.*([^0-9]*\([^)/]*\)'`
	cpu=`expr "$stats" : '.*(\([^0-9 ]*\)'`

	case "$cpu" in
	'')
		echo "$0: terminating call from $link (pgrp=$pgrp)"
		kill -15 $pgrp >/dev/null 2>&1
		;;
	*)
		echo "$0: terminating call from $link (pgrp=[$cpu $pgrp])"
		rsh $cpu kill -15 $pgrp >/dev/null 2>&1
		;;
	esac

	sleep 10

	stats=`_bin/netlink -v $link|grep $link`

	case "$stats" in
		''|*inactive*)	continue	;;
	esac

	pgrp=`expr "$stats" : '.*([^0-9]*\([^)/]*\)'`
	cpu=`expr "$stats" : '.*(\([^0-9 ]*\)'`

	case "$cpu" in
	'')
		echo "$0: terminate of call from $link (pgrp=$pgrp) FAILED"
		;;
	*)
		echo "$0: terminate of call from $link (pgrp=[$cpu $pgrp]) FAILED"
		;;
	esac
done
