#!/bin/sh
#
#	Read messages with headers and queue for router.
#
#	@(#)qmsgs.sh	1.1 93/02/15
#

usage='"Usage: `basename $0` message ...
(Read messages with headers and queue for router.)"'

case $# in
0)	eval echo "$usage"; exit 1	;;
esac

SPOOLDIR=/usr/spool/MHSnet
eval `egrep 'SPOOLDIR' /usr/lib/MHSnetparams 2>/dev/null`
PATH=/bin:/usr/bin:/usr/ucb:${SPOOLDIR}/_bin:/usr/local:/usr/local/bin
export PATH

for i
do
	case "$i" in
	-*)	eval echo "$usage"; exit 1	;;
	esac

	ERROR=

	eval `nethdr -a $i` || exit 1

	case "$ERROR" in
	*'Header LENGTH error'*)
		echo "$i: not a message"
		exit 1
		;;
	esac

	nethdr -s $i >/dev/null || continue

	nethdr -r $i || exit 1
done
