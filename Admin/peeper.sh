#!/bin/sh
trap '' 15
trap 'kill -15 0; exit 1' 14
#
#	Shell for remote host information using `handler' as `peeper'.
#
#	SCCSID @(#)peeper.sh	1.9 96/02/12
#
#	Invoked in SPOOLDIR with optional args:
#		-r			(message is being returned)
#		-A<ID>			(message is an acknowledgement, with original ID)
#		-D<site;[site;...]>	(message may have been duplicated at each site)
#		-E<reason>		(error message from header)
#		-F<flag;[flag;...]>	(handler flags inserted at source)
#		-W<site;[site;...]>	(message was forwarded by each site)
#	and these:
#		<home name>
#		<typed home name>
#		<source address>
#		<typed source address>
#		<message ID>
#		<data size>

# Process these options
#	-r	message has been returned due to failure of some kind
#	-E	the error generated at the remote site
#	-F	the user calling the service, and the host name desired

PATH=/bin:/usr/bin:/usr/ucb:/usr/local:/usr/local/bin
export PATH

returned=
error=
for i
do
	case "$i" in
	-r)	returned=true;				shift ;;
	-E*)	error=`echo \"$i\" | sed '1s/-E//'`;	shift ;;
	-F*)	requester=`expr "$i;" : '-F\([^;]*\);.*'`
		     host=`expr "$i;" : '-F[^;]*;\([^;]*\);.*'`
		    qtype=`expr "$i;" : '-F[^;]*;[^;]*;\([^;]*\);.*'`
		dnsserver=`expr "$i;" : '-F[^;]*;[^;]*;[^;]*;@\([^;]*\);.*'`
		shift
		;;
	-*)						shift ;;
	*)						break ;;
	esac
done

# Now interpret arguments

home=$1
source=$3

# Handle returned messages gracefully

case "$returned" in
true)
	data=`echo "$error"|sed 1q`
	requester=`expr "$data" : '\([^;]*\);.*'`
	host=`expr "$data" : '[^;]*;\(.*\)'`
	error=`echo "$error"|sed 1d`
	echo "`date '+%y/%m/%d %H:%M'` $requester@$home $host RETURNED@$source" >> _stats/peeper.log
	echo "Your request was:

	dig		<$host>
	dig_server	<$source>
	requester	<$requester@$home>

Below is the output of the dig command:

$error" \
	| _bin/netmail -mort \
		LOGNAME=postmaster \
		NAME="MHSnet DNS Gateway" \
		SUBJECT="Results of your DNS request at $source" \
		"$requester"
	exit 0
esac

case "$requester" in
"")	echo "`date '+%y/%m/%d %H:%M'` host=$host user=<> source=$source ret=$returned err=$error ERROR" >> _stats/peeper.log
	echo "no user!" 1>&2
	exit 100	;;
esac

case "$qtype" in
"")	qtype="mx";;
esac

# Process the request

(
	echo "Your request was:

	dig		<$host>
	querytype	<$qtype>
	dig_server	<$home>
	DNS_server	<${dnsserver:-$home}>
	requester	<$requester@$source>

Below is the output of the dig command:
"
	dig ${dnsserver:+@$dnsserver} $host $qtype 2>&1
) \
	| _bin/netmail -mort \
		LOGNAME=postmaster \
		NAME="MHSnet DNS Gateway" \
		SUBJECT="Results of your DNS request at ${dnsserver:-$home}" \
		$requester@$source

echo "`date '+%y/%m/%d %H:%M'` $requester@$source $host" >> _stats/peeper.log
exit 0
