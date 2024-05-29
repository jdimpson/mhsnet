#!/bin/sh
#
#	`Drop' shell script for `filter'.
#
#	SCCSID @(#)dropfilter.sh	1.4 96/04/26
#
#	Any inbound message matching handler/source/dest will be dropped.
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

# TRACE -- enable for tests
# echo `date +'%y/%m/%d %H:%M'` "$@" >>_stats/drop.log

for i
do
	case "$i" in
	-F*)	flags=$i;	shift ;;
	-i)	dir='IN';	shift ;;
	-o)	dir='OUT';	shift ;;
	-*)			shift ;;
	*)			break ;;
	esac
done

home="$1"
link="$2"
shift
linkdir="$2"
size="$3"
handler="$4"
id="$5"
source="$6"
dest="$7"
fuser="$8"
tuser="$9"

# TRACE -- enable for tests
# echo "`date +'%y/%m/%d %H:%M'` $dir $id $handler $source $dest" >>_stats/drop.log

#	Data comes in on stdin, changed data should go out on stdout.

case "$dir" in
IN)	case "$handler" in
	stater)
		case "$source" in
		9=gldsyd*|9=munnari*|9=murtoa*)
			case "$dest" in
			\*.4=cs*)
				echo `date`"DROPPED $dir $id $handler $source $dest" >>_stats/drop.log
				exit 64	# Drop message
				;;
			esac
			;;
		esac
		;;
	esac
	;;
esac

exit 0	# Data unchanged
