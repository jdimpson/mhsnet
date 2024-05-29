#!/bin/sh
#
#	@(#)ftpserver.sh	1.24 96/04/23
#
#	Shell for remote ftp service using `handler' as `ftpserver'.
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

#	CHECK THIS IS VALID FOR YOUR SITE!
PATH=/bin:/usr/bin:/usr/ucb:/usr/bsd:/usr/local:/usr/local/bin
export PATH

#	CHECK THESE ARE VALID FOR YOUR SITE!
COUNTRY=au
ARCHIE=plaza.aarnet.oz.au

DATE="`date '+%y/%m/%d %H:%M'`"
LOG=_stats/ftpserver.log
TMP=/tmp/M_ftp$$
tmp=$TMP.

timeout="_bin/nettimeout -w 9000"	# NB: *more* than "-A" flag to router

trap 'rm -f $TMP ${tmp}*; echo TIMEOUT >&2; exit 1' 1 14 15

# Process these options:
#	-r	message has been returned due to failure of some kind
#	-E	the error generated at the remote site
#	-F	the flags: -l (do ls), -r (recursive), -v (verbose)
#	-I	original message identifier

returned=
error=
flags=
for i
do
	case "$i" in
	-r)	returned=true;				shift ;;
	-E*)	error=`echo \"$i\" | sed '1s/-E//'`;	shift ;;
	-F*)	flags=`expr "$i" : '-F\(.*\);'`;	shift ;;
	-I*)	OID="`expr \"$i\" : '-I\(.*\)'`";	shift ;;
	-*)						shift ;;
	*)						break ;;
	esac
done

# Ignore flags unless "l" present - then do "ls" instead of "get".
# In addition, "r" causes recursion (i.e. ls -R) [NOT ANY MORE]
# "v" causes verbose (i.e. ls -l).
# Together they do verbose recursion (i.e. ls -lR) [NOT ANY MORE]
# "a" causes ftp "ascii" mode [default: "binary"].

cmd=get
case "$flags" in
*l*)	case $flags in
#	*r*v*|*v*r*)	cmd="ls -lR"	;;	# -R too dangerous
#	*r*)		cmd="ls -R"	;;	# -R too dangerous
	*v*)		cmd="dir"	;;
	*)		cmd="ls"	;;
	esac
esac

case "$flags" in
*a*)	ftpmode=ascii	;;	# Useful for non-UNIX
*x*)	ftpmode=tenex	;;	# For talking to obsolete trash...
*)	ftpmode=binary	;;	# Default, and safest
esac

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

# Then set parameters for ftp from contents of standard input:
#
#	first argument)		the name of the sender
#	second argument)	the name of the host to send the FTP request to
#				OR, a -ve number representing extra parameters
#	rest of arguments)	the names of files to retrieve

set '' `cat`	# 1st is null in case `stdin' is empty
shift		# Remove 1st

case "$1" in
'')	echo "$DATE $ID $size $cmd <>@$source <> ERROR (null ftp parameters)" >> $LOG
				exit 0	;;
*)	requester="$1";		shift	;;
esac

# Default username & password are "anonymous" and requester's address.

username="anonymous"

case "$1" in
'')	ftphost=the_null_hostname	;;
-3)	# Next three args are hostname, username, password
	shift;	ftphost="$1"	shift
		username="$1"	shift
		password="$1"	shift	;;
*)	ftphost=$1;		shift	;;
esac

files="$@"

case "$username" in
ftp)		username=anonymous		;;
esac

case "$username" in
anonymous)	password="$requester@$source"	;;
esac

case "$ftphost:$username" in
*:|:*)
	echo "$DATE $ID $size $cmd $requester@$source $ftphost $files FAIL" >> $LOG
	# Return error as mail in case ftpserver doesn't exist at source.
	echo "Your request was:

	$cmd			<$files>
	internet_address	<$ftphost>
	username		<$username>
	FTP_server		<$home>
" |
	_bin/netmail -mort \
		LOGNAME=postmaster \
		NAME="MHSnet Gateway to Internet FTP" \
		SUBJECT="FTP Request failed at $home" \
		"$requester@$source"
	exit 0
	;;
esac

# Handle returned messages gracefully.

case "$returned" in
true)
	echo "$DATE $ID $size $cmd $requester $ftphost $files RETURNED@$source" >> $LOG
	echo "Your request was:

	$cmd			<$files>
	internet_address	<$ftphost>
	FTP_server		<$source>

Below is the standard error output of the ftp command:

