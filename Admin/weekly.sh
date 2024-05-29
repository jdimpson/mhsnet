#!/bin/sh
#
#	@(#)weekly.sh	1.11 94/05/30
#
PATH=/bin:/usr/bin:/etc:/etc/bin:/usr/ucb:/usr/local:/usr/local/bin
export PATH
SPOOLDIR=/usr/spool/MHSnet
eval `egrep 'SPOOLDIR' /usr/lib/MHSnetparams 2>/dev/null`
#
cd $SPOOLDIR
#
#	Turn over (active) log files:
#
echo "	Turn over log files."
for i in `find . \( -name log -o -name '*.log' \) -type f -print`
do
	case "$i" in
	*.log)
		part=`expr $i : '\(.*\)\.log'`
		ol=$part.ol
		ool=$part.ool
		;;
	*)
		dir=`dirname $i`
		ol=$dir/oldlog
		ool=$dir/olderlog
		;;
	esac
	if [ -s $ol ]
	then
		mv $ol $ool
	fi
	mv $i $ol
done
connect=_stats/connect
if [ -s $connect ]
then
	mv $connect $connect.`date '+%y%m%d'`
	>$connect
fi
#
#	Curtail routers:
#
_bin/netcontrol -f curtail router
#
#	Tell routers to terminate on queue empty
#	and wait for them to terminate:
#
locks=`ls _route/lock _route/*[0-9].lock 2>/dev/null|sed 's/.*/-s & -o/'`
while [ $locks -n '' ]; do >_route/STOP; sleep 10; done
rm -f _route/STOP
#
#	Start new logfiles:
#
_bin/netcontrol -f newlog
sleep 10
#
#	Restart router:
#
_bin/netcontrol -f start router
#
#	Finished.
#
echo "	 `date +%T` Weekly administration commands finished."
