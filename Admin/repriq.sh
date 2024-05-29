#!/bin/sh
#
#	change priorities of messages on queue based on `handler'.
#

usage='"Usage: `basename $0` -A <handler> -P <oldpri>:<newpri> linkname
(Messages for <handler> queued at priority <oldpri> are re-queued at <newpri>.)"'

handler=
oldpri=:

dir=
for i
do
	case "$i" in
	-A)	shift; handler=$1; shift		;;
	-A*)	handler=`expr "$i" : '-A\(.*\)'`; shift	;;
	-P)	shift; oldpri=$1; shift			;;
	-P*)	oldpri=`expr "$i" : '-P\(.*\)'`; shift	;;
	-*)	eval echo "$usage"; exit 1		;;
	*)	break					;;
	esac
done

case $# in
1)	link="$1"			;;
*)	eval echo "$usage"; exit 1	;;
esac

newpri=`expr $oldpri : '.*:\(.*\)'`
oldpri=`expr $oldpri : '\([^:]*\)'`

case "$handler/$oldpri/$newpri" in
/*/*|*//*|*/*/)
	eval echo "$usage"; exit 1	;;
esac

SPOOLDIR=/usr/spool/MHSnet
eval `egrep 'SPOOLDIR' /usr/lib/MHSnetparams 2>/dev/null`
PATH=/bin:/usr/bin:/usr/ucb:${SPOOLDIR}/_bin:/usr/local:/usr/local/bin
export PATH

cd $SPOOLDIR

netq -ahkmv $link | egrep '/cmds/|HANDLER' | awk '
/\/cmds\//	{
	file = $1
}
/HANDLER/	{
	handler = $2
	if ( handler !~ /'$handler'/ ) {
		next
	}
	n = split(file, a, "/")
	pri = substr(a[n], 1, 1)
	if ( pri != '$oldpri' )
		next
	tail = substr(a[n], 2)
	head = ""
	for ( i = 1 ; i < n ; i++ ) {
		head = head a[i] "/"
	}
	printf "mv %s %s%s%s\n", file, head, "'$newpri'", tail
}
' | sh -x
