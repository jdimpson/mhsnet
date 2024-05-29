#!/bin/sh
#
#	@(#)monthly.sh	1.7 90/07/31
#
PATH=/bin:/usr/bin:/usr/ucb:/usr/local:/usr/local/bin
export PATH
SPOOLDIR=/usr/spool/MHSnet
eval `egrep 'SPOOLDIR' /usr/lib/MHSnetparams 2>/dev/null`
#
cd $SPOOLDIR
#
#	Turn over the statistics:
#
_stats/turn-acct
#
#	Finished.
#
echo "	 `date +%T` Monthly administration commands finished."
