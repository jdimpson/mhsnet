#!/bin/sh
#
#	@(#)checklog.sh	1.4 91/06/28
#
#	Find information from logs concerning link(s).
#
#	(First collect the args, as the `eval' below destroys them.)

usage="Usage: $0 link ...
Collects data from last calls to/from \"link\"
(Pipe output through \"more\".)"

count=$#
case $count in
0)	echo "$usage"
	exit 1
	;;
1)	args="$1"
	case "$args" in
	-?)	echo "$usage"
		exit 1
		;;
	esac
	;;
*)	args="$1"
	shift
	for i
	do
		args="$args $i"
	done
	;;
esac

SPOOLDIR=/usr/spool/MHSnet
eval `egrep 'SPOOLDIR' /usr/lib/MHSnetparams 2>/dev/null`
#
cd $SPOOLDIR

for link in $args
do
	link=`_bin/netaddr $link`
	node=`expr $link : '\([^\.]*\)'`
	dir=`_bin/netpath -l $link`
	case "$dir" in
	'')	exit 1
		;;
	*)	if [ -s $dir/log ]
		then
			log=log
		elif [ -s $dir/oldlog ]
		then
			log=oldlog
		elif [ -s $dir/olderlog ]
		then
			log=olderlog
		else
			echo "No record of call to/from \"$link\"."
			exit 1
		fi
		;;
	esac

	case `egrep $node _call/log 2>/dev/null` in
	'')	;;
	*)	echo "Details of last calls to \"$link\" are in \"$SPOOLDIR/_call/log\":
"
		egrep $node _call/log 2>/dev/null |
			sed 's/VCcall: //' |
			sort | tail
		echo
		;;
	esac

	calllog=_call/$node.log
	if [ -s $calllog ]
	then
		echo "Details for last call to \"$link\" are in \"$SPOOLDIR/$calllog\":
"
		tail $calllog
		echo
	else
		echo "No calls have been made to \"$link\"."
	fi

	case `egrep $node _call/*shell.log 2>/dev/null` in
	'')	;;
	*)	echo "Details of last calls from \"$link\" are in \"$SPOOLDIR/_call/*shell.log\":
"
		egrep $node _call/*shell.log 2>/dev/null |
			sed -e 's/report -- //' -e 's/^.*\.log://' |
			sort | tail
		echo
		;;
	esac

	echo "Reason for termination of last call is in
	\"$dir/$log\":
"
	tail -40 $dir/$log |
	egrep -v 'version|Elapsed time:|Usages:|Daemon:|Children:|Totals:|"-H9=' |
	tail -22
done
