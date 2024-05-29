#!/bin/sh
#
#	@(#)netstatus.sh	1.2 90/08/27
#
#	Show status of last call(s) to <link>.
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

usage="
This command shows status of last call(s) to/from <link>."

SPOOLDIR=/usr/spool/MHSnet
eval `egrep 'SPOOLDIR' /usr/lib/MHSnetparams 2>/dev/null`
cd $SPOOLDIR

link=
case $count in
1)	for i in $args
	do
		case "$i" in
		-*|'')	echo "Usage: $0 link"
			echo "${usage}"
			exit 1	 ;;
		*)	link=$i ;;
		esac
	done ;;
*)	echo "Usage: $0 link"
	echo "${usage}"
	exit 1
	;;
esac

_bin/netaddr -L ${link} >/dev/null || exit 1
link=`_bin/netaddr ${link}`
node=`expr ${link} : '\([^\.]*\)'`
log=_call/${node}.log
oldlog=_call/${node}.ol

echo "Status for link \"${link}\":-

Control status:"
_bin/netcontrol status $link

echo "
Status of recent calls from \"_call/log\":"
egrep `netaddr -ti ${link}` _call/log | tail

echo "
Connection log of last call from end of \"${log}\":"
if [ -s ${log} ]
then
	tail ${log}
elif [ -s ${oldlog} ]
then
	tail ${oldlog}
else
	echo "(no call logs)"
fi

dir=`netpath -l ${link}`
log=${dir}/log
oldlog=${dir}/oldlog

echo "
Daemon log of last call from end of
\"${log}\":"
if [ -s ${log} ]
then
	egrep -v '^(Usages|Daemon|Children|Totals):' ${log} | tail -12
elif [ -s ${oldlog} ]
then
	egrep -v '^(Usages|Daemon|Children|Totals):' ${oldlog} | tail -12
else
	echo "(no daemon logs)"
fi

echo "
Accounts for recent calls from \"netac(8)\":"
_bin/netac ${link} '*log' | tail
