#!/bin/sh
#
#	modify or create link information
#
trap 'exit 0' 2
echo '
	- Network address of remote machine -
'
getaddr.sh || exit 1
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
	if answeryes "Is this a dial IN connection? "
	then
		dialin.sh || exit 1
	fi
	if answeryes "Is this a dial OUT connection? "
	then
		dialout.sh || exit 1
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
	sed -e "/type:.*dialin/d" ${link}.lnk >${link}.tmp
	mv ${link}.tmp ${link}.lnk
	if answeryes "Is this a dial IN connection? "
	then
		dialin.sh || exit 1
	fi

	if answeryes "Is this a dial OUT connection? "
	then
		if grep -s "type:.*dialout" $link.lnk >/dev/null
		then
			chngdialout.sh || exit 1
		else
			dialout.sh || exit 1
		fi
	else
		sed     -e "/type:.*dialout/d" \
			-e "/^crontime/d" \
			-e "/^modem type/d" \
			-e "/^modem device/d" \
			-e "/^login name/d" \
			-e "/^password/d" \
			-e "/^phone number/d" \
			-e "/^line speed/d" \
			-e "/^line fixspeed/d" \
			-e "/^link flags/d" \
		${link}.lnk >${link}.tmp
		mv ${link}.tmp ${link}.lnk
	fi
	restrict.sh || exit 1
done
