#!/bin/sh
#
#	@(#)windup.sh	1.25 96/02/12
#
#	Shell script to wind up network activities:
#	all background processes will be stopped,
#	and the `router' will terminate when its
#	message queue empties.
#
#	If invoked as `netpause' - just windup routers.
#
#	If invoked as `netresume' - restart routers.

PATH=/bin:/usr/bin:/etc:/etc/bin:/usr/ucb:/usr/local:/usr/local/bin
NCC_ADMIN=root
NETUSERNAME=daemon
SPOOLDIR=/usr/spool/MHSnet
eval `egrep 'NCC_ADMIN|NETUSERNAME|SPOOLDIR' /usr/lib/MHSnetparams 2>/dev/null`

cd $SPOOLDIR
if >_state/Q$$
then
	rm -f _state/Q$$
else
	echo "Must be \"root\" or \"$NETUSERNAME\" to execute $0."
	exit 1
fi

#
#	Check network is running.
#
if _bin/netcontrol -sL _lib
then
	case "$0" in
	*pause)
		actstr=pausing
		;;
	*resume)
		actstr=resuming
		;;
	*)
		actstr=terminating
		;;
	esac
	echo "MHSnet $actstr..."
else
	echo "MHSnet is inactive."
	exit 1
fi

case "$0" in
*pause)
	#
	#	Curtail router processes:
	#
	_bin/netcontrol -f curtail router
	;;
*resume)
	#
	#	Restart router processes:
	#
	_bin/netcontrol -f start router
	;;
*windup)
	#	Curtail router and admin. processes and stop links
	#	-- you may need to edit the `initfile' patterns
	#	to reflect local names:
	#
	for i in `_bin/netcontrol -f status|sed -e 's/^\([^ 	]*[-_]\).*/\1/' -e 's/[ 	].*//'|sort -u`
	do
		cmd=stop
		case "$i" in router*|admin*) cmd=curtail;; '') continue;; esac
		sleep 1
		_bin/netcontrol -f $cmd "^$i"
	done
	#
	#	Send mail about termination:
	#
	_bin/netmail -mort \
		SUBJECT="Network terminated." \
		"Network Admin <$NCC_ADMIN>" </dev/null
	;;
esac

case "$0" in
*resume)
	;;
*)
	#
	#	Tell routers to terminate on queue empty
	#	and wait for them to terminate:
	#
	cd _route

	trap 'rm -f STOP' 1 2 15
	count=100
	testlocks=`ls lock *[0-9].lock 2>/dev/null|sed 's/.*/-s & -o/'`

	rm -f STOP
	while [ '(' $testlocks -n '' ')' -a $count -gt 0 ]
	do
		>STOP
		sleep 1
		count=`expr $count - 1`
	done
	rm -f STOP

	cd ..
	;;
esac

case "$0" in
*windup)
	#
	#	Shutdown:
	#
	_bin/netcontrol -f shutdown
	;;
esac
