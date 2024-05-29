#!/bin/sh

loginname=`grep '^addr' site.details|sed 's/.*NODE=\([^\.]*\).*/\1/'`

case "$loginname" in '') echo "You need to create your site details first."; sleep 2; exit 1;; esac

loginname=`getdefans "Login name at remote machine?" "$loginname"`

echo "
	Your machine will login at the remote machine using the login
	name \"$loginname\". The remote machine should be set up to accept
	MHSnet connections from your machine using that account.
	The account should be given a password.
"
password=`getans "What is the password?"`

linespeed=`getdefans "What is the line speed of the connection?" $defspeed`
modemdev=`getdefans "What serial line device is to be used for the connection?" $defdev`

. getcron.sh

echo "crontime:	\"$crontime\""		>>$link.lnk
echo "login name:	$loginname"	>>$link.lnk
echo "password:	$password"		>>$link.lnk
echo "line speed:	$linespeed"	>>$link.lnk
echo "modem device:	$modemdev"	>>$link.lnk
echo "type:	directout"	>>$link.lnk

case "$crontime" in
restart)echo 'link flags:	-call'		>>$link.lnk
	;;
*)	if answeryes "Do you want the connection made on demand (auto-call)? "
	then
		echo 'link flags:	+call'		>>$link.lnk
	else
		echo 'link flags:	-call'		>>$link.lnk
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
	echo 'link filter:'			>>$link.lnk
else
	echo 'link filter:	filter43'	>>$link.lnk
fi
