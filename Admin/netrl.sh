#!/bin/sh
#
#	@(#)netrl.sh	1.8 91/04/10
#
#	Find out details of links at destination.
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

Usage="[-q] [-r|-v] site ..."
usage="
Details for \`-v' will be returned by mail to the user \`\`network''.
(Use \`\`netforward -Amail -Unetwork <user>@<address>''
to forward the mail if necessary.)"

SPOOLDIR=/usr/spool/MHSnet
eval `egrep 'SPOOLDIR' /usr/lib/MHSnetparams 2>/dev/null`
cd $SPOOLDIR

privs=`_bin/netprivs`

dest=
env=
qid=
case $count in
0)	echo "Usage: $0 ${Usage}"
	case "$privs" in *SU*) echo "${usage}" ;; esac
	exit 1
	;;
*)	for i in $args;
	do
		case "$i" in
		-q)	qid="q"		;;
		-r)	env=-E-r	;;
		-v)	env=-E-v	;;
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

case "$privs$env" in
*SU*-E-v)	echo "network" ;;
*)		env=; expr "$privs" : '\(.*\):.*' ;;
esac | _bin/netmsg -Allooker -${qid}dP1 $env -D $dest
