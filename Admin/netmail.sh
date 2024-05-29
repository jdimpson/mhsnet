#!/bin/sh
#
#	Shell program to use local `mail' for reading,
#	and `netmail' for sending.
#
#	@(#)netmail.sh	1.9 96/02/12
#
#	(First collect the args, as the `eval' below destroys them.)
count=$#
case $count in
0)	args=;;
*)	args=$1
	shift
	for i
	do
		case "$i" in
		'*')	exit 0		;;
		esac
		case "$args" in
		'')	args="$i"	;;
		*)	args="$args $i"	;;
		esac
	done
	;;
esac

SPOOLDIR=/usr/spool/MHSnet
eval `egrep 'SPOOLDIR' /usr/lib/MHSnetparams 2>/dev/null`

MAILSENDER=${SPOOLDIR}/_bin/netmail
MAILREADER=mail		# Assume it can take `-f mbox' args.

#case $USER in '') ;; *) MAILBOX=$USER ;; esac
#case $LNAME in '') ;; *) MAILBOX=$LNAME ;; esac
#case $LOGNAME in '') ;; *) MAILBOX=$LOGNAME ;; esac
#MAILBOX=/usr/mail/$MAILBOX

set -- $args

case $# in
	0)	case $MAILBOX in
			'')	exec $MAILREADER ;;
		esac
		exec $MAILREADER -f $MAILBOX ;;
esac

case $1 in -f) exec $MAILREADER "$@" ;; esac

exec $MAILSENDER "$@"
