#!/bin/sh
trap '' 14 15
#
#	@(#)wwwpusher.sh	1.3 96/02/12
#
#	Shell script for WWW push-cache service.
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

PATH=/bin:/usr/bin:/usr/ucb:/usr/bsd:/usr/local:/usr/local/bin
export PATH

NAME=wwwpcserver
VERSION=0.1	# 1.3
LOG=_stats/$NAME.log

CACHE_BASE=/usr/other/web/cache
CACHE_OWNER=webcache
#CACHE_BASE=/tmp/$NAME.dir

DATE="`date '+%y/%m/%d %T'`"

# TRACE -- enable for tests
# echo $DATE "$@" >>$LOG

for i
do
	case "$i" in
#	-A*)	ACKID=$i;						shift ;;
#	-D*)	DUP=$i;							shift ;;
	-E*)	ERR=`echo "$i" | sed '1s/-E//'`;			shift ;;
	-F*)	FLAGS=`echo "$i" | sed '1s/-F//' | tr ';' '\012'`;	shift ;;
	-I*)	OID=`expr "$i" : '-I\(.*\)'`;				shift ;;
#	-W*)	FWD=$i;							shift ;;
	-r)	RET=true;						shift ;;
	-*)								shift ;;
	*)								break ;;
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

case "$OID" in
'')	OID="$ID"	;;
esac

# TRACE -- enable for tests
# echo "$DATE $ID $source $size" >>$LOG

case "$RET" in
true)	eval `_bin/netparams -w NCC_ADMIN`
	echo "Message for handler \"$NAME\" returned from \"$source\":
Flags=$FLAGS

$ERR" \
	| _bin/netmail -mort \
		LOGNAME=postmaster \
		NAME="MHSnet $NAME message handler" \
		SUBJECT="\"$NAME\" request failed at \"$source\"" \
		"Network Admin <$NCC_ADMIN>"
	echo "$DATE $ID $source $size RETURNED" >>$LOG
	exit 0
esac

#	Validate sender.

case "$source" in
*.cs.su.oz.au)
	;;
*)	echo "Invalid source for cache document" 1>&2
	echo "$DATE $ID $source $size INVALID SOURCE" >>$LOG
	exit 1
	;;
esac

#	Process flags.

for f in $FLAGS
do
	case "$f" in
	URL=ftp*)
		file=ftp/`expr "$f" : 'URL=ftp://\(.*\)'`
		;;
	URL=gopher*)
		file=gopher/`expr "$f" : 'URL=gopher://\(.*\)'`
		;;
	URL=http*)
		file=http/`expr "$f" : 'URL=http://\(.*\)'`
		;;
	Content-type=*)
		CONTENT_TYPE=`expr "$f" : 'Content-type=\(.*\)'`
		;;
	*)
		echo "unknown flag \"$f\"" 1>&2
		echo "$DATE $ID $source $size UNKNOWN FLAG: $f" >>$LOG
		exit 1
		;;
	esac
done

case "$file:$CONTENT_TYPE" in
*:|:*|:)
	echo "missing cache specifications" 1>&2
	echo "$DATE $ID $source $size MISSING SPEC: $CONTENT_TYPE:$file" >>$LOG
	exit 1
esac

echo "$DATE $ID $source $size $CONTENT_TYPE $file" >>$LOG

CACHE_FILE=$CACHE_BASE/$file
CACHE_DIR=`dirname $CACHE_FILE`
mkdir -m 775 -p $CACHE_DIR || exit 1

#	Cache info file ($CACHE_DIR/.cache_info) format:

#	name	delay	last_chkd	expires		max_und	last_mod
#	jon.gif	19	804763802	809392365	1209600	758478004

DATE_NUM=`_bin/netdatenum`
EXPIRES_NUM=`expr $DATE_NUM + 10000000`

echo "`basename $CACHE_FILE`	20	$DATE_NUM	$EXPIRES_NUM	2000000	$DATE_NUM" >>$CACHE_DIR/.cache_info || exit 1
chmod 644 $CACHE_DIR/.cache_info

#	Cached document header:

#	HTTP/1.0 200 Document follows
#	MIME-Version: 1.0
#	Server: CERN/3.0
#	Date: Monday, 03-Jul-95 09:29:44 GMT
#	Content-Type: image/gif
#	Content-Length: 32730
#	Last-Modified: Thursday, 13-Jan-94 16:20:04 GMT

DATE=`date -u '+%a, %d %b %Y %T GMT'`

cat >$CACHE_FILE <<!
HTTP/1.0 200 Document follows
MIME-Version: 1.0
Server: $NAME/$VERSION
Date: $DATE
Content-type: $CONTENT_TYPE
Content-length: $size
Last-modified: $DATE

!

#	Followed by data (comes in on stdin).

cat >>$CACHE_FILE

chmod 664 $CACHE_FILE || exit 1
chown $CACHE_OWNER $CACHE_DIR $CACHE_DIR/.cache_info $CACHE_FILE

exit 0
