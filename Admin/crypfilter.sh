#!/bin/sh
#
#	`Encrypt' shell script for `filter'.
#
#	SCCSID @(#)crypfilter.sh	1.5 96/04/26
#
#	Any inbound message with the flag ENCRYPTED will be decrypted and decompressed,
#	any outbound message will be compressed, encrypted, and flagged.
#
#	Invoked with optional args:
#		-D<data descriptions>	(each file terminated by a ';')
#		-E<message flags>	(each flag terminated by a ';')
#		-F<filter flags>	(each flag terminated by a ';')
#		-R<message route>	(each node terminated by a ';')
#	and these:
#		-i OR -o		(INPUT/OUTPUT message)
#		<home name>
#		<link name>
#		<link dir>
#		<data size>
#		<handler>
#		<message ID[/part]>
#		<source address>
#		<dest address>
#		[sender user name]	(if FTP message)
#		[dest user names]	(if FTP message)
#

PATH=/bin:/usr/bin:/etc:/etc/bin:/usr/ucb:/usr/local:/usr/local/bin
export PATH
#
#	We recommend that you use PGP - which does compression automatically
#	(available from ftp://ftp.connect.com.au/pub/security/pgp26ui-src.tar.gz)
#
ENCRYPT='pgp'
DECRYPT='pgp'
#
#	Otherwise, if you have your own encryption program:
#
#ENCRYPT=encrypt		# must accept key from a file
#DECRYPT=decrypt		# must accept key from a file
#COMPRESS='gzip -f'		# OR "compress -f"
#UNCOMPRESS=gunzip		# OR "uncompress"

# TRACE -- enable for tests
# echo `date +'%y/%m/%d %H:%M'` "$@" >>_stats/crypt.log

for i
do
	case "$i" in
	-F*)	flags=$i;	shift ;;
	-i)	dir='IN ';	shift ;;
	-o)	dir='OUT';	shift ;;
	-*)			shift ;;
	*)			break ;;
	esac
done

home="$1"
link="$2"
shift
dir="$2"
size="$3"
handler="$4"
id="$5"
source="$6"
dest="$7"
fuser="$8"
tuser="$9"

# TRACE -- enable for tests
# echo "`date +'%y/%m/%d %H:%M'` $dir $home $link $size $id $flags" >>_stats/crypt.log

#	Data comes in on stdin, changed data should go out on stdout.

case "$ENCRYPT" in
*pgp)
	case "$dir" in
	OUT)	case "$handler" in
		mailer|filer)	;;
		*)	exit 0	;;	# why bother?
		esac
		PGPPASSFD=3 $ENCRYPT -fc 3<$linkdir/cryptkey &&
			echo "ENCRYPTED@$home" 1>&2 &&
			exit 40	# Data changed, flag on stderr
		;;
	*)	case "$flags" in
		*ENCRYPTED@$link\;)
			PGPPASSFD=3 $DECRYPT -f 3<$linkdir/cryptkey &&
				echo "DELETE ENCRYPTED@$link" 1>&2 &&
				exit 40	# Data changed, delete flag on stderr
			echo "Error return from: $DECRYPT -f" 1>&2
			exit 128 # ERROR: data unchanged, error on stderr.
			;;
		*ENCRYPTED@$home\;)
			PGPPASSFD=3 $DECRYPT -f 3<$$linkdir/cryptkey &&
				echo "DELETE ENCRYPTED@$home" 1>&2 &&
				exit 40	# Data changed, delete flag on stderr
			echo "Error return from: $DECRYPT -f" 1>&2
			exit 128 # ERROR: data unchanged, error on stderr.
			;;
		esac
		;;
	esac
	;;
*)
	case "$dir" in
	OUT)	case "$handler" in
		mailer|filer)	;;
		*)	exit 0	;;	# why bother?
		esac
		$COMPRESS |
			$ENCRYPT -k $linkdir/cryptkey &&
			echo "ENCRYPTED@$home" 1>&2 &&
			exit 40	# Data changed, flag on stderr
		;;
	*)	case "$flags" in
		*ENCRYPTED@$link\;)
			$DECRYPT -k $linkdir/cryptkey |
				$UNCOMPRESS &&
				echo "DELETE ENCRYPTED@$link" 1>&2 &&
				exit 40	# Data changed, delete flag on stderr
			echo "Error return from: $DECRYPT -k | $UNCOMPRESS" 1>&2
			exit 128 # ERROR: data unchanged, error on stderr.
			;;
		*ENCRYPTED@$home\;)
			$DECRYPT -k $linkdir/cryptkey |
				$UNCOMPRESS &&
				echo "DELETE ENCRYPTED@$home" 1>&2 &&
				exit 40	# Data changed, delete flag on stderr
			echo "Error return from: $DECRYPT -k | $UNCOMPRESS" 1>&2
			exit 128 # ERROR: data unchanged, error on stderr.
			;;
		esac
		;;
	esac
	;;
esac

exit 0	# Data unchanged
