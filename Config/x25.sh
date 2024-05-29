#!/bin/sh
#
#	modify or create X25 link information
#

case "$X25" in
none)	echo "X25 support not installed."; sleep 3
	exit 0;;
*)	echo "X25 system not supported."; sleep 3
	exit 0;;
esac

trap 'exit 0' 2
echo
echo '	- Network address of remote machine -'
echo
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

while true
do
	sed -e "/type:.*x25in/d" ${link}.lnk >${link}.tmp
	mv ${link}.tmp ${link}.lnk
	if answeryes "Is this an incoming X25 connection? "
	then
		echo "type:	x25in"		>>$link.lnk
		echo
		echo "An entry must be added to the X25 services file."
		echo 
	fi
	
	sed -e "/type:.*x25out/d" ${link}.lnk >${link}.tmp
	mv ${link}.tmp ${link}.lnk
	if answeryes "Is this an outgoing X25 connection? "
	then
		echo "type:	x25out"		>>$link.lnk
	fi
	
	restrict.sh || exit 1

	echo
	echo "The link information for $link is:"
	echo
	cat $link.lnk
	echo
	if answerno "Do you want to edit this information? "
	then
		exit 0
	fi
done
