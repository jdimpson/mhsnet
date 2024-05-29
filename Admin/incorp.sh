#!/bin/sh
#
#	@(#)incorp.sh	1.12 93/02/17
#
#	Incorporate pre-existing state messages, eg:-
#		incorp basser.cs.su.oz.au
#
#	Warning: missing arg defaults to ALL!
#
#	(First collect the args, as the `eval' below destroys them.)

Usage="[-<netstate flags>] [address ...]
Missing address defaults to all!"

args=
opts=
nextisopt=
case $# in
0)	args=;;
*)	for i
	do
		case "$nextisopt" in
		true)	case "$opts" in
			'')	opts="$i"	;;
			*)	opts="$opts $i"	;;
			esac
			nextisopt=
			continue
			;;
		esac

		case "$i" in
		'*')	exit 0
			;;
		-\?)	echo "Usage: $0 $Usage"
			exit 1
			;;
		-[a-z])	case "$opts" in
			'')	opts="$i"	;;
			*)	opts="$opts $i"	;;
			esac
			;;
		-[A-Z])	case "$opts" in
			'')	opts="$i"	;;
			*)	opts="$opts $i"	;;
			esac
			nextisopt=true
			;;
		-[A-Z]*)case "$opts" in
			'')	opts="$i"	;;
			*)	opts="$opts $i"	;;
			esac
			;;
		*)	case "$args" in
			'')	args="$i"	;;
			*)	args="$args $i"	;;
			esac
			;;
		esac
	done
	;;
esac

SPOOLDIR=/usr/spool/MHSnet
eval `egrep 'SPOOLDIR' /usr/lib/MHSnetparams 2>/dev/null`
PATH=$PATH:$SPOOLDIR/_bin

set -- $args

cd ${SPOOLDIR}/_state

case $# in
0)
	if [ ! -d MSGS ]; then echo "No state messages"; exit 1; fi
	cd MSGS
	for i in `find 0* -type f -print`
	do
		addr=`netpath ${i}`
		echo "netstate -isC ${opts} -F ${addr} <${i}"
		netstate -isC ${opts} -F ${addr} <${i}
	done
	echo "netstate -rsxC ${opts}"
	exec netstate -rsxC ${opts}
	;;
1)
	case "$opts" in
	'')	opts="-r"	;;
	*)	opts="$opts -r"	;;
	esac
	;;
esac

for i
do
	if [ -f ${i} ]
	then
		addr=`netpath ${i} MSGS`
		echo "netstate -isC ${opts} -F ${addr} <${i}"
		netstate -isC ${opts} -F ${addr} <${i}
	else
		file=`netpath -s ${i} MSGS`
		if [ -f ${file} ]
		then
			addr=`netaddr -ti ${i}`
			echo "netstate -isC ${opts} -F ${addr} <${file}"
			netstate -isC ${opts} -F ${addr} <${file}
		else
			echo "${i}: not found"
		fi
	fi
done

case $# in
1)
	;;
*)
	echo "netstate -rsxC ${opts}"
	exec netstate -rsxC ${opts}
	;;
esac
