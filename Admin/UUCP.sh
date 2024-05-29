#!/bin/sh
#
#	UUCP `uucp'/`uux' message handler for `spooler'
#
#	SCCSID @(#)UUCP.sh	1.11 95/10/26
#
#############################################################################
#
#	NOTE: this will have to be altered to suit the local version of uucp.
#
#	You should also use the `linkname' command to netstate(8) to add
#	a name that is the same as the UUCP name for the remote site.
#
#############################################################################
#
#	Invoked with optional args:
#		-D<dup address>		(message may have been duplicated there)
#		-E<error message>	(message is returning with this error reason)
#		-F<message flags>	(handler flags added at source)
#		-I<message ID>		(original message identifier)
#		-L<link name>		(local name for link from `linkname')
#		-O<origin>		(first MHSnet node in route iff != source)
#		-T<trunc address>	(message was truncated there)
#	and these:
#		<handler>
#		<ID>
#		<data size>
#		<source address>
#		<source address + domains>
#		<dest address>
#		<dest address + domains>
#	and, if FTP, these:
#		[sender user name]
#		[dest user name]
#		[data name]
#

PATH=/bin:/usr/bin:/etc:/etc/bin:/usr/ucb:/usr/local:/usr/local/bin
export PATH

noautocall=-r	# Remove if you want uucico triggered from uucp/uux.
use_uux=1	# Set to 0 if you want to use `uucp' rather than `uux'.

LOG=_stats/UUCP.log

ermsg=
hname="`_bin/netaddr`"
origin=
ulink=
date="`date '+%a %h %d %T 19%y'`"
logdate="`date +'%y/%m/%d %H:%M'`"

# TRACE -- enable for tests
# echo $logdate " $@" >>"$LOG"

for i
do
	case "$i"
	in
	-E*)	ermsg="`echo \"$i\" | sed '1s/-E//'`";	shift ;;
	-I*)	OID="`expr \"$i\" : '-I\(.*\)'`";	shift ;;
	-L*)	ulink="`expr \"$i\" : '-L\(.*\)'`";	shift ;;
#	-O*)	origin="`expr \"$i\" : '-O\(.*\)'`";	shift ;;
	-*)						shift ;;
	*)						break ;;
	esac
done

for var in handler ID size source fullso dest fulldest souser user name
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

case "$user" in
'')	user=postmaster	;;
esac

uuser="$user"

case "$ulink" in
'')	link=`_bin/netaddr -r "$fulldest"|sed 's/.* \.//'`
	;;
*)	link=`_bin/netaddr -r "$ulink"|sed 's/.* \.//'`
	;;
esac

case "$ulink" in
'')	ulink=`expr "$link" : '\([^.]*\)'`
	;;
esac

# TRACE -- disable after tests
echo "$logdate $handler From: $souser@$fullso To: $user@$fulldest Via: $ulink" >>"$LOG"

uname=`(uuname -l)2>/dev/null`

case "$uname" in
'')	echo "UUCP gateway to $dest failed: UUCP package not installed." 1>&2
	echo "$logdate UUCP gateway to $dest failed: UUCP package not installed." >>"$LOG"
	exit 1
	;;
esac

case "$ermsg" in
'')	;;
*)
	case "$link" in
	*$fulldest)			;;
	*)	uuser="($dest!$user)"	;;
	esac
	echo "$logdate uux $noautocall - -gC -aPostmaster@$fullso \"$ulink!rmail\" \"$uuser\"" >>"$LOG"
	{
		case "$source" in
		"$uname")
			echo "From Postmaster $date remote from $uname
Received: by $hname with MHSnet
	id $OID; $date
	(from Postmaster for $user@$fulldest)"
			;;
		*)	echo "From Postmaster@$fullso $date remote from $uname
Received: from $fullso by $hname with MHSnet
	id $OID; $date
	(from Postmaster@$fullso for $user@$fulldest)"
			;;
		esac
		echo "Date: $date
From: Postmaster@$fullso
Subject: Undelivered mail returned from $fullso

*******************************************************************************
MAIL SENT TO: $souser
RETURNED FROM: $fullso
MESSAGE ID WAS: $OID

Failure explanation follows :-
$ermsg
*******************************************************************************
"
		sed -e 's/^From />From /'
	} | uux $noautocall - -gC "-aPostmaster@$fullso" "$ulink!rmail" "$uuser" >>"$LOG" 2>&1
	exit 0
	;;
esac

case "$handler" in
filer)
	case "$name" in
	'')	if [ -s $0.num ]; then read name <$0.num; else name=0; fi
		name=`expr ${name:-0} + 1`
		echo $name >$0.num
		name="$source.$name"
		;;
	???????????????*)	# >14 chars
		name=`expr "$name" : '.*\(..............\)'`
		;;
	esac
	case "$link" in
	*$fulldest)	uuser="~$user/$name"		;;
	*)		uuser="($dest!~$user/$name)"	;;
	esac
	case "$use_uux" in
	1)
		echo "$logdate uux $noautocall - -gF -a$souser@$fullso -b \"$ulink!uusend\" - \"$uuser\"" >>"$LOG"
		exec uux $noautocall - -gF "-a$souser@$fullso" -b "$ulink!uusend" - "$uuser" >>"$LOG" 2>&1
		;;
	*)
		# WARNING - under BSD/386 and possibly some other operating systems
		# there are severe restrictions on the source path of uucp'ed files.
		# Ensure that the temporary path used is within such a path.
		# Usually uucppublic is permitted.
		tmpnam=/var/spool/uucppublic/MHS.$$
		cat >"$tmpnam"
		# The source file must typically be globally readable...
		chmod 644 "$tmpnam"
		echo "$logdate uucp $noautocall -gF -C -n$user $tmpnam $ulink!~/$name" >>"$LOG"
		uucp $noautocall -gF -C "-n$user" "$tmpnam" "$ulink!~/$name" >>"$LOG" 2>&1
		rm "$tmpnam"
		;;
	esac
	exit 0
	;;
mailer)
	case "$link" in
	*$fulldest)			;;
	*)	uuser="($dest!$user)"	;;
	esac
	echo "$logdate uux $noautocall - -gC -a$souser@$fullso \"$ulink!rmail\" \"$uuser\"" >>"$LOG"
	{
		case "$source" in
		"$uname")
			echo "From $souser $date remote from $uname
Received: by $hname with MHSnet
	id $OID; $date
	(from $souser for $user@$fulldest)"
			;;
		*)	echo "From $souser@$fullso $date remote from $uname
Received: from $fullso by $hname with MHSnet
	id $OID; $date
	(from $souser@$fullso for $user@$fulldest)"
			;;
		esac
		sed -e '1s/^From \([^ ]*\) .*/Sender: \1/'
	} | uux $noautocall - -gC "-a$souser@$fullso" "$ulink!rmail" "$uuser" >>"$LOG" 2>&1
	exit 0
	;;
reporter)
	case "$link" in
	*$fulldest)
		echo "$logdate uux -r - -gn -n \"$ulink!rnews\"" >>"$LOG"
		exec uux -r - -gn -n "$ulink!rnews"
		exit 0
		;;
	esac
	type="News"
	;;
stater)
	exec cat > /dev/null
	exit 0
	;;

fileserver)
	type="Fetch files"
	;;
peter)
	type="Name query"
	;;
printer)
	type="Printing"
	;;
*)
	type="$handler"
	;;
esac

case "$link" in
*$fulldest)
	echo "$type service unavailable at $fulldest" >&2
	;;
*)
	echo "$type service unavailable at $fulldest via $link" >&2
	;;
esac
exit 1
