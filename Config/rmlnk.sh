#!/bin/sh
#
#	remove a link
#
echo=/bin/echo
trap 'menu links.menu; exit 0' 2
$echo $n "What is the node name of the remote machine? $c"
read link
case "$link" in
'')	echo "No link specified - press return to continue$c"; read x; exit 1;;	
esac

if [ -f $link.lnk ]
then
	echo
	echo 'Existing link information is:'
	echo
	cat $link.lnk
	echo
	if answeryes "Are you sure you want to remove the link to \"$link\"? "
	then
		mv $link.lnk $link.brk;
		echo "Link to \"$link\" removed."
	fi
else
	echo "There is no link called \"$link\""
	echo
fi
	
$echo $n "- hit return to continue -$c"
read x
