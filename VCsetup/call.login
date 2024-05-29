#!/bin/sh

#	Setup permanent circuit to "link" via a direct RS-232 link with a "login".

USAGE="[-d<daemon>] [-f] link device speed login passwd"

PATH=$PATH:/etc:/usr/ucb:/usr/local:/usr/local/bin

argsdmn=	# -a<arg ...> - optional extra daemon args
andflag="-&"	# "-f" - run in foregound [else background]
daemon=		# -d<daemon> - alternate daemon name
trace=		# "-Tn"- trace level <n>

for i do
	case "${i}" in
	-a)	shift; argsdmn="$1";			shift	;;
	-a*)	argsdmn="`expr "${i}" : '-a\(.*\)'`";	shift	;;
	-d)	shift; daemon="$1";			shift	;;
	-d*)	daemon="`expr "${i}" : '-d\(.*\)'`";	shift	;;
	-f)	andflag=;				shift	;;
	-T[0-9])trace="$i";				shift	;;
	-*)	echo "Usage: $0 $USAGE";		exit 1	;;
	esac
done

case $# in
5)					;;
*)	echo "Usage: $0 $USAGE"; exit 1	;;
esac

link=$1
device=$2
speed=$3
login=$4
passwd=$5

case "${speed}" in
38400)	dmnargs="-dnD1024 -X3840"	;;
EXTB)	dmnargs="-dnD1024 -X3840"	;;
19200)	dmnargs="-dnD1024 -X1920"	;;
EXTA)	dmnargs="-dnD1024 -X1920"	;;
9600)	dmnargs="-dnD1024 -X960"	;;
4800)	dmnargs="-dnD512  -X480"	;;
2400)	dmnargs="-dnD256  -X240"	;;
1200)	dmnargs="-dnD128  -X120"	;;
600)	dmnargs="-dnD64   -X60"		;;
300)	dmnargs="-dnD32   -X30"		;;
*)	echo "$0: Unrecognised speed ${speed}"; sleep 60; exit 1;;
esac

case "$argsdmn" in
'')					;;
*)	dmnargs="$dmnargs $argsdmn"	;;
esac

case "$daemon" in
VCdaemon|'')	daemon=				;;
*)		daemon="-Ddmnname=$daemon"	;;
esac

SPOOLDIR=/usr/spool/MHSnet
eval `egrep 'SPOOLDIR' /usr/lib/MHSnetparams 2>/dev/null`

cd $SPOOLDIR
umask 027

case "$link" in
*=*)	node=`expr ${link} : '[^=]*=\([^\.]*\)'`	;;
*)	node=`expr ${link} : '\([^\.]*\)'`		;;
esac
log=_call/${node}.log
olog=_call/${node}.ol

if [ -s ${log} ]
then
	mv ${log} ${olog}
fi

exec _lib/VCcall ${andflag} -v $trace \
	${daemon} \
	-D "dmnargs=${dmnargs}" \
	-D "linespeed=${speed}" \
	-D "loginstr=${login}" \
	-D "passwdstr=${passwd}" \
	-D "ttydevice=${device}" \
	-S login.cs \
	-X ${log} \
	${link}
