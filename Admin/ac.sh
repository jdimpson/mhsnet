#!/bin/sh
#
#	@(#)ac.sh	1.13 94/12/15
#
#	Summarise a daemon's logs.
#

usage='"Usage: `basename $0` [-f<file>] [-k<kbytes>] [-m<mins>] [-r] <link> [log ...]
(Produces summary of calls from transport daemon logs.)"'

log=log
kbyts=0
mins=0
rate=false

skip=
for i
do
	case "$skip$i" in
	SKIP*)	skip=; shift				;;
	-f)	shift; log=$1; skip=SKIP		;;
	-f*)	log=`expr $i : '-f\(.*\)'`; shift	;;
	-k)	shift; kbyts=$1; skip=SKIP		;;
	-k*)	kbyts=`expr $i : '-k\(.*\)'`; shift	;;
	-m)	shift; mins=$1; skip=SKIP		;;
	-m*)	mins=`expr $i : '-m\(.*\)'`; shift	;;
	-r)	rate=true; shift			;;
	-*)	eval echo "$usage"; exit 1		;;
	*)	break					;;
	esac
done

case $# in
0)	eval echo "$usage"; exit 1	;;
1)	link=$1				;;
*)	link=$1; shift; log="$@"	;;
esac

SPOOLDIR=/usr/spool/MHSnet
eval `egrep 'SPOOLDIR' /usr/lib/MHSnetparams 2>/dev/null`
PATH=/bin:/usr/bin:/usr/ucb:${SPOOLDIR}/_bin:/usr/local:/usr/local/bin
export PATH

cd `netpath -l $link`

case "$log" in
-)	log=	;;
esac

cat $log |
awk '
BEGIN {
	esecs = 0
	et = ""
	ibyts = 0
	imsgs = 0
	mbyts ='$kbyts' * 1000
	msecs ='$mins' * 60
	msgin = ""
	msgout = ""
	obyts = 0
	omsgs = 0
	rate = "'$rate'"
	tibyts = 0
	timsgs = 0
	tobyts = 0
	tomsgs = 0
	ttime = 0
}
/daemon: .* STARTED$/ {
	if ( et != "" ) {
		printf "%s %s %s IN %s OUT %6s\n", date, time, msgin, msgout, et
		ttime += esecs
		timsgs += imsgs
		tomsgs += omsgs
		tibyts += ibyts
		tobyts += obyts
		et = ""
	}
	date = $1
	time = $2
	msgin = sprintf("%4s msgs %8s byts", 0, 0)
	msgout = sprintf("%4s msgs %8s byts", 0, 0)
	imsgs = 0
	omsgs = 0
	ibyts = 0
	obyts = 0
}
/^..reader:	.* messages, / {
	msgin = sprintf("%4s msgs %8s byts", $2, $4)
	imsgs = $2
	ibyts = $4
}
/^..writer:	.* messages, / {
	msgout = sprintf("%4s msgs %8s byts", $2, $4)
	omsgs = $2
	obyts = $4
}
/^Elapsed time: / {
	et = $3
	len = length(et)
	n1 = substr(et, 0, 3-7+len)
	n2 = substr(et, 5-7+len, 2)
	if ( et ~ /s$/ ) {
		esecs = int(n1) * 60
		esecs += int(n2)
	} else if ( et ~ /m$/ ) {
		esecs = int(n1) * 60 * 60
		esecs += int(n2) * 60
	} else if ( et ~ /h$/ ) {
		esecs = int(n1) * 24 * 60 * 60
		esecs += int(n2) * 60 * 60
	} else if ( et ~ /d$/ ) {
		esecs = int(et) * 24 * 60 * 60
	} else if ( et ~ /^0$/ ) {
		esecs = 0
	} else {
		print "UNRECOGNISED ELAPSED TIME: " et " IN SESSION STARTING: " date " " time
		exit
	}
	if ( esecs < msecs || ( obyts + ibyts ) < mbyts ) {
		et = ""
	}
}
END {
	if ( et != "" ) {
		printf "%s %s %s IN %s OUT %6s\n", date, time, msgin, msgout, et
		ttime += esecs
		timsgs += imsgs
		tomsgs += omsgs
		tibyts += ibyts
		tobyts += obyts
	}
	ttime /= 60*60;
	if ( ttime < 10.0 ) {
		printf "TOTAL             %4s msgs %8s byts IN %4s msgs %8s byts OUT %.3fh\n", timsgs, tibyts, tomsgs, tobyts, ttime
	} else if ( ttime < 100.0 ) {
		printf "TOTAL             %4s msgs %8s byts IN %4s msgs %8s byts OUT %.2fh\n", timsgs, tibyts, tomsgs, tobyts, ttime
	} else {
		printf "TOTAL             %4s msgs %8s byts IN %4s msgs %8s byts OUT %.1fh\n", timsgs, tibyts, tomsgs, tobyts, ttime
	}
	if ( rate ~ /^true$/ ) {
		ttime *= 60*60;
		timsgs /= ttime;
		tibyts /= ttime;
		tomsgs /= ttime;
		tobyts /= ttime;
		printf "rate/second      %.3f msgs %8.0f byts   %.3f msgs %8.0f byts\n", timsgs, tibyts, tomsgs, tobyts
	}
}
' |
sort

#		printf "GRAND TOTAL       %4s msgs %8s byts\n", timsgs+tomsgs, tibyts+tobyts
