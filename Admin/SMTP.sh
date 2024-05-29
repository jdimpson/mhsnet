#!/bin/sh
#
#	IPsendfile / SMTP-mail interface message handler for `spooler'
#
#	SCCSID @(#)SMTP.sh	1.17 95/11/13
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

# Alter `sm' (location of sendmail, and args) if necessary:
sm="/usr/lib/sendmail -oit -oee -oMrMHSnet"

# Alter `sf' (location of TCPmsg sendfile) if you have it installed:
sf=/local/usr/bin/sendfile
# `sendfile' is used for the MHSnet->IP gateway for file transfer.
# It is a public-domain software package available from ftp://www.cs.su.oz.au/TCPmsg

log=_stats/SMTP.log

# TRACE -- enable for tests
# echo "`date +'%y/%m/%d %H:%M'` ARGS $@" >>"$log"

errmsg=
flags=
origin=

for i
do
	case "$i"
	in
	-E*)	errmsg="`echo \"$i\" | sed '1s/-E//'`";	shift ;;
	-F*)	flags="`expr \"$i\" : '-F\(.*\)'`";	shift ;;
	-I*)	OID="`expr \"$i\" : '-I\(.*\)'`";	shift ;;
	-O*)	origin="`expr \"$i\" : '-O\(.*\)'`";	shift ;;
	-*)						shift ;;
	*)						break ;;
	esac
done

for var in handler ID size source fullso dest fulldest souser user data
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

# TRACE -- enable for tests
# echo "`date +'%y/%m/%d %H:%M'` TRACE $ID $souser@$fullso $fulldest $user $handler" >>"$log"

case "$fulldest" in
*!*)	fulldest="`expr \"$fulldest\" : '.*!\.*\(.*\)'`" ;;
esac

case "$errmsg" in
'')	;;
*)
    {
	sed 's/^	//' <<!
	From: Postmaster@$fullso
	To: $user@$fulldest
	Subject: Undelivered mail returned from $fullso

	*******************************************************************************
	MAIL SENT TO: $souser
	RETURNED FROM: $fullso
	MESSAGE ID WAS: $OID

	Failure explanation follows :-
	$errmsg
	*******************************************************************************

!
    } | $sm "-oMs$fullso" -f"Postmaster@$fullso" "<$user@$fulldest>"
	exit 0
	;;
esac

case "$handler" in
filer)
	# TRACE -- remove after tests
echo "`date +'%y/%m/%d %H:%M'` TCPmsg $ID $souser@$fullso $user@$fulldest $origin" >>"$log"
	exec $sf -f "$souser@$fullso" -n "$data" "$user@$fulldest"
	echo "$0 couldn't exec \"$sf\" for $user@$fulldest" >&2
	exit 1
	;;

fileserver)
	# the set and shift combination ensures that the arguments
	# are nullified if the sed does not output anything

	set '' `(cat -v; echo) | sed -e 's/\^@\^@\^@/^@-^@/' -e 's/\^@/ /g'`
	shift

	souser="$1"
	command="$2"
	options="$3"
	shift; shift; shift
	files="$@"

	flags=
	cmd="get"
	case "$command" in
	List)		case "$options" in
			*R*)	flags="-Elr;"	cmd="ls -R";;
			*)	flags="-El;"	cmd="ls";;
			esac
			;;
	ListVerbose)	case "$options" in
			*R*)	flags="-Elrv;"	cmd="ls -lR";;
			*)	flags="-Elv;"	cmd="ls -l";;
			esac
	esac
	home=`_bin/netaddr`

	# TRACE -- remove after tests
echo "`date +'%y/%m/%d %H:%M'` FTP $ID $souser@$fullso $fulldest $files" >>"$log"

	echo "\
Your message was converted from a netfetch to a netftp request:

	$cmd			<$files>
	internet_address	<$fulldest>
	FTP_server		<$home>

The standard error output of the conversion process was:
" 1>&2

	# We recreate an ftpserver message, so that
	# processing can be done asynchronously if desired.

	echo "$souser" "$fulldest" $files | _bin/netmsg -Aftpserver $flags -D$home "-O$fullso" -I$OID
	exit $?
	;;

mailer)
	case "$user" in
	@*)					;;
	*@*)	user="`echo $user|tr @ %`"	;;
	esac
	# TRACE -- remove after tests
echo "`date +'%y/%m/%d %H:%M'` SMTP $ID $souser@$fullso $fulldest $user" >>"$log"
	exec $sm "-oMs$fullso" -f"$souser@$fullso" "<$user@$fulldest>"
	echo "$0 couldn't exec \"$sm\" for <$user@$fulldest>" >&2
	exit 1
	;;

peeper)
	home=`_bin/netaddr`
	dnsserver=`expr "$flags;" : '[^;]*;[^;]*;[^;]*;@\([^;]*\);.*'`

	# TRACE -- remove after tests
	requester=`expr "$flags;" : '\([^;]*\);.*'`
	    query=`expr "$flags;" : '[^;]*;\([^;]*\);.*'`
echo "`date +'%y/%m/%d %H:%M'` DIG $ID $requester@$fullso $query@${dnsserver:-$fulldest}" >>"$log"

	_bin/netmsg -Apeeper -D$home "-O$fullso" -I$OID "-E$flags;@${dnsserver:-$fulldest}"
	exit $?
	;;

peter)
	home=`_bin/netaddr`
	enquirer=`expr "$flags" : '[^;]*;\([^;]*\)'`
	query=`expr "$flags" : '\([^;]*\)'`

	# TRACE -- remove after tests
echo "`date +'%y/%m/%d %H:%M'` FINGER $ID $enquirer@$fullso $query@$fulldest" >>"$log"

	echo "$enquirer" "$query@$fulldest" | _bin/netmsg -Afinger -D$home "-O$fullso" -I$OID
	exit $?
	;;
esac

case "$handler" in
	mailer)		type="Mail" ;;
	stater)		type="Routing update" ;;
	filer)		type="File delivery" ;;
	fileserver)	type="Fetch files" ;;
	finger)		type="Finger query" ;;
	ftpserver)	type="Remote FTP" ;;
	peeper)		type="DNS query" ;;
	peter)		type="Name query" ;;
	printer)	type="Printing" ;;
	reporter)	type="News" ;;
	*)		type="$handler" ;;
esac

# TRACE -- remove after tests
echo "`date +'%y/%m/%d %H:%M'` REJECT $ID $type From: $fullso To: $fulldest" >>"$log"

echo "$type service unavailable at \"`_bin/netaddr`\" for \"$fulldest\".
This is an SMTP/FTP gateway (mail/ftp only) to \"$fulldest\".

[Please check your address - MHSnet addresses for the \"$type\"
 service should be fully-qualified (end in a country code).]" >&2
exit 1
