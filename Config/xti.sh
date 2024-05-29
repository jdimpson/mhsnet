#!/bin/sh
#
#	modify or create XTI link information
#

case "$XTI" in
none)	echo "XTI interface not installed."; sleep 3
	exit 0;;
*)	echo "XTI interface not supported."; sleep 3
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
	sed -e "/type:.*xtiin/d" ${link}.lnk >${link}.tmp
	mv ${link}.tmp ${link}.lnk
	if answeryes "Is this an incoming XTI connection? "
	then
		echo "type:	xtiin"		>>$link.lnk
		echo
		echo "An entry must be added to the XTI configuration file."
		echo 
	fi
	
	sed -e "/type:.*xtiout/d" ${link}.lnk >${link}.tmp
	mv ${link}.tmp ${link}.lnk
	if answeryes "Is this an outgoing XTI connection? "
	then
		echo "type:	xtiout"			>>$link.lnk

		echo "	What is the XTI circuit designation (a/b) for the remote site?"
		X=`getans "What is the circuit designation?" "The circuit designation must be specified"`
		text -n "$X" && echo "circuit:	$X"	>>$link.lnk

		echo "	What is the CMX global name for the remote site?"
		echo "	(if none, type RETURN):"
		X=`getans "What is the global name?"`
		text -n "$X" && echo "cmx_remote:	$X"	>>$link.lnk

		echo "	What is the XTI local name for the remote site?"
		echo "	(if none, type RETURN):"
		X=`getans "What is the local name?"`
		text -n "$X" && echo "cmx_local:	$X"	>>$link.lnk

		echo "	What is the XTI transport address for the remote site?"
		echo "	(if none, type RETURN):"
		X=`getans "What is the transport address?"`
		text -n "$X" && echo "address:	$X"	>>$link.lnk
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
