#!/bin/sh

loginname=`grep '^addr' site.details|sed 's/.*NODE=\([^\.]*\).*/\1/'`

case "$loginname" in '') echo "You need to create your site details first."; sleep 2; exit 1;; esac

loginname=`getdefans "Login name at remote machine?" "$loginname"`

echo "
	Your machine will login at the remote machine using the login
	name \"$loginname\". The remote machine should be set up to accept
	MHSnet connections from your machine using that account.
	The account should be given a login password, or a \"netpasswd\".
"
password=`getans "What is the password?"`

echo "
	To make the connection you need the phone number of the modem
	attached to the remote machine. The number must be given in a
	form suitable for your modem to use for dialling. For example,
	it might be prefixed with T for tone dialing or P for pulse
	dialing, it might include access codes for a PABX or long distance
	and it might include commas indicating a pause during dialing.
"
telno=`getans "What is the phone number of the remote machine?" "A phone number must be given."`
telno2=`getans "Optional 2nd phone number for the remote machine?"`

linespeed=`getdefans "What is the line speed of the connection?" $defspeed`
modemdev=`getdefans "What serial line device is to be used for the connection?" $defdev`

echo "
	You may select between several different modem types.

	All supported modems use the \"hayes\" command set, look
	in the file \"_call/hayes_2.cs\" for the various types. The
	common ones are \"hayes\", \"interlink\", \"trailblazerRTS\",
	(RTS/CTS flow control) and \"trailblazerXON\" (XON/XOFF flow control),
	and \"hayesXON\" for forcing XON/XOFF flow-control and \"cooked\" mode.

	NB: use \"XON\" if you are unsure, it should always work.
"
modem=`getdefans "What is the modem type?" $defmodem`

norestart=true
. getcron.sh

echo "crontime:	\"$crontime\""		>>$link.lnk
echo "modem type:	$modem"		>>$link.lnk
echo "modem device:	$modemdev"	>>$link.lnk
echo "login name:	$loginname"	>>$link.lnk
echo "password:	$password"		>>$link.lnk
echo "phone number:	$telno"		>>$link.lnk
echo "type:	dialout"		>>$link.lnk
case "$telno2" in
'')	;;
*)	echo "phone number2:	$telno2"	>>$link.lnk	;;
esac

echo "
	If the connection to the modem is at a fixed speed,
	(ie: should not change if the remote answers at a
	different speed) answer yes below.
"
if answeryes "Is the modem speed fixed ? "
then
	echo "line fixspeed:	$linespeed"	>>$link.lnk
else
	echo "line speed:	$linespeed"	>>$link.lnk
fi

echo "
	Does the modem (at either end of the connection) use
	XON/XOFF flow-control? If so the transport daemon will
	have to \"cook\" the data to avoid these characters.
	This is also necessary if the connection is via any
	device which interprets some of the input characters,
	eg: a terminal concentrator.
	If the circuit interferes with bytes other than just
	XON/XOFF then choose \"full cooking\".
"
if answeryes "Does the circuit need \"cooking\" ? "
then
	if answeryes "Does the circuit need full cooking? "
	then
		echo "line cooked:"		>>$link.lnk
	else
		echo "line cookedX:"		>>$link.lnk
	fi
fi

echo "
	The modem at the other end may be attached to an Annex
	terminal server - if so you must supply the name of
	the host running MHSnet to be reached via the Annex
	and, if the Annex has local security enabled, a user
	name and password.
"
if answeryes "Is the remote modem attached to an Annex ? "
then
	ahost=`getans "What is the MHSnet host name?" "A host name must be given"`
	echo "annexhost:	$ahost"		>>$link.lnk
	auser=`getans "What is the Annex user name?"`
	case "$auser" in '');; *)
		echo "annexuser:	$auser"	>>$link.lnk
		apass=`getans "What is the Annex password?"`
		case "$apass" in '');; *)
		    echo "annexpass:	$apass"	>>$link.lnk
		esac
	esac
	echo
fi

case "$crontime" in
restart)echo 'link flags:	-call'		>>$link.lnk
	;;
*)	if answeryes "Do you want the connection made on demand (auto-call)? "
	then
		echo 'link flags:	+call'	>>$link.lnk
	else
		echo 'link flags:	-call'	>>$link.lnk
	fi
	;;
esac

echo "
        SUN-III is a predecessor to MHSnet which is less flexible and lacks
        many of MHSnet's features. It also lacks MHSnet's advanced protocols.
        Some sites are still running SUN-III; generally you will be told if
        this is so.
"
if answerno "Is this a connection to an ACSnet site with SUN III? "
then
	echo "
	Do you wish to use the \"compress filter\"?

	WARNING: you must ensure that it is specified
	for *both* ends of the link, if you use it.
"
	if answeryes "Use compress filter? "
	then
	     echo 'link filter:	pressfilter'	>>$link.lnk
	else
		echo 'link filter:'		>>$link.lnk
	fi
else
	echo 'link filter:	filter43'	>>$link.lnk
fi
