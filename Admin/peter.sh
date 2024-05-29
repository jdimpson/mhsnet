#!/bin/sh
trap '' 15
trap 'kill -15 0; exit 1' 14
#
#	@(#)peter.sh	1.4 96/02/12
#
#	Shell for remote name-server enquiry using `handler' as `peter'.
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
#

PATH=/bin:/usr/bin:/usr/ucb:/usr/local:/usr/local/bin
export PATH

err=
flags=
fwd=
returned=

for i
do
	case "$i" in
	-A*)	echo "Unexpected ACK" 1>&2; exit 1;	shift ;;
	-E*)	err="$err: `echo \"$i\"|sed '1s/-E//'`";shift ;;
	-F*)	flags="$i";				shift ;;
	-W*)	fwd="$fwd: `expr \"$i\" : '-W\(.*\)'`";	shift ;;
	-r)	returned=true;				shift ;;
	-*)						shift ;;
	*)						break ;;
	esac
done

WHOISARGS='-e'
WHOISFILE=/usr/pub/whois
WHOISPROG=/usr/bin/egrep

# If the above are correct, comment out these next commands...

eval `_bin/netparams -w peter WHOIS`
case "$WHOISARGS" in
*,*)	WHOISARGS=`echo $WHOISARGS|sed 's/,/ /g'`
esac

# TRACE -- enable for tests
# echo `date +'%y/%m/%d %H:%M'` " $@" >>_stats/peter.log

home="$1"
source="$3"

#	Old handler returned result as an error.

case "$returned" in
true)
	queryend="`expr index \"$err\" '
'`"
	query="`expr substr \"$err\" 1 $queryend`"
	result="`expr substr \"$err\" $queryend 1000`"
	case "$result" in
	'')	result="There was no match."	;;
	esac
	enquirer="`expr \"$query\" : '[^;]*;\(.*\)'`"
	query="`expr \"$query\" : '\([^;]*\)'`"
	# TRACE -- enable for tests
	echo `date +'%y/%m/%d %H:%M'` " $enquirer: $query@$source - $result" >>_stats/peter.log
	echo "Your query was: $query
The result was:
$result" | _bin/netmail -mort \
		LOGNAME=postmaster \
		NAME="Name Server" \
		SUBJECT="result of your query at $source" \
		$enquirer@$home
	exit 0
	;;
esac

#	Flags contain query.

enquirer=`expr "$flags" : '[^;]*;\([^;]*\)'`
query=`expr "$flags" : '-F\([^;]*\)'`

#	Check WHOISFILE exists, othewise try `netfinger'
finger=true
finger >/dev/null 2>&1 || finger=false

case "$WHOISFILE.$finger" in
'')	;;
*.true)	test -r $WHOISFILE || WHOISFILE=
	;;
esac

fingermsg=
case "$WHOISFILE.$finger" in
.true)	echo $enquirer "$query" | _bin/netmsg -Afinger -D$home -O$source
	exit 0
	;;
*.true)	echo $enquirer "$query" | _bin/netmsg -Afinger -D$home -O$source
	fingermsg="

(\`finger' result follows.)"
	;;
esac

# Lookup WHOISFILE
result=`$WHOISPROG $WHOISARGS "$query" $WHOISFILE`

# Service log -- disable if not required
echo `date` " $enquirer@$source \"$query\": $result" >>_stats/peter.log

echo "Your query was: $query
The result was:
$result$fingermsg" |
_bin/netmail -mort \
	LOGNAME=postmaster \
	NAME="Name Server" \
	SUBJECT="result of your query at $home" \
	$enquirer@$source >/dev/null
