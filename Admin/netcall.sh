#!/bin/sh
#
#	@(#)netcall.sh	1.5 96/02/09
#
#	Start a call, track the connection,
#	show the link statistics, print the result.
#
#	(First collect the args, as the `eval' below destroys them.)
count=$#
case $count in
0)	args=;;
*)	args=$1
	shift
	for i
	do
		args="$args $i"
	done
	;;
esac

usage="Usage: $0 [-q] link

	This command starts a call to <link>,
	then tracks the progress of the connection,
	finally displaying the link throughput statistics.

	The \"-q\" option starts the call only
	if the queue to <link> is non-empty."

SPOOLDIR=/usr/spool/MHSnet
eval `egrep 'SPOOLDIR' /usr/lib/MHSnetparams 2>/dev/null`
cd $SPOOLDIR

checkq=
link=
err=

case $count in
1|2)	for i in $args
	do
		case "$i" in
		-q)	checkq=true	;;
		-*|'')	err=true	;;
		*)	case "$link" in
			'')	link=$i	;;
			*)	err=true;;
			esac
		esac
	done	;;
*)	err=true;;
esac

case "$err" in
true)	echo "${usage}"
	exit 1	;;
esac

_bin/netaddr -L ${link} >/dev/null || exit 1
link=`_bin/netaddr ${link}`

case "$checkq" in
true)	netq -facs ${link} && exit 0
	;;
esac

node=`expr ${link} : '\([^\.]*\)'`
log=_call/${node}.log

tmp=/tmp/td$$
>${tmp}

# if more than one method - choose `call'

found=false
for word in `_bin/netcontrol status $link`
do
	case $word in
	call-*)	link=$word	;;
	esac
	found=true
done

case $found in
false)	echo "no entry in ${SPOOLDIR}/_lib/initfile to call!"
	exit 2	;;
esac

_bin/netcontrol start $link

trap "trap 'exit 0' 2; rm -f ${tmp}; _bin/netlink -vc1 ${link} | _bin/netdis" 2

echo "
<intr> to get link statistics, after log below:

tail -f ${SPOOLDIR}/${log}"

while t=`find ${log} -newer ${tmp} -print 2>/dev/null; exit 0`
do
	case "$t" in
	'')	sleep 1;;
	*)	break 1;;
	esac
done

tail -f ${log}
