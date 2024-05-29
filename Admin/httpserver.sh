#!/bin/sh
#
#	SCCSID	@(#)httpserver.sh	1.16 06/05/23
#
#	Shell for remote http service using `handler' as `httpserver'.
#
#	Invoked in SPOOLDIR with optional args:
#		-r			(message is being returned)
#		-A<message ID>		(message is an acknowledgement, with original ID)
#		-D<site;[site;...]>	(message may have been duplicated at each site)
#		-E<reason>		(error message from header)
#		-F<flag;[flag;...]>	(handler flags inserted at source)
#		-I<message ID>		(original message identifier)
#		-W<site;[site;...]>	(message was forwarded by each site)
#	and these:
#		<home name>
#		<typed home name>
#		<source address>
#		<typed source address>
#		<message ID>
#		<data size>

#	Needs <restrict> flag in "_lib/handlers" for security.

#	CHECK THIS IS VALID FOR YOUR SITE!
PATH=/bin:/usr/bin:/usr/ucb:/usr/bsd:/usr/local:/usr/local/bin
export PATH

DATE="`date '+%y/%m/%d %H:%M'`"
LOG=_stats/httpserver.log
TMP=/tmp/M_http$$
tmp=$TMP.
TMP2=/tmp/M_http_log$$

httpget="_lib/httpget"
wget="/usr/bin/wget"
timeout="_bin/nettimeout -w 36000"	# NB: *more* than "-A" flag to router

trap 'rm -f $TMP $TMP2 ${tmp}*; echo TIMEOUT >&2; exit 1' 1 14 15

# Process these options:
#	-r	message has been returned due to failure of some kind
#	-E	the error generated at the remote site
#	-I	original message identifier

returned=
error=
for i
do
	case "$i" in
	-r)	returned=true;				shift ;;
	-E*)	error=`echo \"$i\" | sed '1s/-E//'`;	shift ;;
	-I*)	OID="`expr \"$i\" : '-I\(.*\)'`";	shift ;;
	-*)						shift ;;
	*)						break ;;
	esac
done

# Setup useful arguments.

for var in home typedhome source typedsource ID size
do
	case $# in
	0)	eval "$var=-"
		;;
	*)	eval "$var=\"\$1\""
		shift
		;;
	esac
done

case "$OID" in
'')	OID="$ID"	;;
esac

# Then set parameters for http from contents of standard input:
#
#	first argument)		the name of the sender
#	rest of arguments)	the URLs

set '' `cat`	# 1st is null in case `stdin' is empty
shift		# Remove 1st

case "$1" in
'')	echo "$DATE $ID $size <>@$source <> ERROR (null http parameters)" >>"$LOG"
				exit 0	;;
*)	requester="$1";		shift	;;
esac

URLs="$@"

# Handle returned messages gracefully.

case "$returned" in
true)
	echo "$DATE $ID $size $requester $URLs RETURNED@$source" >>"$LOG"
	echo "Your request was:

	URLs			<$URLs>
	HTTP_server		<$source>

Below is the standard error output of the http command:

$error" \
	| _bin/netmail -mort \
		LOGNAME=postmaster \
		NAME="MHSnet Gateway to Internet HTTP" \
		SUBJECT="HTTP Request failed at $source" \
		"$requester"
	exit 0
esac

# Check for someone else's "core".

if [ -s core ]
then
	mv core _lib/unk$$.core
	eval `_bin/netparams -w NCC_ADMIN`
	_bin/netmail -mort \
		LOGNAME=postmaster \
		NAME="MHSnet Gateway to Internet HTTP" \
		SUBJECT="\"core\" moved to \"_lib/unk$$.core\"" \
		"Network Admin <$NCC_ADMIN>" </dev/null
fi

# Process the request.

rm -f ${tmp}*
count=0
for i in $URLs
do
	case "$i" in
	http://*/*)		;;
	http://*)	i="$i/"	;;
	esac
	tmf=$tmp$count
	count=`expr $count + 1`
	test -f $tmf ||
		if test -x $wget
		then
			# Might need proxies setup here!
			$timeout $wget -nv -O $tmf "\"$i\"" >$TMP2 2>&1 || echo "wget error exit: $?"
		else
			$timeout $httpget "\"$i\"" $tmf >$TMP2 2>&1 || echo "httpget error exit: $?"
		fi >$TMP 2>&1
done

if [ -s core ]	# Cruddy httpget!
then
	echo "httpget: *internal error*" >>$TMP
	mv core $httpget.core
	eval `_bin/netparams -w NCC_ADMIN`
	_bin/netmail -mort \
		LOGNAME=postmaster \
		NAME="MHSnet Gateway to Internet HTTP" \
		SUBJECT="\"core\" moved to \"$httpget.core\"" \
		"Network Admin <$NCC_ADMIN>" <$TMP
fi

if [ -s $TMP ]
then
	# Return error as mail in case httpserver doesn't exist at source.
	( echo "Your request was:

	URLs			<$URLs>
	HTTP_server		<$home>

Below is the standard error output of the httpserver command:
"; cat $TMP $TMP2 ) |
	_bin/netmail -mort \
		LOGNAME=postmaster \
		NAME="MHSnet Gateway to Internet HTTP" \
		SUBJECT="HTTP Request failed at $home" \
		"$requester@$source"
	rm -f $TMP $TMP2 ${tmp}*
	echo "$DATE $ID $size $requester@$source $URLs ERROR" >>"$LOG"
	exit 0
fi

# Now send results back.

count=0
size=0
for i in $URLs
do
	tmf=$tmp$count
	count=`expr $count + 1`
	if [ -f $tmf ]
	then
		size=`expr $size + \`ls -lod $tmf|awk '{print $4}'\``
		_bin/netfile -drxN"$i" -I"$OID" -Shttpserver $requester@$source <$tmf 2>>$TMP
		rm -f $tmf
		echo "$DATE $ID $size $requester@$source $i" >>"$LOG"
	else
		echo "No output from URL \"$i\"" >>$TMP
		echo "$DATE $ID 0 $requester@$source $i FAIL" >>"$LOG"
	fi
done

if [ -s $TMP ]
then
	# Return error as mail in case httpserver doesn't exist at source.
	( echo "Your request was:

	URLs			<$URLs>
	HTTP_server		<$home>

Below is the standard error output of the http command:
"; cat $TMP ) |
	_bin/netmail -mort \
		LOGNAME=postmaster \
		NAME="MHSnet Gateway to Internet HTTP" \
		SUBJECT="HTTP Request failed at $home" \
		"$requester@$source"
fi

rm -f $TMP $TMP2
exit 0
