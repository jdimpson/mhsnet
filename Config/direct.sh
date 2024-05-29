#!/bin/sh
#
#	modify or create link information
#
trap 'exit 0' 2
echo '
	- Network address of remote machine -
'
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
	if answeryes "Does the other system establish the connection? "
	then
		directin.sh || exit 1
	fi
	if answeryes "Does your system establish the connection? "
	then
		directout.sh || exit 1
	fi
	restrict.sh || exit 1
fi

while true
do
	echo
	echo "The link information for $link is:"
	echo
	cat $link.lnk
	echo
	if answerno "Do you want to change this information? "
	then
		exit 0
	fi
	sed -e "/type:.*directin/d" ${link}.lnk >${link}.tmp
	mv ${link}.tmp ${link}.lnk
	if answeryes "Does the other system establish the connection? "
	then
		directin.sh || exit 1
	fi
	
	if answeryes "Does your system establish the connection? "
	then
		if grep -s "type:	directout" $link.lnk >/dev/null
		then
			chngdirout.sh || exit 1
		else
			directout.sh || exit 1
		fi
	else
		sed	-e "/type:.*directout/d" \
			-e "/^crontime/d" \
			-e "/^login name/d" \
			-e "/^password/d" \
			-e "/^line speed/d" \
			-e "/^modem device/d" \
			-e "/^link flags/d" \
		${link}.lnk >${link}.tmp
		mv ${link}.tmp ${link}.lnk
	fi
	restrict.sh || exit 1
done
