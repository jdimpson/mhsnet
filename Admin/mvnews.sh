#!/bin/sh
#
#	Move news commands to new directory.
#
#	@(#)mvnews.sh	1.1 93/02/15
#

usage='"Usage: `basename $0` -d<directory> command_file ...
(Messages for handler \"reporter\" are moved to new directory.)"'

dir=
for i
do
	case "$i" in
	-d)	shift; dir=$1; shift			;;
	-d*)	dir=`expr $i : '-d\(.*\)'`; shift	;;
	-*)	eval echo "$usage"; exit 1		;;
	*)	break					;;
	esac
done

case $# in
0)	eval echo "$usage"; exit 1	;;
esac

case "$dir" in
'')	eval echo "$usage"; exit 1	;;
esac

SPOOLDIR=/usr/spool/MHSnet
eval `egrep 'SPOOLDIR' /usr/lib/MHSnetparams 2>/dev/null`
PATH=/bin:/usr/bin:/usr/ucb:${SPOOLDIR}/_bin:/usr/local:/usr/local/bin
export PATH

if [ ! -d $dir ]
then
	echo "create $dir? \c"
	read ans
	case "$ans" in
	y|yes)	mkdir $dir || exit 1	;;
	*)	exit 1			;;
	esac
fi

for i
do
	case "$i" in
	[0-9]?????????????)	;;
	*)	continue	;;
	esac

	HANDLER=
	ERROR=

	eval `netcmds -a $i` || exit 1

	case "$ERROR" in
	*"Commands garbled"*)
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
		netcmds -m $i >$dir/$i || exit 1
		rm $i
		;;
	esac
done
