#!/bin/sh
#
#	This example script, if installed as `shellprog' in `_call',
#	or in a transport daemon's operating directory, will be
#	executed by the virtual circuit shell on startup,
#	with `stdin' & `stdout' pointing to the virtual circuit.
#	It is passed three arguments:
#		site address, daemon name, shell name.

PATH=/bin:/usr/bin
export PATH

case "$3" in
tty*shell)
	;;
*)	exit 0	# ignore non-tty shells
	;;
esac

SPOOLDIR=/usr/spool/MHSnet
eval `egrep 'SPOOLDIR' /usr/lib/MHSnetparams 2>/dev/null`

cd $SPOOLDIR

site="$1"
daemon=`basename $2`
shell="$3"

tty=`tty`
tty=`basename $tty`
date=`date +'%y/%m/%d %H:%M'`
mesg='ACCESS ERROR -- illegal tty port'

case "$tty" in
illegal_tty)
	echo "$mesg"		# to remote caller
	echo "$mesg" 1>&2	# to stderr for daemon log
	sleep 2
	echo $date $tty : "DENIED " : $daemon $shell $site >>$0.log
	exit 1
	;;
esac

echo $date $tty : ALLOWED : $daemon $shell $site >>$0.log
exit 0
