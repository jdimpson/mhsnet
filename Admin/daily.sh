#!/bin/sh
#
#	@(#)daily.sh	1.30 96/02/12
#
PATH=/bin:/usr/bin:/usr/ucb:/usr/local:/usr/local/bin
export PATH
SPOOLDIR=/usr/spool/MHSnet
eval `egrep 'SPOOLDIR' /usr/lib/MHSnetparams 2>/dev/null`
#
cd $SPOOLDIR
#
#	Backup the statefile for use in crashes:
#
echo "	Backup the statefile."
if _bin/netstate -sC
then
	cp _state/statefile _state/statefile.back
	cp _state/commandsfile _state/commands.back
else
	eval `_bin/netparams -w NCC_ADMIN`
	site=`_bin/netaddr`
	_bin/netmail -mort \
		LOGNAME=postmaster \
		NAME="MHSnet daily script" \
		SUBJECT="Network statefile corrupted." \
		"Network Admin <$NCC_ADMIN>" <<!
Please check "$SPOOLDIR/_state/statefile" with:
	$SPOOLDIR/_bin/netmap -vi $site

If that doesn't show any problem, read the error files in
"$SPOOLDIR/_state/NOTES/*", and then check
"$SPOOLDIR/_state/commandsfile" with:
	$SPOOLDIR/_bin/netstate -sC
and if that shows an error, fix it.

Otherwise if the statefile is badly corrupted,
you may attempt to recover it with:
	cd $SPOOLDIR/_state
	cp statefile.back statefile
	$SPOOLDIR/_bin/netstate -sC

If that doesn't fix the problem use:
	$SPOOLDIR/_bin/netstate -esC
to remove the errors, and then use:
	$SPOOLDIR/_bin/netrequest -a <link>
to fetch all data from one of the links to your site.

Finally, incorporate all the state messages that may have failed
while the statefile or commandsfile was unusable:
	cd $SPOOLDIR/_state/NOTES
	$SPOOLDIR/_bin/netincorp *

Mail generated by network program "$0" [$$]
!
fi
#
#	Backup other admin. files for use in crashes:
#
test -s _call/passwd && cp _call/passwd _call/passwd.back
test -s _lib/handlers && cp _lib/handlers _lib/handlers.back
test -s _lib/initfile && cp _lib/initfile _lib/initfile.back
test -s _lib/privsfile && cp _lib/privsfile _lib/privsfile.back
#
#	Backup all _lib/*.sh files for use in crashes:
#
cd _lib
	for i in *.sh
	do
		cp $i ${i}.back
	done
cd ..
#
#	Curtail routers:
#
_bin/netcontrol -f curtail router
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
	trap 1 2 15
cd ..
#
#	Clean out junk and timed-out files:
#
echo "	Remove timed-out messages."
_bin/netpurge -dfr
#
#	Clean up broadcast messages data-base:
#
echo "	Compact messages data-base."
_bin/netcheckdb
#
#	Restart router:
#
_bin/netcontrol -f start router
sleep 10
#
#	Mark any dead links:
#
echo "	Mark any dead links."
_bin/netchange
#
#	Remove old state messages and directories
#	(could make these age faster than a year):
#
(
	cd _state/MSGS
	find * -type f -mtime +365 \
		-exec ls -l '{}' ';' \
		-exec rm -f '{}' ';'
	rmdir `find * -type d -print | sort -r` >/dev/null 2>&1
) >>_state/removed.MSGS
chmod 640 _state/removed.MSGS
#
#	Remove old state error files:
#
if [ -d _state/NOTES ]
then
	echo "	Remove old state comments."
	find _state/NOTES -mtime +7 -exec ls -l '{}' ';' -exec rm -f '{}' ';'
fi
#
#	Finished.
#
echo "	 `date +%T` Daily administration commands finished."
