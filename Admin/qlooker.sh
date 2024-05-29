#!/bin/sh
#
#	@(#)qlooker.sh	1.9 96/02/12
#
#	Shell for remote queue enquiry using ``handler'' as ``qlooker''.
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

# TRACE -- enable for tests
# echo `date +'%y/%m/%d %H:%M'` "$@" >>_stats/qlooker.log || echo

for i
do
	case "$i" in
	-A*)	echo "Unexpected ACK" 1>&2; exit 1;		shift ;;
	-E*)	err="$err: `echo \"$i\" | sed '1s/-E//'`";	shift ;;
	-W*)	echo "Forwarding disallowed" 1>&2; exit 1;	shift ;;
	-r)	err="Enquiry RETURNED"; returned=true;		shift ;;
	-*)							shift ;;
	*)							break ;;
	esac
done

home="$1"
source="$3"

#	Data is name of enquirer

read enquirer

case "$returned" in
true)	case "$enquirer" in
	'')	echo "$err" 1>&2; exit 1;;
	esac
	echo "$err" | _bin/netmail -mort \
		LOGNAME=postmaster \
		NAME="Network Administration" \
		SUBJECT="network queues enquiry failed at $source" \
		$enquirer
	exit 0
	;;
esac

case "$enquirer" in
#[Pp][Oo][Ss][Tt][Mm][Aa][Ss][Tt][Ee][Rr]|daemon|network|root)
network)restrict=; qualifier=; flags=ae		;;	# Trusted names
'')	echo "No name for enquirer" 1>&2; exit 1;;	# Anon!
*)	restrict="-U$enquirer"
	qualifier=" for $enquirer@$source"
	flags=					;;	# Some user
esac

# LOG -- disable if not required
echo `date` " $enquirer@$source $restrict" >>_stats/qlooker.log || echo

{
  echo "Messages to   $source:
"
  _bin/netq -${flags}D$source $restrict
  echo "
Messages from $source:
"
  _bin/netq -${flags}S$source $restrict
} | _bin/netmail -mort \
	LOGNAME=postmaster \
	NAME="Network Administration" \
	SUBJECT="network queues at $home$qualifier" \
	$enquirer@$source >/dev/null
