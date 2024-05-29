#!/bin/sh
#
#	This example script, if installed as `endprog' in `_call',
#	or in a transport daemon's operating directory, will be
#	executed by the selected daemon(s) on exit, with `stdin'
#	& `stdout' pointing to the virtual circuit.
#	It is passed three arguments:
#		site address, termination reason, daemon name.

trap '' 1 2

PATH=/bin:/usr/bin
export PATH
SPOOLDIR=/usr/spool/MHSnet
eval `egrep 'SPOOLDIR' /usr/lib/MHSnetparams 2>/dev/null`

cd $SPOOLDIR

line=`tty`
export line

(
	sleep 2
	echo `date +'%y/%m/%d %T'` $line : $3 : $2 : $1 >>_call/endprog.log 2>&1
) >/dev/null 2>&1 &

#	(The `&' allows the daemon to terminate first.)
