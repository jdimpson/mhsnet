#!/bin/sh
#
#	Nightly administration (not in "daily").
#
#	Run from "initfile" as in:
#		admin-night "15 1 * * *" _lib/nightly
#
#
PATH=/bin:/usr/bin:/usr/ucb:/usr/local:/usr/local/bin
export PATH
SPOOLDIR=/usr/spool/Sun4
eval `egrep 'SPOOLDIR' /usr/lib/MHSnetparams 2>/dev/null`
tmp=/tmp/nightly.$$
#
cd $SPOOLDIR
#
#	Check changes in statefile, and update "nodes".
#
_bin/netmap -l >_state/newnodes
diff _state/nodes _state/newnodes |
sed -e '/^[-0-9]/d' -e '/^< /s//Old: /' -e '/^> /s//New: /' >$tmp

if [ -s $tmp ]
then
	eval `_bin/netparams -w NCC_ADMIN`
	_bin/netmail -mort \
		LOGNAME=postmaster \
		NAME="MHSnet nightly script" \
		SUBJECT="New/Old nodes in statefile" \
		"Network Admin <$NCC_ADMIN>" <$tmp
fi
mv _state/newnodes _state/nodes
_bin/netsort <_state/nodes >_state/regions
rm -f $tmp
#
#	Finished.
#
echo "	 `date +%T` Nightly administration commands finished."
