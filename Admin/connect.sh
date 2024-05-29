#!/bin/sh
#
#	@(#)connect.sh	1.15 94/12/15
#
#	Summarise "_stats/connect".
#

usage='"Usage: `basename $0` [-a] [-c <$/hr>] [-d] [-f <file>] [-h] \\
 [-i <link>] [-k <kbytes>] [-m <mins>] [-r] [-s] [-v] [link]
(Produces summary of calls from transport daemon \"connect\" logs.)"'

TMP=/tmp/cnct.awk.$$

SPOOLDIR=/usr/spool/MHSnet
eval `egrep 'SPOOLDIR' /usr/lib/MHSnetparams 2>/dev/null`

all=false
connect=$SPOOLDIR/_stats/connect
cost=0
debug=false
down=false
head=false
ignore=
kbyts=0
mins=0
rate=false
strip=false
roundup=false
verbose=false

skip=
for i
do
	case "$skip$i" in
	SKIP*)	skip=; shift				;;
	-D)	debug=true; shift			;;
	-a)	all=true; shift				;;
	-c)	shift; cost=$1; skip=SKIP		;;
	-c*)	cost=`expr $i : '-c\(.*\)'`; shift	;;
	-d)	down=true; shift			;;
	-f)	shift; connect=$1; skip=SKIP		;;
	-f*)	connect=`expr $i : '-f\(.*\)'`; shift	;;
	-h)	head=true; shift			;;
	-i)	shift; ignorelink=$1; skip=SKIP		;;
	-i*)	ignorelink=`expr $i : '-i\(.*\)'`; shift;;
	-k)	shift; kbyts=$1; skip=SKIP		;;
	-k*)	kbyts=`expr $i : '-k\(.*\)'`; shift	;;
	-m)	shift; mins=$1; skip=SKIP		;;
	-m*)	mins=`expr $i : '-m\(.*\)'`; shift	;;
	-r)	rate=true; shift			;;
	-s)	strip=true; shift			;;
	-u)	roundup=true; shift			;;
	-v)	verbose=true; shift			;;
	-*)	eval echo "$usage"; exit 1		;;
	*)	break					;;
	esac

	case "$ignorelink" in
	'')	;;
	*)
		case "$strip" in
		true)	ignorelink=`$SPOOLDIR/_bin/netaddr $ignorelink`
			;;
		false)	ignorelink=`$SPOOLDIR/_bin/netaddr -ti $ignorelink`
			;;
		esac
		case "$ignorelink" in
		'')	;;
		*)	case "$ignore" in
			'')	ignore="$ignorelink"		;;
			*)	ignore="$ignore|$ignorelink"	;;
			esac
			;;
		esac
		ignorelink=
	esac
done

case $# in
0)	link=.
	;;
1)	case "$strip" in
	true)	link=`$SPOOLDIR/_bin/netaddr $1`
		;;
	false)	link=`$SPOOLDIR/_bin/netaddr -ti $1`
		;;
	esac
	;;
*)	eval echo "$usage"
	exit 1
	;;
esac

case "$ignore" in
'')	ignore='9=9=9=9'	;;
esac

case "$connect" in
'')	eval echo "$usage"; exit 1	;;
-)					;;
*)	if [ ! -f $connect ]
	then
		echo "Connect statistics turned off"
		exit 0
	fi
	;;
esac

