#!/bin/sh
PATH=/bin:/usr/bin export PATH

TMPLOG=/tmp/uucp_from
if [ ! -w $TMPLOG ]; then TMPLOG=${TMPLOG}$$; fi
export TMPLOG
date=`date '+%y/%m/%d %T'`

# Remap + to / for use in X.400 addressing - uux doesn't like /
dlist=''
for i
do
	case $i in
	*+*@*)	dlist="$dlist `echo $i | sed 's/+/\//g'`"		;;
	*+*)	dlist="$dlist `echo $i | sed 's/+/\//g'`@telememo.au"	;;
	*)	dlist="$dlist $i"					;;
	esac
done
export dlist
echo "
$date : rmail $dlist" >>$TMPLOG

# This is the function which does most of the work.
netfile_fwd(){
	dests=''
	for i in $dlist
	do
		dests="-Q $i $dests"
	done
	echo netfile -Amailer -NMail -R"$from" $dests - >>$TMPLOG
	/usr/spool/MHSnet/_bin/netfile -Amailer -NMail -R"$from" $dests - >>$TMPLOG 2>&1
}

SPOOLDIR=/usr/spool/MHSnet
eval `egrep 'SPOOLDIR' /usr/lib/MHSnetparams 2>/dev/null`

# The format of the first line of the message is:
#	From <fromuser> <date with 5 fields> remote from <uucp_host>

read waste1 from date1 date2 date3 date4 date5 waste2 waste3 nodename nodename2

UUCPLOG=/usr/spool/uucp/.Log/rmail/$nodename

echo $waste1 $from $date1 $date2 $date3 $date4 $date5 $waste2 $waste3 "***$nodename"'***' >>$TMPLOG
echo $date : $waste1 $from $date1 $date2 $date3 $date4 $date5 $waste2 $waste3 $nodename >>$UUCPLOG

# Adjust for possible presence of time zone in field "waste2"
case "$waste3" in
remote)	nodename=$nodename2
	;;
esac

case "$from" in
	# Handle name@site addressing
*@*)	export from
	netfile_fwd $dlist
	exit $?
	;;
	# Convert UUCP-style "site!name" address in From line
*!*)	name=`expr "$from" : '.*!\(.*\)'`
	system=`expr "$from" : '\(.*\)!.*'`
	from=$name@$system; export from
	netfile_fwd $dlist
	exit $?
	;;
*)	from="$from@$nodename.cs.su.oz.au"
	export from
	netfile_fwd $dlist
	exit $?
	;;
esac
