#!/bin/sh
#
#	@(#)netrq.sh	1.8 91/04/10
#
#	Find out details of messages for/from us at destination.
#
#	(First collect the args, as the `eval' below destroys them.)
count=$#
case $count in
0)	args=;;
*)	args=$1
	shift
	for i
	do
		args="$args $i"
	done
	;;
esac

Usage="[-a] [-q] site ..."
usage="
Details for \`-a' will be returned by mail to the user \`\`network''.
(Use \`\`netforward -Amail -Unetwork <user>@<address>''
to forward the mail if necessary.)"

SPOOLDIR=/usr/spool/MHSnet
eval `egrep 'SPOOLDIR' /usr/lib/MHSnetparams 2>/dev/null`
cd $SPOOLDIR

privs=`_bin/netprivs`

all=0
dest=
qid=
case $count in
0)	echo "Usage: $0 ${Usage}"
	case "$privs" in *SU*) echo "${usage}" ;; esac
	exit 1 ;;
*)	for i in $args;
	do
		case "$i" in
		-a)	all=1		;;
		-q)	qid="q"		;;
		-*|'')	echo "Usage: $0 ${Usage}"
			case "$privs" in *SU*) echo "${usage}" ;; esac
			exit 1		;;
		*)	case "$dest" in
			'')	dest="$i"	;;
			*)	dest="$dest,$i"	;;
			esac		;;
		esac
	done ;;
esac

case "$dest" in
'')	echo "Usage: $0 ${Usage}"
	;;
esac

case "$privs$all" in
*SU*1)	echo "network" ;;
*)	expr "$privs" : '\(.*\):.*' ;;
esac | _bin/netmsg -Aqlooker -${qid}dP1 -D $dest
