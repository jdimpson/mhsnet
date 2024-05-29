#!/bin/sh
#
#	`Compress' shell script for `filter'.
#
#	SCCSID @(#)pressfilter.sh	1.6 96/04/26
#
#	Any inbound message with the flag COMPRESSED will be uncompresed,
#	any outbound message >999 bytes will be compressed and flagged.
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

COMPRESS='compress -f -b 14'	# default, works anywhere
UNCOMPRESS=uncompress

LOG=_stats/press.log

# TRACE -- enable for tests
# echo `date +'%y/%m/%d %H:%M'` "$@" >>"$LOG"

for i
do
	case "$i" in
	-F*)	flags="$i";	shift ;;
	-i)	dir='IN ';	shift ;;
	-o)	dir='OUT';	shift ;;
	-*)			shift ;;
	*)			break ;;
	esac
done

for var in home link linkdir size handler id source dest
do
	case $# in
	0)	eval "$var=-"
		;;
	*)	eval "$var=\"\$1\""
		shift
		;;
	esac
done

# Compression algorithm can be link-dependant:

case "$linkdir" in
has_16_bit)	COMPRESS='compress -f'	# More efficient
		;;
has_gzip)	COMPRESS='gzip -f'	# Even better
		UNCOMPRESS=gunzip
		;;
esac

# TRACE -- enable for tests
# echo "`date +'%y/%m/%d %H:%M'` $dir $home $link $size $id $flags" >>"$LOG"

#	Data comes in on stdin, changed data should go out on stdout.

case "$dir" in
OUT)	case "$size" in
	????*)		;;
	*)	exit 0	;;	# < 1000 bytes -- don't bother
	esac
	$COMPRESS &&
	echo "COMPRESSED@$home" 1>&2 &&
	exit 40	# Data changed, flag on stderr
	;;
*)	case "$flags" in
	*COMPRESSED@$link\;)
		$UNCOMPRESS &&
		echo "DELETE COMPRESSED@$link" 1>&2 &&
		exit 40	# Data changed, delete flag on stderr
		echo "Error [$?] return from: $UNCOMPRESS" 1>&2
		exit 128 # ERROR: data unchanged, error on stderr.
		;;
	*COMPRESSED@$home\;)
		$UNCOMPRESS &&
		echo "DELETE COMPRESSED@$home" 1>&2 &&
		exit 40	# Data changed, delete flag on stderr
		echo "Error [$?] return from: $UNCOMPRESS" 1>&2
		exit 128 # ERROR: data unchanged, error on stderr.
		;;
	esac
	;;
esac

exit 0	# Data unchanged
