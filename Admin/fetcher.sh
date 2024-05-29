#!/bin/sh
trap '' 15
trap 'kill -15 0; exit 1' 14
#
#	@(#)fetcher.sh	1.8 96/02/12
#
#	Shell for remote management service using ``handler'' as ``fetcher''.
#
#	By default, this script will return an error for all requests.
#	To enable, edit the lines starting "legal ...".
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

# Process these options:
#	-r	message has been returned due to failure of some kind
#	-E	the error generated at the remote site

PATH=/bin:/usr/bin:/usr/ucb:/usr/local:/usr/local/bin:/usr/spool/MHSnet/_bin
export PATH

LOG=_stats/fetcher.log

returned=
error=
flags=
for i
do
	case "$i" in
	-r)	returned=true;				shift ;;
	-E*)	error=`echo \"$i\" | sed '1s/-E//'`;	shift ;;
	-*)						shift ;;
	*)						break ;;
	esac
done

# Now interpret arguments.

home=$1
source=$3
ID="$5"
size=$6

# Then set arguments to contents of standard input
#	first argument)		the name of the sender
#	second argument)	the name of the command
#	rest of arguments)	the arguments of the command

set '' `cat`	# 1st is null in case `stdin' is empty
shift		# Remove 1st

DATE=`date '+%y/%m/%d %H:%M'`

case "$1" in
"")	echo "$DATE $size user=<> (cannot return request) ERROR" >>$LOG; exit 0;;
*)	requester=$1;					shift	;;
esac

case "$1" in
"")	command=the_null_command				;;
*)	command=$1;					shift	;;
esac

# Handle returned messages gracefully.

case "$returned" in
true)
	echo "$DATE $size $requester $command $* RETURNED@$source" >>$LOG
	echo "Your request was:

	$command $*

Below is the standard error output of the command:

$error" \
	| _bin/netmail -mort \
		LOGNAME=postmaster \
		NAME="MHSnet remote management service" \
		SUBJECT="\"$command\" failed at $source" \
		"$requester"
	exit 0
esac

# Process the command.

out=/tmp/fetcher.$$
err=/tmp/fetcherr.$$

if [ -s core ]	# Someone else's
then
	mv core _lib/unknown.core
fi

NOT=

for i
do
	case "$i" in
	*';'*|*'
'*|*'|'*|*'&'*|*'<'*|*'>'*)
		NOT=1	# Suspicious chars in arg list
		;;
	esac
done

case "$requester" in
"legal usernames")
	;;
*)	NOT=1
	;;
esac

case "$source" in
"legal source addresses")
	;;
*)	NOT=1
	;;
esac

case "$NOT$command$NOT" in
"legal commands")
	$command "$@" >$out 2>$err
	;;
*)	echo "Illegal command: \"$command $*\"." >$err
	NOT=1
	;;
esac
status=$?

case "$NOT" in
1)	eval `netparams NCC_MANAGER`	# Copy to mgmt if illegal
	;;
esac

# Now send results back.

if [ -s $out ]
then
	size=`ls -lod $out|awk '{print $4}'`
	_bin/netmail -mort \
		LOGNAME=postmaster \
		NAME="MHSnet remote management service" \
		SUBJECT="$command $*" \
		"Network Manager <$NCC_MANAGER>" \
		$requester@$source <$out >/dev/null 2>>$err
else
	size=0
	case $status in
	0)	;;
	*)	echo "No output from:  \"$command $*\"." >>$err
		;;
	esac
fi

if [ -s core ]	# Some command!
then
	echo "$command: *internal error*" >>$err
	mv core "_lib/$command.core"
fi

if [ -s $err ]
then
	size=`expr $size + \`ls -lod $err|awk '{print $4}'\``
	echo "$DATE $size $requester@$source $command $* FAIL" >>$LOG
	# Return error as mail in case fetcher doesn't exit at source.
	( echo "Your request was:

	$command $*

Below is the standard error output of the command:
"; cat $err ) |
	_bin/netmail -mort \
		LOGNAME=postmaster \
		NAME="MHSnet remote management service" \
		SUBJECT="\"$command\" failed at $home" \
		"Network Manager <$NCC_MANAGER>" \
		"$requester@$source" >/dev/null 2>&1
else
	echo "$DATE $size $requester@$source $command $*" >>$LOG
fi

rm -f $out $err
exit 0