cat >$TMP <<!
BEGIN		{
	all = "$all"
	cost = "$cost"
	debug = "$debug"
	down = "$down"
	link = "$link"
	mbyts = $kbyts * 1000
	msecs = $mins * 60
	rate = "$rate"
	roundup = "$roundup"
	verbose = "$verbose"
	debugfmt = "\t%4d: %s MISSING \"%s\" record for link %s\n"
	imsgs = 1
	idata = 2
	omsgs = 3
	odata = 4
	last = 5
	starttime = 0
	if ( "$head" ~ /^true$/ && verbose ~ /^true$/ ) {
		printf "%-17s ", "   Start Date"
		if ( link ~ /^\.$/ )
			printf "%-32s ", " Link"
		printf "%10s ", "Messages"
		if ( all ~ /^true$/ ) {
			printf "%10s ", "IN  Bytes"
			printf "%10s %10s", "Messages", "OUT  Bytes"
		} else
			printf "%10s", "Bytes"
		if ( down ~ /^true$/ )
			printf " %7s", " Down"
		printf " %7s %7s", "Hours", "Time%"
		if ( rate ~ /^true$/ )
			printf " %6s", "Rate"
		if ( cost > 0 )
			printf " %8s", "Cost"
		printf "\n"
	}
}
/$ignore/	{
	next
}
/ i&o $link/	{
	if ( starttime == 0 && \$9 ~ /^[0-9]/ ) {
		starttime = \$9
	}
	if ( \$6 < msecs || \$8 < mbyts )
		next
	secs = \$6
	finish = \$9 + secs
	if ( roundup ~ /^true$/ )
		secs += 60 - (secs%60)
	if ( lasttime < finish )
		lasttime = finish
	a[\$5 elps] += secs
	a[\$5 omsgs] += \$7
	a[\$5 odata] += \$8
	if ( verbose ~ /^true$/ ) {
		printf "%s %s", \$1, \$2
		if ( link ~ /^\.$/ )
			printf " %-32s", \$5
		if ( all ~ /^true$/ )
			printf " %10.0f %10.0f", 0, 0
		printf " %10.0f %10.0f", \$7, \$8
		if ( down ~ /^true$/ ) {
			downsecs = \$9 - a[\$5 last]
			if ( downsecs < \$9 )
				printf " %7.2f", (downsecs+(60*60)/200)/(60*60)
			else
				printf "        "
		}
		printf " %7.2f", (secs+(60*60)/200)/(60*60)
		if ( lasttime > starttime && starttime > 0 )
			printf " %7.1f", a[\$5 elps] * 100 / (lasttime-starttime)
		else
			printf "     ?  "
		if ( rate ~ /^true$/ )
			printf " %6.0f", (\$8 + secs/2) / secs
		if ( cost > 0 )
			printf " %8.2f", cost * (secs+(60*60)/200)/(60*60)
		printf "\n"
	}
	a[\$5 last] = finish
	n[\$5]++
	next
}
/ in +$link/	{
	if ( starttime == 0 && \$9 ~ /^[0-9]/ ) {
		starttime = \$9
	}
	secs = \$6
	finish = \$9 + secs
	if ( lasttime < finish )
		lasttime = finish
	if ( tn != "" && debug ~ /^true$/ )
		printf debugfmt, NR, td, "out", tn
	td = \$1 " " \$2
	tn = \$5
	timsgs = \$7
	tidata = \$8
	next
}
/ out $link/	{
	if ( starttime == 0 && \$9 ~ /^[0-9]/ ) {
		starttime = \$9
	}
	if ( tn != \$5 ) {
		if ( debug ~ /^true$/ ) {
			printf debugfmt, NR, \$1, \$2, "in", \$5
			if ( tn != "" )
				printf debugfmt, NR, td, "out", tn
		}
		next
	}
	tn = ""
	tomsgs = \$7
	todata = \$8
	if ( \$6 < msecs || (tidata+todata) < mbyts )
		next
	secs = \$6
	finish = \$9 + secs
	if ( roundup ~ /^true$/ )
		secs += 60 - (secs%60)
	if ( lasttime < finish )
		lasttime = finish
	a[\$5 elps] += secs
	a[\$5 imsgs] += timsgs
	a[\$5 idata] += tidata
	a[\$5 omsgs] += tomsgs
	a[\$5 odata] += todata
	if ( verbose ~ /^true$/ ) {
		printf "%s", td
		if ( link ~ /^\.$/ )
			printf " %-32s", \$5
		if ( all ~ /^true$/ ) {
			printf " %10.0f %10.0f", timsgs, tidata
			printf " %10.0f %10.0f", tomsgs, todata
		} else
			printf " %10.0f %10.0f", timsgs+tomsgs, tidata+todata
		if ( down ~ /^true$/ ) {
			downsecs = \$9 - a[\$5 last]
			if ( downsecs < \$9 )
				printf " %7.2f", (downsecs+(60*60)/200)/(60*60)
			else
				printf "        "
		}
		printf " %7.2f", (secs+(60*60)/200)/(60*60)
		if ( lasttime > starttime && starttime > 0 )
			printf " %7.1f", a[\$5 elps] * 100 / (lasttime-starttime)
		else
			printf "     ?  "
		if ( rate ~ /^true$/ )
			printf " %6.0f", (todata+tidata+secs/2) / secs
		if ( cost > 0 )
			printf " %8.2f", cost * (secs+(60*60)/200)/(60*60)
		printf "\n"
	}
	a[\$5 last] = finish
	n[\$5]++
	next
}
END		{
	if ( starttime == 0 )
		exit
	if ( verbose ~ /^true$/ ) {
		m = 32
	} else {
		for ( i in n )
			if ( (x = length(i)) > m )
				m = x
	}

	if ( "$head" ~ /^true$/ && verbose !~ /^true$/ ) {
		if ( link ~ /^\.$/ ) {
			s = " Link"
			printf "%s ", s
			if ( (x = length(s)) < m )
				while ( ++x <= m )
					printf " "
		}
		printf "%10s ", "Messages"
		if ( all ~ /^true$/ ) {
			printf "%10s ", "IN  Bytes"
			printf "%10s %10s", "Messages", "OUT  Bytes"
		} else
			printf "%10s", "Bytes"
		if ( down ~ /^true$/ )
			printf " %7s", " Down"
		printf " %7s %7s", "Hours", "Time%"
		if ( rate ~ /^true$/ )
			printf " %6s", "Rate"
		if ( cost > 0 )
			printf " %8s", "Cost"
		printf "\n"
	}

	for ( i in n ) {
		im = a[i imsgs]
		om = a[i omsgs]
		id = a[i idata]
		od = a[i odata]
		el = a[i elps]

		totimsgs += im
		totomsgs += om
		totidata += id
		totodata += od
		totelps  += el

		if ( verbose ~ /^true$/ )
			printf "%-17s ", "~ TOTAL"
		if ( link ~ /^\.$/ ) {
			printf "%s ", i
			if ( (x = length(i)) < m )
				while ( ++x <= m )
					printf " "
		}
		if ( all ~ /^true$/ ) {
			printf "%10.0f %10.0f ", im, id
			printf "%10.0f %10.0f", om, od
		} else
			printf "%10.0f %10.0f", im+om, id+od
		if ( down ~ /^true$/ )
			printf " %7.2f", ((lasttime-starttime) - el + (60*60)/200) / (60*60)
		printf " %7.2f", (el + (60*60)/200) / (60*60)
		if ( lasttime > starttime && starttime > 0 )
			printf " %7.1f", el * 100 / (lasttime-starttime)
		else
			printf "     ?  "
		if ( rate ~ /^true$/ )
			printf " %6.0f", (id + od + (el/2)) / el
		if ( cost > 0 )
			printf " %8.2f", cost * (el + (60*60)/200) / (60*60)
		printf "\n"
	}
	if ( link !~ /^\.$/ )
		exit
	if ( verbose ~ /^true$/ ) {
		printf "%-17s ", "~ _GRAND_TOTAL"
		s = ""
	}
	else
		s = "~ TOTAL"
	printf "%s ", s
	if ( (x = length(s)) < m )
		while ( ++x <= m )
			printf " "
	if ( all ~ /^true$/ ) {
		printf "%10.0f %10.0f ", totimsgs, totidata
		printf "%10.0f %10.0f", totomsgs, totodata
	} else
		printf "%10.0f %10.0f", totimsgs+totomsgs, totidata+totodata
	printf " %7.2f", (totelps + (60*60)/200) / (60*60)
	printf "        "
	if ( rate ~ /^true$/ )
		printf " %6.0f", (totidata + totodata + (totelps/2)) / totelps
	if ( cost > 0 )
		printf " %8.2f", cost * (totelps + (60*60)/200) / (60*60)
	printf "\n"
}
!

case "$verbose/$link" in
*/.)	sort=sort	;;
true/*)	sort=sort	;;
*)	sort=cat	;;
esac

case "$connect:$strip" in
-:false)
	sort -t' ' +0 -3 +4 -5 +3
	;;
*:false)
	sort -t' ' +0 -3 +4 -5 +3 $connect
	;;
-:true)
	sort -t' ' +0 -3 +4 -5 +3 | sed 's/[0-9]=//g'
	;;
*:true)
	sort -t' ' +0 -3 +4 -5 +3 $connect | sed 's/[0-9]=//g'
	;;
esac | awk -f $TMP | $sort

rm -f $TMP
