#!/bin/sh
#
#	Turn over accounting files.
#
#	NB: default removes any accounting files older than 6 months.
#
#	@(#)turn-acct.sh	1.16 96/03/30
#
COMPRESS=compress
chown=chown
PATH=/bin:/usr/bin:/etc:/etc/bin:/usr/ucb:/usr/local:/usr/local/bin:.
NETUSERNAME=daemon
SPOOLDIR=/usr/spool/MHSnet
eval `egrep 'SPOOLDIR|NETUSERNAME' /usr/lib/MHSnetparams 2>/dev/null`

cd $SPOOLDIR/_stats

if [ ! -f Accumulated -a ! -f connect ]
then
	exit 0	# Don't bother if stats turned off
fi

year=`date '+%y'`
month=`date '+%m'`
error="network statistics turnover: date '+%m' failed"

# Need last month

case $month
in
	01) month=12; year=`expr $year - 1`	;;
	02) month=01				;;
	03) month=02				;;
	04) month=03				;;
	05) month=04				;;
	06) month=05				;;
	07) month=06				;;
	08) month=07				;;
	09) month=08				;;
	10) month=09				;;
	11) month=10				;;
	12) month=11				;;
	*)  echo $error; exit 1			;;
esac

datenum=$year$month

mv Accumulated Acc${datenum}.bak 2>/dev/null 2>&1 && {
	>Accumulated
	$chown $NETUSERNAME Accumulated
	chmod 664 Accumulated

	mv New_stats Acc${datenum}.start
	>New_stats
	$chown $NETUSERNAME New_stats
	chmod 664 New_stats

	# nice -18 ../_lib/stats -tAcc${datenum}.bak >Acc${datenum}.stats
	# $chown $NETUSERNAME Acc${datenum}.stats
	# chmod 664 Acc${datenum}.stats

	nice -18 $COMPRESS Acc${datenum}.bak &
}

test -s servedfiles && {
	if test -w servedfiles
	then
		cp servedfiles servedf.${datenum}
		>servedfiles	# preserve owner
		$chown $NETUSERNAME servedf.${datenum}
		chmod 664 servedf.${datenum}
		nice -18 $COMPRESS servedf.${datenum}
	else
		eval `../_bin/netparams -w NCC_ADMIN NETGROUPNAME`
		../_bin/netmail -mort \
			LOGNAME=postmaster \
			NAME="MHSnet accounting management" \
			SUBJECT="Unwriteable file: $SPOOLDIR/_stats/servedfiles" \
			"Network Admin <$NCC_ADMIN>" <<!
$0 is unable to turn over the accounting file
	$SPOOLDIR/_stats/servedfiles
as it is unwritable by group $NETGROUPNAME.

Please do "chmod 664 $SPOOLDIR/_stats/servedfiles".
!
	fi
}

# Remove any accounting files older than 6 months.

files=`ls Acc* connect* servedf* 2>/dev/null`
rm -f `find $files -mtime +180 -print`

# Back another month

case $month
in
	01) month=12; year=`expr $year - 1`	;;
	02) month=01				;;
	03) month=02				;;
	04) month=03				;;
	05) month=04				;;
	06) month=05				;;
	07) month=06				;;
	08) month=07				;;
	09) month=08				;;
	10) month=09				;;
	11) month=10				;;
	12) month=11				;;
esac

datenum=$year$month

for i in connect.${datenum}[0-3][0-9]
do
	test -r $i || exit 0
done

cat connect.${datenum}[0-3][0-9] >connect.${datenum}
rm connect.${datenum}[0-3][0-9]
nice -18 $COMPRESS connect.${datenum}