$error" \
	| _bin/netmail -mort \
		LOGNAME=postmaster \
		NAME="MHSnet Gateway to Internet FTP" \
		SUBJECT="FTP Request failed at $source" \
		"$requester"
	exit 0
esac

#	Check for foreign country requests,
#	and forward to local `archie' for more efficient processing.

forward=false	# default: process here

case "$home" in
*.$COUNTRY)
	case "$ftphost" in 
	*.$COUNTRY)		;;
	*)	forward=true	;;
	esac
	;;
esac

case "$forward" in
true)	case "$flags" in
	'')				;;
	*)	flags="-E$flags"	;;
	esac
	echo $requester $ftphost $files | _bin/netmsg -Aftpserver $flags -D$ARCHIE -O$source
	echo "$DATE $ID $size $cmd $requester@$source $ftphost $files FORWARDED to $ARCHIE" >> $LOG
	exit 0
	;;
esac

# Check for someone else's "core".

if [ -s core ]
then
	mv core _lib/unk$$.core
	eval `_bin/netparams -w NCC_ADMIN`
	_bin/netmail -mort \
		LOGNAME=postmaster \
		NAME="MHSnet Gateway to Internet FTP" \
		SUBJECT="\"core\" moved to \"_lib/unk$$.core\"" \
		"Network Admin <$NCC_ADMIN>" </dev/null
fi

# Process the request.

rm -f ${tmp}*
count=0

case "$cmd" in
get)
	echo "user $username $password
$ftpmode"
	for i in $files
	do
		tmf=$tmp$count
		count=`expr $count + 1`
		test -f $tmf || echo "get $i $tmf"
	done
	echo quit
	;;
*)
	echo "user $username $password"
	for i in $files
	do
		tmf=$tmp$count
		count=`expr $count + 1`
		test -f $tmf || echo "$cmd $i $tmf"
	done
	echo quit
esac \
	| $timeout ftp -i -n -g "\"$ftphost\"" >$TMP 2>&1 || echo "ftp error: $?" >>$TMP

# Now send results back.

count=0
size=0

case "$cmd" in
get)
	for i in $files
	do
		tmf=$tmp$count
		count=`expr $count + 1`
		if [ -f $tmf ]
		then
			size=`expr $size + \`ls -lod $tmf|awk '{print $4}'\``
			_bin/netfile -drxN"$ftphost/$i" -I"$OID" -Sftpserver $requester@$source <$tmf 2>>$TMP
			rm -f $tmf
			echo "$DATE $ID $size $cmd $requester@$source $ftphost $i" >> $LOG
		else
			echo "No output from \"$cmd $i\"" >>$TMP
			echo "$DATE $ID 0 $cmd $requester@$source $ftphost $i FAIL" >> $LOG
		fi
	done
	;;
*)
	for i in $files
	do
		tmf=$tmp$count
		count=`expr $count + 1`
		if [ -f $tmf ]
		then
			size=`expr $size + \`ls -lod $tmf|awk '{print $4}'\``
			_bin/netmail -mort \
				LOGNAME=postmaster \
				NAME="MHSnet Gateway to Internet FTP" \
				SUBJECT="$cmd ftp://$ftphost/$i" \
				$requester@$source <$tmf 2>>$TMP
			echo "$DATE $ID $size $cmd $requester@$source $ftphost $i" >> $LOG
			rm -f $tmf
		else
			echo "No output from \"$cmd $i\"" >>$TMP
			echo "$DATE $ID 0 $cmd $requester@$source $ftphost $i FAIL" >> $LOG
		fi
	done
esac

if [ -s core ]	# Cruddy ftp!
then
	echo "ftp: *internal error*" >>$TMP
	mv core _lib/ftp.core
	eval `_bin/netparams -w NCC_ADMIN`
	_bin/netmail -mort \
		LOGNAME=postmaster \
		NAME="MHSnet Gateway to Internet FTP" \
		SUBJECT="\"core\" moved to \"_lib/ftp.core\"" \
		"Network Admin <$NCC_ADMIN>" <$TMP
fi

if [ -s $TMP ]
then
	# Return error as mail in case ftpserver doesn't exist at source.
	( echo "Your request was:

	$cmd			<$files>
	internet_address	<$ftphost>
	FTP_server		<$home>

Below is the standard error output of the ftp command:
"; cat $TMP ) |
	_bin/netmail -mort \
		LOGNAME=postmaster \
		NAME="MHSnet Gateway to Internet FTP" \
		SUBJECT="FTP Request failed at $home" \
		"$requester@$source"
fi

rm -f $TMP
exit 0
