#!/bin/sh
#
#	Modify or create udp link information
#	(for UDP/IP to SUNIII site).
#

case "$SYSTEM" in
'')	UDP=		# exos/sco_xenix/sco_unix/HP/AIX/none
	;;
*)	UDP=$SYSTEM	# /none
	;;
esac

case "$UDP" in
none)		echo "UDP/IP support not installed."
		exit 0;;
esac

trap 'exit 0' 2
echo "
	- Network address of remote machine -
"
getaddr.sh
. ./getaddr.tmp
link=$node
export link
rm -f $link.brk

if [ ! -f $link.lnk ]
then
	echo "remote address:	$addr" >$link.lnk || {
		echo "A serious error has occurred, a necessary file cannot"
		echo "be created. Your system may not be installed correctly."
		echo "Please consult your support service."
		sleep 2; exit 1; }
	echo "link address:	$saddr" >>$link.lnk
fi

sed -e "/link filter:.*/d" ${link}.lnk >${link}.tmp
mv ${link}.tmp ${link}.lnk
echo 'link filter:	filter43' >>$link.lnk

while true
do
	sed -e "/type:.*/d" -e "/remote alias:.*/d" ${link}.lnk >${link}.tmp
	mv ${link}.tmp ${link}.lnk

	X=`getdefans "IP address of remote machine?" "$saddr"`
		echo "remote alias:	$X"	>>$link.lnk

	if answeryes "Is this an incoming UDP/IP connection? "
	then
		udpin=1
		case "$UDP" in
		exos)	echo "type:	udpin"		>>$link.lnk;;
		*)	echo "type:	UDPin"		>>$link.lnk;;
		esac
	fi
	
	if answeryes "Is this an outgoing UDP/IP connection? "
	then
		udpout=1
		echo "type:	udpout"		>>$link.lnk
	fi
	
	restrict.sh || exit 1

	echo
	echo "The link information for $link is:"
	echo
	cat $link.lnk
	echo
	if answerno "Do you want to change this information? "
	then
		break
	fi
done

case "$udpin$udpout" in
1|11)	;;
*)	exit 0;;
esac

fixinetd=
needsleep=

if [ -s /etc/services ]
then
	if grep -s "^acsnet.*udp" /etc/services >/dev/null
	then
		echo "ACSnet already entry installed in /etc/services"
	else
		echo "Installing ACSnet entry in /etc/services"
		echo 'acsnet	1986/udp	acsnet' >>/etc/services
	fi
	needsleep=true
fi

if [ -s /etc/inetd.conf ]
then
	file=/etc/inetd.conf
elif [ -s /usr/etc/inetd.conf ]
then
	file=/usr/etc/inetd.conf
elif [ -d /etc/xinetd.d ]
then
	file=/etc/xinetd.d/acsnet
else
	file=
fi

case "$file" in
'')	;;
*xinetd*)
	echo "Installing MHSnet entry in $file"
	cp xinetd.d-udp $file
	;;
*)
	if grep -s "^acsnet.*udp" $file >/dev/null
	then
		echo "ACSnet entry already installed in $file"
	else
		echo "Installing ACSnet entry in $file"
		case "$SYSTEM" in
		sco_unix)
			echo "acsnet	dgram	udp	wait	root	/bin/su su root -c $SPOOLDIR/_lib/ENshell" >>$file
			;;
		*)
			echo "acsnet	dgram	udp	wait	root	$SPOOLDIR/_lib/ENshell ENshell" >>$file
			;;
		esac
		fixinetd=true
	fi
	needsleep=true
	;;
esac

if [ -s /etc/servers ]
then
	if grep -s "^acsnet" /etc/servers >/dev/null
	then
		echo "ACSnet entry already installed in /etc/servers"
	else
		echo "Installing ACSnet entry in /etc/servers"
		echo "acsnet	udp	$SPOOLDIR/_lib/ENshell" >>/etc/servers
		fixinetd=true
	fi
	needsleep=true
fi

case "$fixinetd" in
true)	case "$UDP" in
	sco*)	echo "Inetd should be signaled for altered UDP/IP configuration."
		if answeryes "Do you want to restart inetd? "
		then
			set `ps -e|grep inetd` # 1st number is pid
			case "$1" in
			[1-9]*)	kill -1 $1;;
			*)	echo "inetd not running";;
			esac
		fi;;
	HP)	/etc/inetd -c; echo 'Inetd has been signalled to scan the altered "inetd.conf".';;
	AIX)	inetimp; refresh -s inetd; echo 'Inetd has been signalled to scan the altered "inetd.conf".';;
	*)	echo 'Inetd should be restarted to scan the altered "inetd.conf".';;
	esac
	needsleep=true
	;;
esac

case "$needsleep" in
true)	sleep 5;;
esac
