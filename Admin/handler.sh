#!/bin/sh
trap '' 15
trap 'kill -15 0; exit 1' 14
#
#	@(#)handler.sh	1.5 96/02/12
#
#	Default shell script for "handler".
#
#	Invoked in SPOOLDIR with optional args:
#		-r			(message is being returned)
#		-A<ID>			(message is an acknowledgement, with original ID)
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
#

PATH=/bin:/usr/bin:/etc:/etc/bin:/usr/ucb:/usr/local:/usr/local/bin
export PATH

# TRACE -- enable for tests
# echo `date +'%y/%m/%d %H:%M'` "$@" >>_stats/handler.log

for i
do
	case "$i" in
#	-A*)	ACKID=$i;	shift ;;
#	-D*)	DUP=$i;		shift ;;
#	-E*)	ERR=$i;		shift ;;
#	-F*)	FLAGS=$i;	shift ;;
#	-I*)	ORIGID=$i;	shift ;;
#	-W*)	FWD=$i;		shift ;;
	-r)	RET=true;	shift ;;
	-*)			shift ;;
	*)			break ;;
	esac
done

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

# TRACE -- enable for tests
echo `date +'%y/%m/%d %H:%M'` " $home $source $ID $size" >>_stats/handler.log

#	Data comes in on stdin.

cat >>/dev/null

case "$RET" in
true)	eval `_bin/netparams -w NCC_ADMIN`
	echo "Message for handler \"handler\" returned from \"$source\"" \
	| _bin/netmail -mort \
		LOGNAME=postmaster \
		NAME="MHSnet default message handler" \
		SUBJECT="\"handler\" failed at \"$source\"" \
		"Network Admin <$NCC_ADMIN>"
	exit 0
esac

echo "Message received for \"handler\" returned." 2>&1
exit 1
