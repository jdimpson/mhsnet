#!/bin/sh
#
#	Remove news messages.
#
#	@(#)rmnews.sh	1.1 93/02/15
#

usage='"Usage: `basename $0` command_file ...
(Messages for handler \"reporter\" are removed.)"'

for i
do
	case "$i" in
	-*)	eval echo "$usage"; exit 1		;;
	*)	break					;;
	esac
done

case $# in
0)	eval echo "$usage"; exit 1	;;
esac

SPOOLDIR=/usr/spool/MHSnet
eval `egrep 'SPOOLDIR' /usr/lib/MHSnetparams 2>/dev/null`
PATH=/bin:/usr/bin:/usr/ucb:${SPOOLDIR}/_bin:/usr/local:/usr/local/bin
export PATH

for i
do
	HANDLER=
	ERROR=

	eval `nethdr -a $i` || exit 1

	case "$ERROR" in
	*"Header LENGTH error"*)
		echo "$i: not a command file"
		exit 1
		;;
	*"Can't open file"*)
		echo "$i: can't open data file"
		exit 1
		;;
	esac

	case "$HANDLER" in
	reporter)
		rm $i
		;;
	esac
done
