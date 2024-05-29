#!/bin/sh
#
#	Modify or create tcp link information.
#

case "$SYSTEM" in
'')	TCP=		# exos/sco_xenix/sco_unix/HP/AIX
	;;
*)	TCP=$SYSTEM
	;;
esac

case "$TCP" in
none)		echo "TCP/IP support not installed."
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
		echo "
	A serious error has occurred, a necessary file cannot
	be created. Your system may not be installed correctly.
	Please consult your support service."
		sleep 2; exit 1; }
	echo "link address:	$saddr" >>$link.lnk
fi

while true
do
        sed -e "/type:/d" -e "/remote alias:/d" -e "/password:/d" -e "/reliable:/d" ${link}.lnk >${link}.tmp
        mv ${link}.tmp ${link}.lnk

	X=`getdefans "IP address of remote machine?" "$saddr"`
		echo "remote alias:	$X"	>>$link.lnk
	
	if answeryes "Is this a reliable TCP/IP connection (eg: via local Ethernet)? "
	then
		echo "reliable:"		>>$link.lnk
	fi

	if answeryes "Is this an incoming TCP/IP connection (will they call you)? "
	then
		tcpin=1
		case "$TCP" in
		exos)	echo "type:	tcpin"	>>$link.lnk;;
		*)	echo "type:	TCPin"	>>$link.lnk;;
		esac
		if answeryes "Do you want to set a network password for them? "
		then
			if test -s ../_state/routefile
			then
				netpasswd $addr
			else
				echo "Please do \"netpasswd $addr\" after starting the network"
			fi
		fi
	fi
	
	if answeryes "Is this an outgoing TCP/IP connection (are you calling them)? "
	then
		tcpout=1
		echo "type:	tcpout"		>>$link.lnk

		echo "	The next query refers to a \"netpasswd\" installed"
		echo "	at the remote site, (if none, type RETURN):"
		X=`getans "What is the password?"`
		echo "password:	$X"		>>$link.lnk
	fi
	
	restrict.sh || exit 1

	echo "
The link information for $link is:
"
	cat $link.lnk
	echo

	if answerno "Do you want to change this information? "
	then
		break
	fi
done

case "$tcpin$tcpout" in
1|11)	;;
*)	exit 0;;
esac

fixinetd=
needsleep=

if [ -s /etc/services ]
then
	if grep -s "^mhsnet" /etc/services >/dev/null
	then
		echo "MHSnet entry already installed in /etc/services"
	else
		echo "Installing MHSnet entry in /etc/services"
		echo 'mhsnet	1989/tcp	mhsnet' >>/etc/services
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
	file=/etc/xinetd.d/mhsnet
else
	file=
fi

case "$file" in
'')	;;
*xinetd*)
	echo "Installing MHSnet entry in $file"
	cp xinetd.d-tcp $file
	;;
*)
	if grep -s "^mhsnet.*tcp" $file >/dev/null
	then
		if grep -s "^mhsnet.*tcp.*$SPOOLDIR" $file >/dev/null
		then
			echo "MHSnet entry already installed in $file"
		else
			echo "Updating MHSnet entry in $file"
			sed '/^mhsnet.*tcp/d' <$file >/tmp/inetd.$$ &&
			cp /tmp/inetd.$$ $file
			rm -f /tmp/inetd.$$
			fixinetd=true
		fi
	else
		echo "Installing MHSnet entry in $file"
		fixinetd=true
	fi
	case "$fixinetd" in
	true)
		case "$TCP" in
		sco_unix)	echo "mhsnet	stream	tcp	nowait	root	/bin/su su root -c $SPOOLDIR/_lib/tcpshell" >>$file;;
		*)		echo "mhsnet	stream	tcp	nowait	root	$SPOOLDIR/_lib/tcpshell tcpshell" >>$file;;
		esac
		;;
	esac
	needsleep=true
	;;
esac

if [ -s /etc/servers ]
then
	if grep -s "^mhsnet.*tcp" /etc/servers >/dev/null
	then
		echo "MHSnet entry already installed in /etc/servers"
	else
		echo "Installing MHSnet entry in /etc/servers"
		echo "mhsnet	tcp	$SPOOLDIR/_lib/tcpshell" >>/etc/servers
		fixinetd=true
	fi
	needsleep=true
fi

case "$fixinetd" in
true)	case "$TCP" in
	sco*)	echo "Inetd should be signaled for altered TCP/IP configuration."
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
