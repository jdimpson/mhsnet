#!/bin/sh
trap '' 15
trap 'kill -15 0; exit 1' 14
#
#	@(#)finger.sh	1.7 96/02/12
#
#	Shell for remote user information using ``handler'' as ``finger''.
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

PATH=/bin:/usr/bin:/usr/ucb:/usr/local:/usr/local/bin
export PATH

returned=
error=
for i
do
	case "$i" in
	-r)	returned=true;				shift ;;
	-E*)	error=`echo \"$i\" | sed '1s/-E//'`;	shift ;;
	-*)						shift ;;
	*)						break ;;
	esac
done

# Now interpret arguments

home=$1
source=$3

# Then set arguments to contents of standard input
#	first argument)		the name of the sender
#	rest of arguments)	the strings to pass to finger

set '' `cat`	# 1st is null in case `stdin' is empty
shift		# Remove 1st

case "$1" in
"")	echo "`date '+%y/%m/%d %H:%M'` user=<> (cannot return request from $source) ERROR" >> _stats/finger.log
	exit 0			;;
*)	requester="$1";	shift	;;
esac

names="$@"

# Handle returned messages gracefully

case "$returned" in
true)
	echo "`date '+%y/%m/%d %H:%M'` $requester@$home $names RETURNED@$source" >> _stats/finger.log
	echo "Your request was:

	finger		<$names>
	finger_server	<$source>
	requester	<$requester@$home>

Below is the standard error output of the finger command:

$error" \
	| _bin/netmail -mort \
		LOGNAME=postmaster \
		NAME="MHSnet Gateway to Internet \`finger'" \
		SUBJECT="Finger Request failed at $source" \
		"$requester"
	exit 0
esac

# Process the request

(
	echo "Your request was:

	finger		<$names>
	finger_server	<$home>
	requester	<$requester@$source>

Below is the standard output and standard error of the finger command:
"
	finger $names 2>&1 | tr -d '\015'
) \
	| _bin/netmail -mort \
		LOGNAME=postmaster \
		NAME="MHSnet Gateway" \
		SUBJECT="Results of your finger request at $home" \
		"$requester@$source"

echo "`date '+%y/%m/%d %H:%M'` $requester@$source $names" >> _stats/finger.log
exit 0
